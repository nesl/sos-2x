/**
 * Copyright (c) 2004-2005, Regents of the University of California
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * Neither the name of the University of California, Los Angeles nor the
 * names of its contributors may be used to endorse or promote products
 * derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

package sos.monitor;

import avrora.monitors.Monitor;
import avrora.monitors.MonitorFactory;

import avrora.sim.Simulator;
import sos.monitor.IntegerMemPrint;
import avrora.sim.util.*;
import cck.text.Terminal;
import cck.text.*;
import cck.util.Util;
import cck.util.Option;

/**
 * The <code>HeapPrintMonitor</code>  Print the heap usage 
 *
 * @author Simon Han
 */
public class HeapPrintMonitor extends MonitorFactory {
	public final Option.Long BASEADDR = options.newOption("base-addr", 0, "Specify the base address of the heap (mallo.c)");

    public class Monitor implements avrora.monitors.Monitor {
        public final IntegerMemPrint memprofile0;
        public final IntegerMemPrint memprofile1;
        public final IntegerMemPrint memprofile2;
        public final IntegerMemPrint memprofile3;
        public final IntegerMemPrint memprofile4;
        public final IntegerMemPrint memprofile5;
        public final IntegerMemPrint memprofile6;
        public final IntegerMemPrint memprofile7;
        public final IntegerMemPrint memprofile8;
        private static final int BASE = 418;
		public final Simulator sim;
		
        Monitor(Simulator s) {
			int b = (int)BASEADDR.get();

			if( b == 0 ) {
				b = BASE;
			}
			memprofile0 = new IntegerMemPrint(b, 2, new String("malloc_efrag"), false);
            s.insertWatch(memprofile0, b);

			memprofile1 = new IntegerMemPrint(b+2, 2, new String("malloc_ifrag"),false);
            s.insertWatch(memprofile1, b+2);

			memprofile2 = new IntegerMemPrint(b+4, 2, new String("num_blocks"), true);
            s.insertWatch(memprofile2, b+4);

			memprofile3 = new IntegerMemPrint(b+6, 2, new String("alloc"), true);
            s.insertWatch(memprofile3, b+6);
			memprofile4 = new IntegerMemPrint(b+8, 2, new String("num_outstanding"), true);
            s.insertWatch(memprofile4, b+8);
			memprofile5 = new IntegerMemPrint(b+10, 2, new String("gc_bytes"), false);
            s.insertWatch(memprofile5, b+10);

			memprofile6 = new IntegerMemPrint(b+12, 1, new String("alloc_pid"), true);
            s.insertWatch(memprofile6, b+12);
			memprofile7 = new IntegerMemPrint(358, 1, new String("ping_succ"), true);
            s.insertWatch(memprofile7, 358);
			memprofile8 = new IntegerMemPrint(359, 2, new String("data_node_id"), true);
            s.insertWatch(memprofile8, 359);
			sim = s;
        }

        public void report() {
			String idstr = SimUtil.getIDTimeString(sim);	
			double temp;

			synchronized ( Terminal.class ) {
				Terminal.println("Heap Profile: ");
				Terminal.println(" name                       count         avg       std          max        min");
				TermUtil.printThinSeparator(Terminal.MAXLINE);
				memprofile0.report();
				memprofile1.report();
				memprofile3.report();
				memprofile5.report();
			}
        }
    }

    public HeapPrintMonitor() {
        super("The \"print\" monitor watches a dedicated range of SRAM for instructions " +
                "to print a string or int to the screen.  Be careful to set the BASE variable " +
                "to point to a block of RAM not otherwise used by your app.");
    }

    public avrora.monitors.Monitor newMonitor(Simulator s) {
        return new Monitor(s);
    }
}
