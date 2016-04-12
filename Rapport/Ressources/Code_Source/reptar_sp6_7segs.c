/*
 * reptar_sp6_7segs.c
 *
 *  Created on: Apr 12, 2016
 *      Author: redsuser
 */

#include "reptar_sp6.h"
#include <stdlib.h>

sp6_state_t *sp6_7segs_state;

uint8_t reptar_sp6_7segs_read(int segNb)
    {
    return (uint8_t) sp6_7segs_state->regs[segNb >> 1];
    }

void reptar_sp6_7segs_write(uint8_t value, int segNb)
    {
    cJSON *root;
    DBG("7seg digit %d new value = %d\n", ((segNb >> 1) - 0x17), value);

    sp6_7segs_state->regs[segNb >> 1] = (uint16_t) value;

    root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "perif", PERID_SEVEN_SEG);
    cJSON_AddNumberToObject(root, "digit", ((segNb >> 1) - 0x17));
    cJSON_AddNumberToObject(root, "value", sp6_7segs_state->regs[segNb >> 1]);

    DBG("%s\n", cJSON_Print(root));

    /*cJSON_AddStringToObject(root, "perif", "7seg");
     cJSON_AddNumberToObject(root, "digit", 1);
     cJSON_AddNumberToObject(root, "value", 5);*/

    sp6_emul_cmd_post(root);
    }

int reptar_sp6_7segs_init(void *opaque)
    {
    sp6_7segs_state = (sp6_state_t*) opaque;

    // Init de la valeur des leds
    sp6_7segs_state->regs[SP6_7SEG1 >> 1] = 0x0000;
    sp6_7segs_state->regs[SP6_7SEG2 >> 1] = 0x0000;
    sp6_7segs_state->regs[SP6_7SEG3 >> 1] = 0x0000;

    DBG("7Seg initialized!\n");
    return 0;
    }
