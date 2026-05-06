#include "cst816.h"

// Touch is now initialised inside lcd_lvgl_Init() via ESP_PanelTouch_CST816S.
// These stubs keep the .ino's existing Touch_Init() call from breaking.

void Touch_Init(void) {}

uint8_t getTouch(uint16_t *x, uint16_t *y)
{
    return 0;
}
