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
    static final private int SOS_HEADER_SIZE = 8;
    
    int unsign(byte sign) {
	int pos = sign;
	if(pos < 0) {
            pos += 256;
	}
	return pos;
    }
    
    public SOSPacket(byte buf[], int size) {
	valid = true;
	if (size < SOS_HEADER_SIZE) valid = false;
	dstMod = unsign(buf[0]);
	srcMod = unsign(buf[1]);
	dstAdr = unsign(buf[2]) + unsign(buf[3]) * 256;
	srcAdr = unsign(buf[4]) + unsign(buf[5]) * 256;
	type = unsign(buf[6]);
	length = unsign(buf[7]);
	if (size < (length + 7)) valid = false;
	data = new byte[length];
	for(int i=0; i < length; i++) {
            data[i] = buf[8+i];
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
	if (!valid) 
	    {
		buf.append("Invalid packet");
		return buf;
	    }
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
		buf.append("Invalid packet");
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

