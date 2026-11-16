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
Open a serial terminal on the ST-LINK virtual COM port at 115200 baud, 8N1, no flow control. Use PuTTY, Tera Term or `screen`: the terminal must send each key as it's pressed, and send the arrow keys as escape codes. The STM32CubeIDE console doesn't, so the arrows won't work there.

| Key | Action |
|:---:|---|
| `1`–`6` | Select that thruster (every output goes back to 1500 µs first) |
| → (right arrow) | +1 µs on the selected thruster |
| ← (left arrow) | −1 µs on the selected thruster |
| space | Selected thruster back to 1500 µs |
| `0` | All thrusters to 1500 µs, none selected |
| `?` / `h` | Help |

The arrow keys are the only way to change the pulse, 1 µs per press. Holding one down repeats at the keyboard's repeat rate. Every other key (including ↑ / ↓, Enter and the old `+ - [ ]` keys) is ignored.

After each key the console prints the current state, e.g. `M3  1527 us  (+27)`, or `all neutral (1500 us). Press 1..6.` when no thruster is selected. Pressing an arrow with no thruster selected prints `select a thruster first (1..6)`.

The timers count in 1 µs steps (TIM1/TIM8: prescaler 180−1 at 180 MHz), so the value shown is the pulse width written to the timer.

### Safety
- All six outputs are set to 1500 µs **before** PWM starts at boot, so the ESCs arm normally.
- The pulse is clamped to 1350–1650 µs (`CAL_PULSE_MIN_US` / `CAL_PULSE_MAX_US` in `motor_cal.h`).
- No key for 120 s → every output back to 1500 µs and `idle timeout -> all neutral` is printed (`CAL_IDLE_TIMEOUT_MS`).
- If the main loop hangs for ~0.5 s, the watchdog resets the chip. The PWM pins stop driving, then the board reboots to neutral.

### How the arrow keys are read
A terminal sends an arrow key as three bytes: `ESC [ C` (right) or `ESC [ D` (left), or `ESC O C` / `ESC O D` in "application cursor" mode. `parse_arrow()` in `motor_cal.c` decodes both forms. `get_key()` polls USART2 for one byte per main-loop pass (no interrupts, never blocks).

## Files
| File | |
|---|---|
| `Core/Src/main.c` | Clock, TIM1/TIM8/USART2/IWDG init, main loop |
| `Core/Motors/motor_cal.c/.h` | The keyboard console: `motor_cal_init()` (neutral + PWM start + help), `motor_cal_service()` (called every main-loop pass), `get_key()`, `parse_arrow()` |
| `Core/Motors/motors.h` | Motor → timer/channel mapping only (keep it the same as the main project) |

Do not flash this onto the ROV for normal use: it ignores the Pi completely.