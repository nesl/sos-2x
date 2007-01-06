#include <hardware.h>
#include <sys/time.h>

static struct timeval start_time;

uint32_t ker_systime32();

uint16_t ker_systime16L()
{
	return (uint16_t)(ker_systime32() & 0x0000ffff);
}

uint16_t ker_systime16H()
{
	return (uint16_t)(ker_systime32() >> 16);
}

uint32_t ker_systime32()
{
	uint32_t	avr_time;
    struct timeval t_now, t_result;
    
    gettimeofday(&t_now, NULL);
    timersub(&t_now, &start_time, &t_result);
    
    // Divide 8.68 to simulate AVR processor at 7.37MHz with 1/64 scale
    avr_time = (uint32_t)((float)((uint32_t)t_result.tv_sec*1000000 + t_result.tv_usec)/8.68);

    if((avr_time & 0xFFFF0000) >= 0x7F000000)
    {
    	avr_time &= 0x0000FFFF;
    	if(t_now.tv_usec + avr_time > 1000000) {
    		start_time.tv_sec = t_now.tv_sec + 1;
    		start_time.tv_usec = t_now.tv_usec + avr_time - 1000000;
		}
		else {
			start_time.tv_sec = t_now.tv_sec;
			start_time.tv_usec = t_now.tv_usec + avr_time;
		}
	}
    	
    return avr_time;
}

void systime_init()
{
	gettimeofday(&start_time, NULL);

}

//It returns (10*ticks)/SYSTIME_FREQUENCY, however it's written in such a
//way that there is no integer overflow after the multiplication 10*ticks,
//if ticks is a really big integer 
uint32_t ticks_to_msec(uint32_t ticks)
{
	uint32_t temp = ticks/SYSTIME_FREQUENCY;
	return (10*temp + ((ticks - temp*SYSTIME_FREQUENCY)*10)/SYSTIME_FREQUENCY);
}

//It returns (SYSTIME_FREQUENCY*msec)/10, however it's written in such a
//way that there is no integer overflow after the multiplication SYSTIME_FREQUENCY*msec,
//if msec is a really big integer 
uint32_t msec_to_ticks(uint32_t msec)
{
	uint32_t temp = msec/10;
	return (SYSTIME_FREQUENCY*temp + ((msec - temp*10)*SYSTIME_FREQUENCY)/10);
}
