/*
 * motor_cal.c
 *
 * Keyboard calibration console for the thruster ESC onset pulses.
 * See motor_cal.h. Terminal: USART2 (Nucleo ST-LINK virtual COM port),
 * 115200 baud, 8N1, no flow control.
 *
 * Keys:
 *   1..6     select that thruster (every output goes back to 1500 us first)
 *   ->       +1 us (right arrow)
 *   <-       -1 us (left arrow)
 *   space    selected thruster back to 1500 us (neutral)
 *   0        all thrusters to 1500 us, none selected
 *   ?        help
 *
 * The arrow keys are the only way to change the pulse, 1 us per press.
 * A terminal sends an arrow key as a 3-byte escape sequence:
 * ESC '[' 'C' (right) / ESC '[' 'D' (left), or ESC 'O' 'C' / ESC 'O' 'D'
 * in "application cursor" mode. parse_arrow() decodes both forms.
 *
 * The timers are set to a 1 us tick (TIM1/TIM8: PSC = 180-1 at 180 MHz),
 * so the compare value written here IS the pulse width in microseconds.
 */

#include "motor_cal.h"

#include <stdio.h>
#include <string.h>
#include "main.h"
#include "motors.h"

extern UART_HandleTypeDef huart2; /* the real variable lives in main.c */

#define CAL_UART      (&huart2)
#define NEUTRAL_US    1500U

typedef struct {
	TIM_HandleTypeDef *tim;
	uint32_t channel;
} MotorOut;

static const MotorOut outs[6] = {
	{ MOTOR1_TIM, MOTOR1_CHANNEL },
	{ MOTOR2_TIM, MOTOR2_CHANNEL },
	{ MOTOR3_TIM, MOTOR3_CHANNEL },
	{ MOTOR4_TIM, MOTOR4_CHANNEL },
	{ MOTOR5_TIM, MOTOR5_CHANNEL },
	{ MOTOR6_TIM, MOTOR6_CHANNEL },
};

static uint8_t  motor = 0;          /* 0 = none selected, 1..6 */
static uint32_t pulse = NEUTRAL_US;
static uint32_t lastKeyTick = 0;

static void put(const char *s) {
	HAL_UART_Transmit(CAL_UART, (const uint8_t*) s, (uint16_t) strlen(s), 50);
}

static void all_neutral(void) {
	for (int i = 0; i < 6; i++) {
		__HAL_TIM_SET_COMPARE(outs[i].tim, outs[i].channel, NEUTRAL_US);
	}
	pulse = NEUTRAL_US;
}

static void show(void) {
	char line[64];
	if (motor == 0) {
		snprintf(line, sizeof line, "all neutral (1500 us). Press 1..6.\r\n");
	} else {
		snprintf(line, sizeof line, "M%u  %lu us  (%+ld)\r\n", motor,
				(unsigned long) pulse, (long) pulse - (long) NEUTRAL_US);
	}
	put(line);
}

static void help(void) {
	put("\r\n--- ESC onset calibration ---\r\n"
		"1..6 select thruster   right / left arrow  +1 / -1 us\r\n"
		"space = neutral        0 = all stop        ?  = help\r\n");
	show();
}

/* Non-blocking read of one byte from USART2. Returns -1 if none waiting. */
static int get_key(void) {
	USART_TypeDef *u = CAL_UART->Instance;
	uint32_t sr = u->SR;
	if (sr & (USART_SR_RXNE | USART_SR_ORE)) {
		return (int) (u->DR & 0xFFU); /* reading SR then DR also clears ORE */
	}
	return -1;
}

/*
 * Arrow-key decoder. Feed it every received byte.
 * Returns +1 (right arrow), -1 (left arrow), 0 if the byte was part of an
 * escape sequence (or an arrow we don't use, e.g. up/down), or 2 if the byte
 * is an ordinary key the caller should handle itself.
 */
static uint8_t escState = 0; /* 0 = idle, 1 = got ESC, 2 = got ESC + '[' or 'O' */

static int parse_arrow(int c) {
	switch (escState) {
	case 1:
		if (c == '[' || c == 'O') {
			escState = 2;
			return 0;
		}
		escState = 0; /* lone ESC: treat this byte as a normal key */
		break;
	case 2:
		escState = 0;
		if (c == 'C') return 1;  /* right */
		if (c == 'D') return -1; /* left  */
		return 0;                /* up/down or anything else: ignore */
	default:
		break;
	}
	if (c == 0x1B) {
		escState = 1;
		return 0;
	}
	return 2;
}

void motor_cal_init(void) {
	/* Load neutral BEFORE starting the timers, so the very first pulse
	 * the ESCs see is 1500 us and they arm normally. HAL_TIM_PWM_Start()
	 * also sets MOE on TIM1/TIM8 (advanced timers). */
	all_neutral();
	for (int i = 0; i < 6; i++) {
		HAL_TIM_PWM_Start(outs[i].tim, outs[i].channel);
	}
	motor = 0;
	lastKeyTick = HAL_GetTick();
	help();
}

void motor_cal_service(void) {
	int c = get_key();
	if (c < 0) {
		/* Safety: nobody touching the keyboard -> stop. */
		if ((motor != 0 || pulse != NEUTRAL_US)
				&& (HAL_GetTick() - lastKeyTick) > CAL_IDLE_TIMEOUT_MS) {
			all_neutral();
			motor = 0;
			put("idle timeout -> all neutral\r\n");
		}
		return;
	}
	lastKeyTick = HAL_GetTick();

	int32_t step = 0;
	int a = parse_arrow(c);
	if (a == 0) {
		return;           /* middle of an escape sequence, or an unused arrow */
	} else if (a != 2) {
		step = a;         /* right/left arrow: +1 / -1 us */
	} else switch (c) {
	case '1': case '2': case '3': case '4': case '5': case '6':
		all_neutral();
		motor = (uint8_t) (c - '0');
		break;
	case '0':
		all_neutral();
		motor = 0;
		break;
	case ' ':
		pulse = NEUTRAL_US;
		break;
	case '?': case 'h': help(); return;
	default: return; /* ignore anything else (CR, LF, ...) */
	}

	if (step != 0) {
		if (motor == 0) {
			put("select a thruster first (1..6)\r\n");
			return;
		}
		int32_t p = (int32_t) pulse + step;
		if (p < (int32_t) CAL_PULSE_MIN_US) p = CAL_PULSE_MIN_US;
		if (p > (int32_t) CAL_PULSE_MAX_US) p = CAL_PULSE_MAX_US;
		pulse = (uint32_t) p;
	}

	if (motor != 0) {
		__HAL_TIM_SET_COMPARE(outs[motor - 1].tim, outs[motor - 1].channel, pulse);
	}
	show();
}
