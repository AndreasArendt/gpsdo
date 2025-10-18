#include "usb.h"
#include "hal.h"

char msg[] = "Hello from STM32 via USB CDC!\r\n";

void usbTask(void *argument) {
	/* USER CODE BEGIN usbTask */
	/* Infinite loop */
	for (;;) {
		CDC_Transmit_FS((uint8_t*) msg, strlen(msg));
		osDelay(1000);
	}
	/* USER CODE END usbTask */
}
