// (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by
#include "defines.h"
#include "Device/Device.h"


#ifndef WIN32
    #if __ARMCC_VERSION != 6210000
        // На других версиях компиляторов не проверялось
        // На 6.23 из Keil 5.42a скорее всего будут проблемы из-за new, malloc
        #error "Requires ARM Compiler V6.21 from uVision 5.39"
    #endif
#endif


/*

    1. После включения около-красный цвет.
    2. Когда читается ID карты - проигрываем первую мелодию и сценарий светодиодов.
    3. После того, как светодиоды поиграли 1 минуту, снова включаем околокрасный.

*/


int main(void)
{
    Device::Init();

    while (true)
    {
        Device::Update();
    }
}
