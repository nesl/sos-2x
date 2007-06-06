package sos.monitor;

import java.nio.*;


public class TreeRoutingPacket
{
	// Based on constant values in tree_routing.h
	// Change these constants when changing tree_routing.h
	static private final int TREE_ROUTING_PID = 141;
	static private final int MSG_BEACON_PKT = 33;
	static private final int MSG_TR_DATA_PKT = 34;
	static private final int BEACON_PKT_MIN_LEN = 6;
	static private final int TR_DATA_PKT_MIN_LEN = 9;


	private boolean valid;
	private int type;
	private int seqno;
	private int parent;
	private int hopcount;
	private int estEntries;
	private int originaddr;
	private int originhopcount;
	private int dst_pid;
	private byte data[];




	public TreeRoutingPacket(SOSPacket sospkt)
	{
		if (sospkt.isValid())
		{
			if ((sospkt.get_dstMod() == TREE_ROUTING_PID) && 
					(sospkt.get_srcMod() == TREE_ROUTING_PID) &&
					((sospkt.get_type() == MSG_BEACON_PKT) ||
					 (sospkt.get_type() == MSG_TR_DATA_PKT)))
			{
				if (sospkt.get_type() == MSG_BEACON_PKT)
				{
					if (sospkt.get_length() >= BEACON_PKT_MIN_LEN)
					{
						valid = true;
						ByteBuffer b = ByteBuffer.allocate(256);
						int i;
						b.put(sospkt.get_data());
						b.rewind();
						b.order(ByteOrder.LITTLE_ENDIAN);
						seqno = b.getShort();
						parent = unsign(b.getShort());
						hopcount = unsign(b.get());
						estEntries = unsign(b.get());
						originaddr = 0;
						originhopcount = 0;
						dst_pid = 0;
						type = MSG_BEACON_PKT;
						i = 0;
						//                      while (b.hasRemaining() &&
						//                             (i < (sospkt.get_length() - BEACON_PKT_MIN_LEN)))
						//                         data[i] = b.get();
					}
					else
						valid = false;
				}
				else
				{
					if (sospkt.get_length() >= TR_DATA_PKT_MIN_LEN)
					{
						valid = true;
						ByteBuffer b = ByteBuffer.allocate(256);
						int i;
						b.put(sospkt.get_data());
						b.rewind();
						b.order(ByteOrder.LITTLE_ENDIAN);
						originaddr = unsign(b.getShort());
						seqno = b.getShort();
						hopcount = unsign(b.get());
						originhopcount = unsign(b.get());
						dst_pid = unsign(b.get());
						b.get();   // get the reserved byte
						parent = unsign(b.getShort());
						estEntries = 0;
						type = MSG_TR_DATA_PKT;
						i = 0;
						//                      while (b.hasRemaining() &&
						//                             (i < (sospkt.get_length() - TR_DATA_PKT_MIN_LEN)))
						//                         data[i] = b.get();
					}
					else
						valid = false;
				}
			}
			else
				valid = false;

		}
		else
			valid = false;
	}

	public boolean isValid(){
		return valid;
	}

	public StringBuffer ToStringBuffer()
	{
		StringBuffer buff = new StringBuffer();
		if (valid)
		{
			if (type == MSG_TR_DATA_PKT)
			{
				buff.append("<TR DATA> ");
				buff.append(" Origin: ");
				buff.append(originaddr);
				buff.append(" Seq. No: ");
				buff.append(seqno);
				buff.append(" HopCount: ");
				buff.append(hopcount);
				buff.append(" Origin HopCount: ");
				buff.append(originhopcount);
				buff.append(" Dest PID: ");
				buff.append(dst_pid);
				buff.append(" Parent Addr: ");
				buff.append(parent);
			}
			else
			{
				buff.append("<TR BEACON> ");
				buff.append(" Seq No: ");
				buff.append(seqno);
				buff.append(" Parent: ");
				buff.append(parent);
				buff.append(" HopCount: ");
				buff.append(hopcount);
				buff.append(" estEntries: ");
				buff.append(estEntries);
			}
		}
		else
			buff.append("Invalid Tree Routing Packet");
		return buff;
	}


	int unsign(short sign) {
		int pos = sign;
		if(pos < 0) {
			pos += 65536;
		}
		return pos;
	}

	int unsign(byte sign) 
	{
		int pos = sign;
		if(pos < 0) {
			pos += 256;
		}
		return pos;
	}
}

