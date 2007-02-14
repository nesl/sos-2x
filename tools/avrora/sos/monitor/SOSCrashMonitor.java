/**
 * @file SOSCrashMonitor.java
 * @brief Detailed crash report of SOS when it crashes
 * @author Ram Kumar {ram@ee.ucla.edu}
 */

package sos.monitor;

import avrora.monitors.Monitor;
import avrora.monitors.MonitorFactory;
import avrora.sim.Simulator;
import avrora.sim.State;
import avrora.core.Program;
import avrora.arch.AbstractInstr;
import avrora.arch.legacy.*;
import cck.util.Option;
import cck.util.Util;
import cck.text.TermUtil;
import cck.text.Terminal;
import cck.text.StringUtil;
import java.util.*;


public class SOSCrashMonitor extends MonitorFactory {

    public final Option.Long CRASHADDR = options.newOption("crash-addr", 0, "Specify byte address of crash site.");
    public final Option.Bool PROFBEFORE = options.newOption("profie-before-fire", false, "Profile before executing an instruction.");
    public final Option.Long SFIEXCEPTIONADDR = options.newOption("sfi-exception-addr", 0, "Specify byte address of crash site.");
    public final Option.Long TRACELEN  = options.newOption("trace-len", 100, "Specify length of intr. trace");
    public final Option.Long MEMMAPADDR = options.newOption("memmap-addr", 0, "Specify length of intr. trace");
    public final Option.Long MALLOCADDR = options.newOption("malloc-addr", 0, "Specify length of intr. trace");
    public final Option.Long FREEADDR = options.newOption("free-addr", 0, "Specify length of intr. trace");
    public final Option.Long CHOWNADDR = options.newOption("chown-addr", 0, "Specify length of intr. trace");
    public final Option.Long VMPOST = options.newOption("vm-post", 0, "Specify length of intr. trace");


    public class Mon implements avrora.monitors.Monitor {
	public final Simulator simulator;
	public int tracelen;
	public int tracePtr;
	public int[] tracePC;
	public int[] traceSP;
	public int[] traceR0;
	public int[] traceR1;
	public int[] traceR26;
	public int[] traceR27;
	public int[] traceR30;
	public int[] traceR31;
	public int memmap_addr;
	public int mallocaddr, freeaddr, chownaddr;
	public boolean profilebefore;

	Mon(Simulator s) {
	    simulator = s;
	    tracePtr = 0;
	    tracelen = (int)TRACELEN.get();
	    tracePC = new int[tracelen];
	    traceSP = new int[tracelen];
	    traceR0 = new int[tracelen];
	    traceR1 = new int[tracelen];
	    traceR26 = new int[tracelen];
	    traceR27 = new int[tracelen];
	    traceR30 = new int[tracelen];
	    traceR31 = new int[tracelen];

	    memmap_addr = (int)MEMMAPADDR.get();

	    int crashaddr = (int)CRASHADDR.get();
	    if (crashaddr != 0)
		s.insertProbe(new SOSCrashProbe(), crashaddr);

	    int sfiexceptionaddr = (int)SFIEXCEPTIONADDR.get();
	    if (sfiexceptionaddr != 0)
		s.insertProbe(new SOSSFIExceptionProbe(), sfiexceptionaddr);

	    mallocaddr = (int)MALLOCADDR.get();
	    if (mallocaddr != 0)
		s.insertProbe(new SOSMemoryMapProbe(), mallocaddr);

	    freeaddr = (int)FREEADDR.get();
	    if (freeaddr != 0)
		s.insertProbe(new SOSMemoryMapProbe(), freeaddr);

	    chownaddr = (int)CHOWNADDR.get();
	    if (chownaddr != 0)
		s.insertProbe(new SOSMemoryMapProbe(), chownaddr);
	    
	    int vmpost = (int)VMPOST.get();
	    if (vmpost != 0)
		s.insertProbe(new SOSDVMProbe(), vmpost);

	    profilebefore = (boolean)PROFBEFORE.get();

	    s.insertProbe(new SOSTraceProbe());
	    
	}


