#include "kernel/delay.h"
#include <hal/hal_timer.h>

void delay_ns(uint64_t ns)
{
	uint64_t end = hal_timer_now_ns() + ns;
	while (hal_timer_now_ns() < end)
		;
}