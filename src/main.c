#include "entry.h"
#include "mcu_wrapper.h"

int main(void)
{
    SYSCFG_DL_init();
    
    entry_main();

    rtos_main();

    vTaskStartScheduler();
    
    // unreachable code
    while(1);

    return 0;
}
