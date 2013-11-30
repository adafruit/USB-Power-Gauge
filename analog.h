#ifndef ANALOG_H_
#define ANALOG_H_

uint16_t getVCC(uint16_t vbgAdc, uint16_t vbgActual);
uint16_t getCal5v(uint16_t vbgAdc);
uint16_t readVBG(void);
uint16_t readVCC(void);

uint16_t readCurrent(void);

#endif /* ANALOG_H_ */