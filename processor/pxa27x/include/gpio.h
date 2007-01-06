#ifndef GPIO_H
#define GPIO_H

int PXA27XGPIOInt_init();
int PXA27XGPIOInt_start();
int PXA27XGPIOInt_stop();
void PXA27XGPIOInt_enable(uint8_t pin, uint8_t mode, void(*callback)(void));
void PXA27XGPIOInt_disable(uint8_t pin);
void PXA27XGPIOInt_clear(uint8_t pin);


#endif
