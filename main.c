/*
 * Created: 12/20/2021
 * Author : Kirk Roerig
 */ 
//#define F_CPU 8000000
#define F_CPU 1000000


#define SEC_PER_HALF_DAY (12 * 3600)
#define STEPS_PER_REV (200)
//#define SEC_TO_WAIT (SEC_PER_HALF_DAY / STEPS_PER_REV)
#define SEC_TO_WAIT 216 


#include <avr/io.h>
#include <util/delay.h>
#include <math.h>

typedef enum {
	OT_PORTA,
	OT_PORTB,
} port_t;

typedef struct {
	port_t port;
	int num;
} pin_t;

typedef struct {
	pin_t coil_pins[2][2];
	pin_t coil_pin_cfg[2][2];
} stepper_t;


#define gpio_set(port, pin, state) { \
	if (state) (port) |=  (1 << (pin)); \
	else       (port) &= ~(1 << (pin)); \
} \


void stepper_disable(stepper_t* m)
{
	for (int i = 0; i < 4; ++i)
	{
		for (int j = 0; j < 2; ++j)
		{
			pin_t pin = m->coil_pin_cfg[i % 2][j];

			switch (pin.port)
			{
				case OT_PORTA:
					gpio_set(PORTA, pin.num, 0);
					break;
				case OT_PORTB:
					gpio_set(PORTB, pin.num, 0);
					break;
			}
		}
	}
}


void stepper_dir(stepper_t* m, int dir)
{
	if (dir < 0)
	{
		m->coil_pin_cfg[0][0] = m->coil_pins[1][0];
		m->coil_pin_cfg[0][1] = m->coil_pins[1][1];
		m->coil_pin_cfg[1][0] = m->coil_pins[0][0];
		m->coil_pin_cfg[1][1] = m->coil_pins[0][1];
	}
	else
	{
		m->coil_pin_cfg[0][0] = m->coil_pins[0][0];
		m->coil_pin_cfg[0][1] = m->coil_pins[0][1];
		m->coil_pin_cfg[1][0] = m->coil_pins[1][0];
		m->coil_pin_cfg[1][1] = m->coil_pins[1][1];
	}
}


int stepper_step(const stepper_t* m)
{
	int elapsed_ms = 0;
	int signals[4][2] = {
		{1, 0},
		{0, 1},
		{0, 1},
		{1, 0},
	};

	for (int i = 0; i < 4; ++i)
	{
		for (int j = 0; j < 2; ++j)
		{
			pin_t pin = m->coil_pin_cfg[i % 2][j];

			switch (pin.port)
			{
				case OT_PORTA:
					gpio_set(PORTA, pin.num, signals[i][j]);
					break;
				case OT_PORTB:
					gpio_set(PORTB, pin.num, signals[i][j]);
					break;
			}
		}

		_delay_ms(3);
		elapsed_ms += 3;
	}

	return elapsed_ms;
}

volatile int8_t step_deltas[2] = {};

stepper_t hand_stepper = {
	.coil_pins = {
		{ { OT_PORTA, 3 }, { OT_PORTA, 2 } },
		{ { OT_PORTA, 1 }, { OT_PORTA, 0 } },
	},
};

stepper_t pitch_stepper = {
	.coil_pins = {
		{ { OT_PORTB, 0 }, { OT_PORTB, 1 } },
		{ { OT_PORTB, 2 }, { OT_PORTA, 7 } },
	},
};


int main(void)
{	
	stepper_t* steppers[2] = { &hand_stepper, &pitch_stepper };


	// Setup pin directionality
	for (int si = 0; si < sizeof(steppers) / sizeof(stepper_t*); ++si)
	{
		for (int ci = 2; ci--;)
		for (int pi = 2; pi--;)
		{
			pin_t pin = steppers[si]->coil_pins[ci][pi];
			if (OT_PORTA == pin.port) { DDRA |= (1 << pin.num); }
			if (OT_PORTB == pin.port) { DDRB |= (1 << pin.num); }
		}
	}

	// main loop
	for (unsigned int t = 0; 1;)
	{
		step_deltas[0] = 1;

		long elapsed_ms = 0;

		for (int i = sizeof(steppers) / sizeof(stepper_t*); i--;)
		{
			if (step_deltas[i])
			{
				stepper_dir(steppers[i], step_deltas[i]);
				elapsed_ms += stepper_step(steppers[i]);
			}
			
			if (step_deltas[i] > 0) { step_deltas[i]--; }
			else if (step_deltas[i] < 0) { step_deltas[i]++; }
		}
		unsigned long wait_time_ms = (SEC_TO_WAIT * 1000) - elapsed_ms;
		
		for (; wait_time_ms--;)
		{
			_delay_ms(1);
		}
	}

	PORTA = 0;
	PORTB = 0;

	return 0;
}
