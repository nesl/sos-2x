/**
 * @brief Monitor for SOS Packets in Avrora
 * @author Ram Kumar
 * @note Adopted from PacketMonitor.java in Avrora
 */

package sos.monitor;


import avrora.monitors.Monitor;
import avrora.monitors.MonitorFactory;
import avrora.sim.Simulator;
import avrora.sim.platform.Platform;
import avrora.sim.radio.Radio;
import avrora.sim.util.SimUtil;
import cck.text.*;
import cck.util.Option;
import java.util.Iterator;
import java.util.LinkedList;


// import avrora.monitors.Monitor;
// import avrora.monitors.MonitorFactory;
// import avrora.sim.Simulator;
// import avrora.sim.platform.Platform;
// import avrora.sim.radio.Radio;
// import cck.util.*;
// import java.util.Iterator;
// import java.util.LinkedList;


public class SOSPacketMonitor extends MonitorFactory {

	/**
	 * The minimum time in clock cycles between two back-to-back packet
	 * transmissions. The value is derived from the time taken to transfer
	 * a single byte over the air.
	 */
	public static final int INTER_PACKET_TIME = 2 * Radio.TRANSFER_TIME;

	protected Option.Bool PACKETS = options.newOption("show-packets", true, 
			"This option enables the printing of packets as they are transmitted.");
	protected Option.Bool PREAMBLE = options.newOption("show-preamble", false,
			"This option will show the preamble of a packet when it is printed out.");
	protected Option.Bool DISCARD = options.newOption("discard-first-byte", true,
			"This option will discard the first byte of a packet, since it is often jibberish.");
	protected Option.Bool TREE_PACKETS = options.newOption("show-tree-packets", true,
			"This option will also display tree routing packets");



	class Mon extends Radio.RadioProbe.Empty implements Monitor {
		// Simulator
		final Simulator simulator;
		final Platform platform;
		SimUtil.SimPrinter printer;

		// Current State
		PacketEndEvent packetEnd;
		LinkedList bytes;
		boolean radio_is_tx;


		// Stats
		int bytesTransmitted;
		int bytesReceived;
		int packetsTransmitted;
		int packetsReceived;

		// Options
		boolean showPackets;
		boolean discardFirst;
		boolean showPreamble;
		boolean showTreePackets;



		Mon(Simulator s) {
			simulator = s;
			printer = SimUtil.getPrinter(simulator, "monitor.packet");
			printer.enabled = true;
			platform = simulator.getMicrocontroller().getPlatform();
			Radio radio = (Radio)platform.getDevice("radio");
			radio.insertProbe(this);
			packetEnd = new PacketEndEvent();
			showPackets = PACKETS.get();
			discardFirst = DISCARD.get();
			showPreamble = PREAMBLE.get();
			showTreePackets = TREE_PACKETS.get();
			bytes = new LinkedList();
		}


		public void fireAtTransmit(Radio r, Radio.Transmission t) {
			radio_is_tx = true;
			simulator.removeEvent(packetEnd);
			simulator.insertEvent(packetEnd, INTER_PACKET_TIME);
			bytes.addLast(t);
			bytesTransmitted++;
		}

		public void fireAtReceive(Radio r, Radio.Transmission t) {
			radio_is_tx = false;
			simulator.removeEvent(packetEnd);
			simulator.insertEvent(packetEnd, INTER_PACKET_TIME);
			bytes.addLast(t);
			bytesReceived++;
		}


		void endPacket() {
			if (radio_is_tx)
				packetsTransmitted++;
			else
				packetsReceived++;
			if (showPackets) {
				StringBuffer buf = buildPacket();
				synchronized (Terminal.class) 
				{
					Terminal.println(buf.toString());
				}
			}
			bytes = new LinkedList();
		}

		private StringBuffer buildPacket() {
			StringBuffer buf = new StringBuffer();
			StringBuffer sospktstrbuffer;
			SimUtil.getIDTimeString(buf, simulator);
			if (radio_is_tx)
				Terminal.append(Terminal.COLOR_BRIGHT_CYAN, buf, "Packet Tx");
			else
				Terminal.append(Terminal.COLOR_BRIGHT_CYAN, buf, "Packet Rx");
			buf.append(": ");
			Iterator i = bytes.iterator();
			int cntr = 0;
			boolean inPreamble = true;
			boolean rxsosgroup = false;
			byte sospktbuf[] = new byte[256];
			int pktsize = 0;
			SOSPacket sospkt;
			TreeRoutingPacket trpkt;


			while ( i.hasNext() ) {
				cntr++;
				Radio.Transmission t = (Radio.Transmission)i.next();
				if ( cntr == 1 && discardFirst ) continue;
				if ( inPreamble && !showPreamble && t.data == (byte)0xAA ) continue;
				if ( inPreamble && !showPreamble && t.data == (byte)0x33 ) continue;
				if ( inPreamble && !showPreamble && t.data == (byte)0xCC ) continue;
				if ( inPreamble && !rxsosgroup )
				{
					rxsosgroup = true;
					continue;
				}
				inPreamble = false;
				if (pktsize < 255){
					sospktbuf[pktsize] = t.data;
					pktsize++;
				}
			}

			sospkt = new SOSPacket(sospktbuf, pktsize);
			sospktstrbuffer = sospkt.ToStringBuffer();

			if (showTreePackets)
			{
				trpkt = new TreeRoutingPacket(sospkt);
				if (trpkt.isValid()){
					sospktstrbuffer = trpkt.ToStringBuffer();
				}
			}


			buf.append(sospktstrbuffer.toString());
			return buf;
		}

		public void report() {
			TermUtil.reportQuantity("Bytes sent", bytesTransmitted, "");
			TermUtil.reportQuantity("Bytes received", bytesReceived, "");
			TermUtil.reportQuantity("Packets sent", packetsTransmitted, "");
			TermUtil.reportQuantity("Packets received", packetsReceived, "");
		}

		class PacketEndEvent implements Simulator.Event {
			public void fire() {
				endPacket();
			}
		}
	}

	/**
	 * create a new monitor
	 */
	public SOSPacketMonitor() {
		super("The \"packet\" monitor tracks packets sent and received by nodes in a sensor network.");
	}


	/**
	 * create a new monitor, calls the constructor
	 *
	 * @see avrora.monitors.MonitorFactory#newMonitor(avrora.sim.Simulator)
	 */
	public avrora.monitors.Monitor newMonitor(Simulator s) {
		return new Mon(s);
	}
}

