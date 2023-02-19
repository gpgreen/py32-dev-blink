#include "debounce.h"

void debounce_init(debounce_pin_t* db, int pull_up_on)
{
    if (pull_up_on) {
        db->buffer = 0xFF;
    }
    else
        db->buffer = 0;
}

void debounce(debounce_pin_t* db)
{
    db->buffer <<= 1;
    db->buffer |= HAL_GPIO_ReadPin(db->bank, db->pin);
}

int debounce_is_high(const debounce_pin_t* db)
{
    return db->buffer == 0xFF;
}

int debounce_is_low(const debounce_pin_t* db)
{
    return db->buffer == 0;
}
