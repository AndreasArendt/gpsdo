/*
 * led.c
 *
 *  Created on: Oct 28, 2025
 *      Author: andia
 */

#include "led.h"
#include "hal.h"

#define LED_RED_PIN  GPIO_PIN_13
#define LED_RED_PORT GPIOC

#define LED_ORANGE_PIN GPIO_PIN_14
#define LED_ORANGE_PORT GPIOC

void toggle_led_red()
{
	HAL_GPIO_TogglePin(LED_RED_PORT, LED_RED_PIN);
}

void set_led_red()
{
	HAL_GPIO_WritePin(LED_RED_PORT, LED_RED_PIN, GPIO_PIN_RESET);
}

void clear_led_red()
{
	HAL_GPIO_SetPin(LED_RED_PORT, LED_RED_PIN, GPIO_PIN_SET);
}


void toggle_led_orange()
{
	HAL_GPIO_TogglePin(LED_ORANGE_PORT, LED_ORANGE_PIN);
}

void set_led_orange()
{
	HAL_GPIO_WritePin(LED_ORANGE_PORT, LED_ORANGE_PIN, GPIO_PIN_RESET);
}

void clear_led_orange()
{
	HAL_GPIO_SetPin(LED_ORANGE_PORT, LED_ORANGE_PIN, GPIO_PIN_SET);
}
