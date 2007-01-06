/**
 * @brief    header file for gpio debugging
 * @author   Naim Busek
 */


#ifndef _DBG_H
#define _DBG_H

enum {
	DBG_BIT0_SET=1,
	DBG_BIT1_SET,
	DBG_BIT2_SET,
	DBG_BIT3_SET,
	DBG_BIT0_CLR,
	DBG_BIT1_CLR,
	DBG_BIT2_CLR,
	DBG_BIT3_CLR,
	DBG_BIT0_TOGGLE,
	DBG_BIT1_TOGGLE,
	DBG_BIT2_TOGGLE,
	DBG_BIT3_TOGGLE,
};



/**
 * @brief dbg functions
 */
#ifndef NO_DBG
#define dbg_bit0_set()      PORTF |= (1<<PF7)
#define dbg_bit1_set()      PORTF |= (1<<PF6)
#define dbg_bit2_set()      PORTF |= (1<<PF5)
#define dbg_bit3_set()      PORTF |= (1<<PF4)
#define dbg_bit0_clr()      PORTF &= ~(1<<PF7)
#define dbg_bit1_clr()      PORTF &= ~(1<<PF6)
#define dbg_bit2_clr()      PORTF &= ~(1<<PF5)
#define dbg_bit3_clr()      PORTF &= ~(1<<PF4)
#define dbg_bit0_toggle()   PORTF ^= (1<<PF7)
#define dbg_bit1_toggle()   PORTF ^= (1<<PF6)
#define dbg_bit2_toggle()   PORTF ^= (1<<PF5)
#define dbg_bit3_toggle()   PORTF ^= (1<<PF4)
#define dbg_init()     {  DDRF |= 0xf0; dbg_bit1_clr(); dbg_bit0_clr(); dbg_bit2_clr(); dbg_bit3_clr(); }
#else 
#define dbg_bit0_set()        
#define dbg_bit1_set()     
#define dbg_bit2_set()   
#define dbg_bit3_set()   
#define dbg_bit0_clr()       
#define dbg_bit1_clr()     
#define dbg_bit2_clr()    
#define dbg_bit3_clr()    
#define dbg_bit0_toggle()    
#define dbg_bit1_toggle()  
#define dbg_bit2_toggle() 
#define dbg_bit3_toggle() 
#define dbg_init() 
#endif

/**
 * kernel writer can just use macros provided by SOS
 *
 * dbg_bit0_set()        
 * dbg_bit1_set()    
 * dbg_bit2_set()   
 * dbg_bit0_clr()      
 * dbg_bit1_clr()     
 * dbg_bit2_clr()    
 * dbg_bit0_toggle() 
 * dbg_bit1_toggle() 
 * dbg_bit2_toggle()
 */
#ifndef _MODULE_
#include <sos.h>
extern int8_t ker_dbg(uint8_t action);
#endif /* _MODULE_ */
#endif

