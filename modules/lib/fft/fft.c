
#include <sys_module.h>

#define fixed int16_t
#include "fft.h"

#define N_WAVE          1024    /* dimension of Sinewave[] */
#define LOG2_N_WAVE     10      /* log2(N_WAVE) */
#define N_LOUD          100 /* dimension of Loudampl[] */
#define M        10
#define NFFT    (1<<M)




static int16_t fix_fft(func_cb_ptr cb, fixed *fr, fixed *fi, int16_t m, int16_t inverse);

static int8_t module_msg_handler(void *start, Message *e);

static const mod_header_t mod_header SOS_MODULE_HEADER = {
	.mod_id         = DFLT_APP_ID1,
	.state_size     = 0,
	.num_sub_func   = 0,
	.num_prov_func  = 0,
	.platform_type  = HW_TYPE /* or PLATFORM_ANY */,
	.processor_type = MCU_TYPE,
	.code_id        = ehtons(DFLT_APP_ID1),
	.module_handler = module_msg_handler,
	.funct          = {
		{fix_fft, "cCw2", DFLT_APP_ID1, 1},
	},          
};       
static int8_t module_msg_handler(void *start, Message *e)
{
	return SOS_OK;
}

#define FIX_MPY(DEST,A,B)       DEST = (int16_t)(((uint32_t)(A) * (uint32_t)(B))>>15)
static fixed fix_mpy(fixed a, fixed b)
{
	FIX_MPY(a,a,b);
	//        a = a*b;
	return a;
}

static int16_t fix_fft(func_cb_ptr cb, fixed *fr, fixed *fi, int16_t m, int16_t inverse)
{
	int16_t mr,nn,i,j,l,k,istep, n, scale, shift;
	fixed qr,qi,tr,ti,wr,wi; //,t;

	n = 1<<m;

	if(n > N_WAVE)
		return -1;

	mr = 0;
	nn = n - 1;
	scale = 0;

	/* decimation in time - re-order data */
	for(m=1; m<=nn; ++m) {
		l = n;
		do {
			l >>= 1;
		} while(mr+l > nn);
		mr = (mr & (l-1)) + l;

		if(mr <= m) continue;
		tr = fr[m];
		fr[m] = fr[mr];
		fr[mr] = tr;
		ti = fi[m];
		fi[m] = fi[mr];
		fi[mr] = ti;
	}

	l = 1;
	k = LOG2_N_WAVE-1;
	while(l < n) {
		if(inverse) {
			/* variable scaling, depending upon data */
			shift = 0;
			for(i=0; i<n; ++i) {
				j = fr[i];
				if(j < 0)
					j = -j;
				m = fi[i];
				if(m < 0)
					m = -m;
				if(j > 16383 || m > 16383) {
					shift = 1;
					break;
				}
			}
			if(shift)
				++scale;
		} else {
			/* fixed scaling, for proper normalization -
			there will be log2(n) passes, so this
			results in an overall factor of 1/n,
			distributed to maximize arithmetic accuracy. */
			shift = 1;
		}
		/* it may not be obvious, but the shift will be performed
		on each data point exactly once, during this pass. */
		istep = l << 1;
		for(m=0; m<l; ++m) {
			j = m << k;
			/* 0 <= j < N_WAVE/2 */
#ifdef MICA2_PLATFORM
			wr =  pgm_read_word(&Sinewave[j+N_WAVE/4]);
			wi = -1 * pgm_read_word(&Sinewave[j]);
#else
			wr =  Sinewave[j+N_WAVE/4];
			wi = -Sinewave[j];
#endif
			if(inverse)
				wi = -wi;
			if(shift) {
				wr >>= 1;
				wi >>= 1;
			}
			for(i=m; i<n; i+=istep) {
				j = i + l;
				tr = fix_mpy(wr,fr[j]) -
					fix_mpy(wi,fi[j]);
				ti = fix_mpy(wr,fi[j]) +
					fix_mpy(wi,fr[j]);
				qr = fr[i];
				qi = fi[i];
				if(shift) {
					qr >>= 1;
					qi >>= 1;
				}
				fr[j] = qr - tr;
				fi[j] = qi - ti;
				fr[i] = qr + tr;
				fi[i] = qi + ti;
			}
		}
		--k;
		l = istep;
	}

	return scale;
}


#ifndef _MODULE_
mod_header_ptr fft_get_header()
{
	return sos_get_header_address(mod_header);
}
#endif

