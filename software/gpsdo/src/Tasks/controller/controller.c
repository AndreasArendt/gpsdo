#include "controller.h"
#include "cmsis_os.h"

void controllerTask(void *argument)
{
	/* Infinite loop */
	for (;;) {
		osDelay(1);
	}
}