	public class SOSDVMProbe implements Simulator.Probe {
	    public void fireAfter(State state, int pc){
		Simulator simul;
		LegacyState legacystate = (LegacyState)state;
		int did = legacystate.getRegisterUnsigned(LegacyRegister.R24);
		int sid = legacystate.getRegisterUnsigned(LegacyRegister.R22);
		int type = legacystate.getRegisterUnsigned(LegacyRegister.R20);
		int len = legacystate.getRegisterUnsigned(LegacyRegister.R18);
		TermUtil.printSeparator(Terminal.MAXLINE, "DVM POST");
		TermUtil.reportQuantity("did: ", StringUtil.addrToString(did), " ");
		TermUtil.reportQuantity("sid: ", StringUtil.addrToString(sid), " ");
		TermUtil.reportQuantity("type: ", StringUtil.addrToString(type), " ");
		TermUtil.reportQuantity("len: ", StringUtil.addrToString(len), " ");
		simul = state.getSimulator();
		simul.stop();
	    }		

	    public void fireBefore(State state, int pc){
		return;
	    }
	}


	public class SOSMemoryMapProbe implements Simulator.Probe {
	    public void fireAfter(State state, int pc){
		LegacyState legacystate = (LegacyState)state;
		if (pc == mallocaddr)
		    Terminal.println("Malloc");
		if (pc == freeaddr)
		    Terminal.println("Free");
		if (pc == chownaddr)
		    Terminal.println("Msg Take Data");
		
		printMemoryMap(legacystate);
	    }
	    public void fireBefore(State state, int pc){
		return;
	    }
	}



	public class SOSSFIExceptionProbe implements Simulator.Probe {
	    private final int SFI_HEAP_EXCEPTION = 3;
	    public void fireBefore(State state, int pc){
		Simulator simul;
		LegacyState legacystate = (LegacyState)state;
		int regval = legacystate.getRegisterByte(LegacyRegister.R24);
		simul = state.getSimulator();

		TermUtil.printSeparator(Terminal.MAXLINE, "SFI Exception");
		TermUtil.reportQuantity("Addr: ", StringUtil.addrToString(pc), " ");
		TermUtil.reportQuantity("Cycles: ", state.getCycles(), " ");
		TermUtil.reportQuantity("Stack: ", StringUtil.addrToString(state.getSP()), " ");
		TermUtil.reportQuantity("Excpetion Code (R24): ", StringUtil.addrToString(regval), " ");
		printExecTrace(simul);
		
		if (regval == SFI_HEAP_EXCEPTION)
		    printMemoryMap(legacystate);
		
		simul.stop();
	    }
	    
	    public void fireAfter(State state, int pc){
		return;
	    }
	}


	public class SOSCrashProbe implements Simulator.Probe {
	    public void fireBefore(State state, int pc){
		TermUtil.printSeparator(Terminal.MAXLINE, "Crash Dump");
		TermUtil.reportQuantity("Addr: ", StringUtil.addrToString(pc), " ");
		TermUtil.reportQuantity("Cycles: ", state.getCycles(), " ");
		TermUtil.reportQuantity("Stack: ", StringUtil.addrToString(state.getSP()), " ");
		printExecTrace(state.getSimulator());
		return;
	    }
	    
	    public void fireAfter(State state, int pc){
		return;
	    }
	}


	public class SOSTraceProbe implements Simulator.Probe{
	    
	    public void fireBefore(State state, int pc){
		/*
		  LegacyState legacystate = (LegacyState)state;
		if (pc == 0xDF44){
		    Terminal.print("PC: ");
		    Terminal.print(StringUtil.addrToString(pc));
		    Terminal.nextln();
		    Terminal.print(StringUtil.addrToString(legacystate.getProgramByte(pc)));
		    Terminal.nextln();
		    Terminal.print(StringUtil.addrToString(legacystate.getProgramByte(pc+1)));
		    Terminal.nextln();
		}
		*/
		
		if (profilebefore)
		    doSOSProfile(state, pc);
		return;
	    }

	    public void fireAfter(State state, int pc){
		if (!profilebefore)
		    doSOSProfile(state, pc);
		return;
	    }


