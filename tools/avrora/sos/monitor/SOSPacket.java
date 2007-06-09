package sos.monitor;


public class SOSPacket {
    private boolean valid;
    private int dstMod;
    private int srcMod;
    private int dstAdr;
    private int srcAdr;
    private int type;
    private int length;
    private byte data[];
	private int buf_size;
    static final private int SOS_HEADER_SIZE = 8;
    
    int unsign(byte sign) {
	int pos = sign;
	if(pos < 0) {
            pos += 256;
	}
	return pos;
    }
    
    public SOSPacket(byte buf[], int size) {
		buf_size = size;
		valid = true;
		if (size < SOS_HEADER_SIZE) valid = false;
		dstMod = unsign(buf[0]);
		srcMod = unsign(buf[1]);
		dstAdr = unsign(buf[2]) + unsign(buf[3]) * 256;
		srcAdr = unsign(buf[4]) + unsign(buf[5]) * 256;
		type = unsign(buf[6]);
		length = unsign(buf[7]);
		if (size < (length + SOS_HEADER_SIZE)) {
		   	valid = false;
		}
		data = new byte[length];
		if( valid == true ) {
			for(int i=0; i < length; i++) {
				data[i] = buf[SOS_HEADER_SIZE+i];
			}
		}
	}


	public boolean isValid()
	{
		return valid;   
	}

	public int get_dstMod()
	{
		return dstMod;
	}

	public int get_srcMod()
	{
		return srcMod;
    }

    public int get_type()
    {
	return type;   
    }
      
    public int get_length()
    {
	return length;
    }

    public byte[] get_data()
    {
	return data;   
    }
      
      
      
      

    public StringBuffer HeaderToStringBuffer(){
		StringBuffer buf = new StringBuffer();
		/*
		if (!valid) {
			return buf;
		}
		*/
		buf.append("DID: ");
		buf.append(dstMod);
		buf.append(" SID: ");
		buf.append(srcMod);
		buf.append(" DST ADDR: ");
		buf.append(dstAdr);
		buf.append(" SRC ADDR: ");
		buf.append(srcAdr);
		buf.append(" TYPE: ");
		buf.append(type);
		buf.append(" LEN: ");
		buf.append(length);
		return buf;
	}
      

    public StringBuffer ToStringBuffer(){
		StringBuffer buf = new StringBuffer();
		if (!valid)
		{
			if( buf_size == 3 ) {
				buf.append("== ACK ==");
			} else {
				buf.append("== Corrupted Packet! ==");
			}
			
			/*else if( buf_size > SOS_HEADER_SIZE ) {
				StringBuffer temp = HeaderToStringBuffer();
				buf.append(temp.toString());
				buf.append(" Packet size: ");
				buf.append(buf_size);
			}*/
			return buf;

		}
		StringBuffer temp = HeaderToStringBuffer();
		buf.append(temp.toString());
		int i;
		buf.append(" DATA: ");
		for (i = 0; i < length; i++)
		{
			int j = unsign(data[i]);
			buf.append(Integer.toHexString(j));
			buf.append(" ");
		}
		return buf;
	}      


}

