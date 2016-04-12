#ifndef REPTAR_SP6_LEDS_H
#define REPTAR_SP6_LEDS_H

#include "cJSON.h"

typedef struct LEDState {

  uint8_t state;          

} LEDState_t;

uint8_t reptar_sp6_leds_read(void);
void reptar_sp6_leds_write(uint8_t value);
int reptar_sp6_leds_init(void *opaque);


#endif 