	    public void doSOSProfile(State state, int pc){
		LegacyState legacystate = (LegacyState)state;
		tracePC[tracePtr] = pc;
		traceSP[tracePtr] = state.getSP();
		traceR0[tracePtr] = legacystate.getRegisterUnsigned(LegacyRegister.R0);
		traceR1[tracePtr] = legacystate.getRegisterUnsigned(LegacyRegister.R1);
		traceR26[tracePtr] = legacystate.getRegisterUnsigned(LegacyRegister.R26);
		traceR27[tracePtr] = legacystate.getRegisterUnsigned(LegacyRegister.R27);
		traceR30[tracePtr] = legacystate.getRegisterUnsigned(LegacyRegister.R30);
		traceR31[tracePtr] = legacystate.getRegisterUnsigned(LegacyRegister.R31);
		tracePtr = (tracePtr + 1) % (tracelen);
		return;
	    }

	}
	

	public void printMemoryMap(LegacyState l){
	    int rdByte;
	    int i;
	    TermUtil.printSeparator(Terminal.MAXLINE, "Memory Map");
	    for (i = 0; i < 256; i++){
		int lrec, hrec;
		if ((i % 16) == 0) Terminal.nextln();
		rdByte = (int)l.getDataByte(memmap_addr + i);
		rdByte &= 0xFF;
		lrec = rdByte % 16;
		hrec = rdByte / 16;
		printMemoryMapByte(2*i, lrec);
		printMemoryMapByte((2*i) + 1, hrec);
	    }
	    Terminal.nextln();
	    return;
	}

	public void printMemoryMapByte(int blknum, int mrec){
	    int domid;
	    domid = mrec % 8;
	    //	    Terminal.print(StringUtil.toDecimal((long)blknum, 3));
	    if (mrec > 8)
		Terminal.print(")(");
	    else
		Terminal.print("--");
	    Terminal.print(StringUtil.toBin((long)domid, 3));
	    return;
	}

	public void printExecTrace(Simulator s){
	    int i;
	    //	    Program p;
	    //	    AbstractInstr instr;
	    //	    p = s.getProgram();

	    TermUtil.printThinSeparator();

	    for (i = tracePtr - 2; i >= 0; i--){
		//		instr = p.readInstr(tracePC[i]);
		Terminal.print("Addr: ");
		Terminal.print(StringUtil.addrToString(tracePC[i]));
		Terminal.print(" SP: ");
		Terminal.print(StringUtil.addrToString(traceSP[i]));
		Terminal.print(" R0: ");
		Terminal.print(StringUtil.addrToString(traceR0[i]));
		Terminal.print(" R1: ");
		Terminal.print(StringUtil.addrToString(traceR1[i]));
		Terminal.print(" R26: ");
		Terminal.print(StringUtil.addrToString(traceR26[i]));
		Terminal.print(" R27: ");
		Terminal.print(StringUtil.addrToString(traceR27[i]));
		Terminal.print(" R30: ");
		Terminal.print(StringUtil.addrToString(traceR30[i]));
		Terminal.print(" R31: ");
		Terminal.println(StringUtil.addrToString(traceR31[i]));
	    }

	    for (i = (tracelen-1); i >= tracePtr; i--){
		Terminal.print("Addr: ");
		Terminal.print(StringUtil.addrToString(tracePC[i]));
		Terminal.print(" SP: ");
		Terminal.print(StringUtil.addrToString(traceSP[i]));
		Terminal.print(" R0: ");
		Terminal.print(StringUtil.addrToString(traceR0[i]));
		Terminal.print(" R1: ");
		Terminal.print(StringUtil.addrToString(traceR1[i]));
		Terminal.print(" R26: ");
		Terminal.print(StringUtil.addrToString(traceR26[i]));
		Terminal.print(" R27: ");
		Terminal.print(StringUtil.addrToString(traceR27[i]));
		Terminal.print(" R30: ");
		Terminal.print(StringUtil.addrToString(traceR30[i]));
		Terminal.print(" R31: ");
		Terminal.println(StringUtil.addrToString(traceR31[i]));
		//		instr = p.readInstr(tracePC[i]);
		// 	TermUtil.reportQuantity(StringUtil.addrToString(tracePC[i]),
		// 					instr.getName(),
		// 					"SP:" + StringUtil.addrToString(traceSP[i]));
	    }
	}


	public void report(){
	    return;
	}
       

    }

    public SOSCrashMonitor(){
	super("The \"SOS Crash Monitor\" inserts a probe at SOS crash site " +
	      "and dumps detailed state report to screen.");
    }
    
    public avrora.monitors.Monitor newMonitor(Simulator s){
	return new Mon(s);
    }

}