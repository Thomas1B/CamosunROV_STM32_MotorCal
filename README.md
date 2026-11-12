# CamosunROV STM32 — Motor (ESC) Calibration

Stripped-down copy of the CamosunROV STM32 firmware that does **one thing**: drive the six thruster PWM outputs from a keyboard so you can find each ESC's deadband (onset) pulses.

Everything else (Pi UART link, I2C sensors, ADC, camera servo, leak break inputs, status LEDs, SWO debug print) has been removed. The results go into the **main** project's `Core/Motors/motors.h` (`MOTORn_REVERSE_ONSET_PULSE` / `MOTORn_FORWARD_ONSET_PULSE`).

Target: NUCLEO-F446RE (STM32F446RE), STM32CubeIDE 2.2.0 / STM32CubeMX 6.18.1, FW_F4 V1.28.3.

## What's left
| Peripheral | Use |
|---|---|
| TIM1 CH1–CH4 | Motors 4, 3, 2, 1 PWM (~300 Hz, 1 µs per tick) |
| TIM8 CH3–CH4 | Motors 6, 5 PWM (~300 Hz, 1 µs per tick) |
| USART2 (PA2/PA3) | Keyboard console, 115200 8N1 — the Nucleo's ST-LINK virtual COM port, so just the USB cable |
| IWDG | ~0.5 s watchdog: if the firmware hangs, the chip resets and the PWM stops |
| SWD (PA13/PA14), SWO (PB3) | Programming / debug |

## Pins
| STM32 Pin | Function |
|:---:|---|
| PA11 | Motor 1 PWM (TIM1_CH4) |
| PA10 | Motor 2 PWM (TIM1_CH3) |
| PA9  | Motor 3 PWM (TIM1_CH2) |
| PA8  | Motor 4 PWM (TIM1_CH1) |
| PC9  | Motor 5 PWM (TIM8_CH4) |
| PC8  | Motor 6 PWM (TIM8_CH3) |
| PA2 / PA3 | USART2 TX / RX (ST-LINK VCP, already wired on the Nucleo) |

All other pins are left unconfigured (reset state: floating input), so anything still wired to them on the breadboard is simply not driven.

## Console keys
| Key | Action |
|:---:|---|
| `1`–`6` | Select that thruster (every output goes back to 1500 µs first) |
| `+` / `=` | +1 µs |
| `-` / `_` | −1 µs |
| `]` | +10 µs |
| `[` | −10 µs |
| space | Selected thruster back to 1500 µs |
| `0` | All thrusters to 1500 µs, none selected |
| `?` / `h` | Help |

Limits: pulse is clamped to 1350–1650 µs (`CAL_PULSE_MIN_US` / `CAL_PULSE_MAX_US` in `motor_cal.h`). No key for 2 minutes → everything back to neutral (`CAL_IDLE_TIMEOUT_MS`).

## Files
| File | |
|---|---|
| `Core/Src/main.c` | Clock, TIM1/TIM8/USART2/IWDG init, main loop |
| `Core/Motors/motor_cal.c/.h` | The keyboard console |
| `Core/Motors/motors.h` | Motor → timer/channel mapping only (keep it the same as the main project) |

Do not flash this onto the ROV for normal use: it ignores the Pi completely.
