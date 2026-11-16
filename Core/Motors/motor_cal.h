/*
 * motor_cal.h
 *
 * ESC onset-pulse calibration console (keyboard over USART2, 115200 8N1).
 *
 * Standalone calibration firmware: this project does nothing else. Flash it,
 * open a serial terminal on the Nucleo's ST-LINK virtual COM port, and drive
 * one thruster at a time with the keyboard to find where each ESC starts
 * turning the motor.
 *
 * The values found here go into MOTORn_REVERSE_ONSET_PULSE /
 * MOTORn_FORWARD_ONSET_PULSE in the main CamosunROV_STM32 project's motors.h.
 * Nothing in this project needs to change once they're found.
 */

#ifndef MOTORS_MOTOR_CAL_H_
#define MOTORS_MOTOR_CAL_H_

/* Pulse limits for the calibration console, in microseconds. */
#define CAL_PULSE_MIN_US     1350U
#define CAL_PULSE_MAX_US     1650U

/* No key pressed for this long -> all thrusters back to neutral, in ms. */
#define CAL_IDLE_TIMEOUT_MS  120000U

/* Call once after MX_TIM1_Init() / MX_TIM8_Init() / MX_USART2_UART_Init().
 * Sets all six outputs to 1500 us (neutral), starts PWM, prints the help. */
void motor_cal_init(void);

/* Call once per main-loop pass. Non-blocking. */
void motor_cal_service(void);

#endif /* MOTORS_MOTOR_CAL_H_ */
