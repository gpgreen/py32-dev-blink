#include "debounce.h"

void debounce_init(debounce_pin_t* db, int pull_up_on)
{
    if (pull_up_on) {
        db->buffer = 0xFFFF;
    }
    else
        db->buffer = 0;
}

inline
void debounce(debounce_pin_t* db)
{
    db->buffer <<= 1;
    if (HAL_GPIO_ReadPin(db->bank, db->pin)) {
        db->buffer |= 1;
    } else {
        db->buffer |= 0;
    }
}

inline
int debounce_is_high(const debounce_pin_t* db)
{
    return (db->buffer == 0xFFFF);
}

inline
int debounce_is_low(const debounce_pin_t* db)
{
    return (db->buffer == 0);
}
