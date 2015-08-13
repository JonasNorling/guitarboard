#include <libopencm3/cm3/itm.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <unistd.h>

#include "platform.h"
#include "wm8731.h"


void platformInit(void)
{
    rcc_clock_setup_hse_3v3(&hse_8mhz_3v3[CLOCK_3V3_168MHZ]);
    rcc_periph_clock_enable(RCC_GPIOC);

    // Enable LED pins and turn them on
    gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO3 | GPIO7);
    gpio_set(GPIOC, GPIO3 | GPIO7);

    codecInit();
}

void platformMainloop(void)
{
    while (true);
}

ssize_t _write(int fd __attribute__((unused)), const void* buf, size_t count)
{
    if (!(ITM_TER[0] & 1)) {
        // Tracing not enabled
        return count;
    }

    const char* charbuf = buf;
    for (size_t i = 0; i < count; i++) {
        while (!(ITM_STIM8(0) & ITM_STIM_FIFOREADY))
            ;
        ITM_STIM8(0) = *charbuf++;
    }
    return count;
}
