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

import avrora.sim.*;
import avrora.sim.util.*;
import cck.text.StringUtil;
import cck.text.Terminal;


/**
 * <code>IntegerMemPrint</code> collects statistics on the integer values stored in 
 * memory.
 *
 * @author Simon Han
 */
public class IntegerMemPrint extends Simulator.Watch.Empty {

    int base;
    int size;
	String name;
	boolean print;

	long cumul;
	long cumul_sqr;
	int count;
	long max;
	long min;

	public IntegerMemPrint(int b, int m, String n, boolean p) {
		base = b;
		size = m;
		name = n;
		print = p;

		cumul = 0;
		cumul_sqr = 0;
		max = 0;
		min = Long.MAX_VALUE;
    }

	public void report()
	{
		float avg = (float)cumul / count; 
		double std = Math.sqrt(((double)cumul_sqr / count) - (avg * avg));
		Terminal.println(" "  
				+StringUtil.leftJustify(name, 22)+"  "
				+StringUtil.rightJustify(count, 8)+"  "
				+StringUtil.rightJustify(avg, 10)+"  "
				+StringUtil.rightJustify((float)std, 10)+"  "
				+StringUtil.rightJustify((float)max, 9)+"  "
				+StringUtil.rightJustify((float)min, 9)
				);
	}

	void record(long a)
	{
		cumul += a;
		cumul_sqr += (a * a);
		max = Math.max(max, a);
		min = Math.min(min, a);
		count++;
	}

	public void fireAfterWrite(State state, int data_addr, byte value) {

		Simulator sim = state.getSimulator();
		AtmelInterpreter a = (AtmelInterpreter) sim.getInterpreter();
        String idstr = SimUtil.getIDTimeString(sim);
		int v;
		
		if( size == 2 ) {
			int b0 = a.getDataByte(base + 0);
			int b1 = a.getDataByte(base + 1);

			v = ((b1 & 0xff) << 8) + (b0 & 0xff);
		} else if ( size == 4 ) {
			int b0 = a.getDataByte(base + 0);
			int b1 = a.getDataByte(base + 1);
			int b2 = a.getDataByte(base + 2);
			int b3 = a.getDataByte(base + 3);

			v = ((b3 & 0xff) << 24) + ((b2 & 0xff) << 16) + ((b1 & 0xff) << 8) + (b0 & 0xff);
		} else if ( size == 1 ) {
			 v = a.getDataByte(base + 0) & 0xff;
		} else {
			return;
		}
		if( print) { 
			synchronized ( Terminal.class ) {
				Terminal.println(idstr + " " + name +  " " + v );
			}
		}
		record(v);
    }
}

