/*
   LILLA Audio Sampler
   Author: Sandro Grassia (info@lillasampler.it)
   www.lillasampler.it
*/

#pragma once

#include "DisplayControlChange.h"
#include "SharedControlChange.h"

class PointerControlChange
{
private:
    DisplayControlChange &Display;
    CC_field_description_struct pointer = {field_CC_Return, 0};

public:
    PointerControlChange(DisplayControlChange &display) : Display(display) {}

    FLASHMEM void Set_pointer_to_return(void);
    FLASHMEM bool Move_pointer(int value); // Direction is -1 or 1; stop at the first/last field.
    FLASHMEM void Display_pointer(void);
    CC_field_description_struct Get_pointer(void) const;
};
