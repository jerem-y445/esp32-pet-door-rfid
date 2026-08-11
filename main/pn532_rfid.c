#include "inc/main.h"

uint32_t find_uid_value(uint8_t arr[], uint8_t length) 
{
    uint32_t concatValue = arr[0];

    for (int i = 0; i < length - 1; ++i) 
    {
        concatValue = (concatValue << 8);
        concatValue += arr[i + 1];
    }

    return concatValue;
}