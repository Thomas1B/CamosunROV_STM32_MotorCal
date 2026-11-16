/*
 * motors.h
 *
 *  Created on: Jul 19, 2026
 *  Author: Thomas Bourgeois
 *
 *  Motor calibration build: thruster timer/channel mapping only.
 *
 *	Refer to STM32f446xx datasheet for TIM and TIM_CHANNEL.
 *
 */

#ifndef MOTORS_MOTORS_H_
#define MOTORS_MOTORS_H_

#include <stdint.h>
#include "main.h"          /* gives us TIM_HandleTypeDef, TIM_CHANNEL_x */

/*
 * DO NOT CHANGE
 */
extern TIM_HandleTypeDef htim1; /* the real variable lives in main.c */
extern TIM_HandleTypeDef htim8; /* the real variable lives in main.c */

/*
 * Motor -> timer/channel hardware mapping.
 *
 * Maps each of the 6 motors to the STM32 timer instance and channel that
 * generates its PWM signal. Motors 1-4 share TIM1 (channels 4 through 1),
 * motors 5-6 share TIM8 (channels 4 and 3). motor_cal.c uses these directly
 * when calling HAL PWM functions (e.g. HAL_TIM_PWM_Start, __HAL_TIM_SET_COMPARE)
 * so this is the single place to update
 * if a motor is rewired to a different timer/channel.
 */

#define MOTOR1_TIM      &htim1
#define MOTOR1_CHANNEL  TIM_CHANNEL_4

#define MOTOR2_TIM      &htim1
#define MOTOR2_CHANNEL  TIM_CHANNEL_3

#define MOTOR3_TIM      &htim1
#define MOTOR3_CHANNEL  TIM_CHANNEL_2

#define MOTOR4_TIM      &htim1
#define MOTOR4_CHANNEL  TIM_CHANNEL_1

#define MOTOR5_TIM      &htim8
#define MOTOR5_CHANNEL  TIM_CHANNEL_4

#define MOTOR6_TIM      &htim8
#define MOTOR6_CHANNEL  TIM_CHANNEL_3

/*
 * The per-motor onset pulses (MOTORn_REVERSE_ONSET_PULSE / _FORWARD_ONSET_PULSE),
 * the ESC pulse limits and the camera-servo settings live in the main
 * CamosunROV_STM32 project's motors.h. This calibration project only needs
 * the timer/channel mapping above. Keep the mapping the same as the main project.
 */

#endif
