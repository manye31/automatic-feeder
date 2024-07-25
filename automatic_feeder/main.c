/*
 * lab6_project.c
 *
 * Created: 6/29/2024 1:55:43 PM
 * Author : Micah Nye
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>

#define CAT 0x00
#define DOG 0x01

enum AugerMode {
    OFF,
    SLOW,
    FAST,
    JIGGLE
};

void readFoodLevels(void);
void fillFoodBowls(void);
void fillWaterBowl(void);
void augerCommand(enum AugerMode mode);

void step_cw(void);
void step_ccw(void);
void rotate(int deg, char dir, float stride, float rot_t);

void wait(volatile int msec);

char food_bowl_level = 0; // food bowl level (TEMP from potentiometer)
char food_bowl_thresh = 127; // threshold when to fill bowl again

char food_supply_level = 255; // food supply level (TEMP manually set for demo)
char food_supply_thresh = 25; // threshold when to signal low supply
char food_stuck = 0; // Assume food is not stuck

char friction_offset = 2; // Experimental offset for motor, min required speed
                           // to overcome friction torque in the motor

char food_size_mode = 0; // 0 - Cat food, 1 - dog food

char begin_feed = 0; // feed flag
char begin_water = 0; // water flag

enum AugerMode AUGER_MODE;

int main(void)
{
    // Registers

    DDRD = 0b11000010; // PD1 - motor dir output M1
                       // PD2 - Manual mode INT input, also food stuck
                       // PD4 - Cat/dog toggle simulator input
                       // PD5 - Animal toggle (WIP)
                       // PD6 - PWM output
                       // PD7 - motor dir output M2

    DDRB = 0b00110000; // PB4-5 LED1/2 (R/G)

    PORTB = 0b00110000; // Default LEDs to OFF

    /* ADC Setup */
    // ADC Multiplexer
    ADMUX = 0b00100000;  // (6-7) REF set to 00 (AREF)
                         // (5) ADLAR to 1 (Left-just)
                         // (3-0) MUX to 0000 (PC0)
    
    // Power reduction register
    PRR = 0x00;          // Power ADC on

    // ADC Control and Status Register A
    ADCSRA = 0b10000011; // (7) Enable ADC (ADEN)
                         // (6) ADC Start conversion - 0 (not started yet)
                         // (5) Auto Trigger Enable - 0 disabled
                         // (4) ADC INT Flag - 0 
                         // (3) ADC Interrupt Enable flag - 0 (INT disabled)
                         // (2-0) Prescaler of 8. 1MHz/8 = 125KHz.
                         //       125 KHz/13 ~= 10kHz

    /* PWM @ D6 (TC0 -- 8bit) */
    // Timer counter control register A for Timer 0
    TCCR0A = 0b10000011; // (7-6) Non-inverting mode for A0
                         // (5-4) Disconnect OC0B
                         // (1-0) Fast PWM mode (X11)
    
    // Timer counter control register B for Timer 0
    TCCR0B = 0b00000001; // (7-6) No compare flags (OCF0A/B)
                         // (3)   Fast PWM mode (0XX)
                         // (2-0) Default clock source to 1 prescaler
                         //       F_PWM = (1MHz)/(256*1) = 4kHz ~ 1kHz
    
    // Output compare register preset
	OCR0A = 0x00;        // Preset duty cycle to 0

    // INTERRUPTS
    EICRA = 0b00000010; // Falling edge for INT0. Switch configured
                        // as active high, so falling edge is at press-in
	EIMSK = 0b00000001; // Enable INT0
	sei(); // Enable global interrupts

    while (1) 
    {
        // Assuming "timer loop" already finished
		// Read food sensors
		readFoodLevels();

        // Check underweight status of food bowl
        if (food_supply_level < food_supply_thresh) {
            PORTB &= 0b11101111; // Turn LED1 (red) ON
            PORTB |= 0b00100000; // Turn LED2 (green) OFF
        } else {
            PORTB &= 0b11011111; // Turn LED2 (green) ON
            PORTB |= 0b00010000; // Turn LED1 (red) OFF 
        }

        // Read food size mode input
        if (PIND & 0b00010000) {
            // Toggle food size mode
            food_size_mode = !food_size_mode;
        }

        // Set auger mode
        if (food_size_mode == CAT) {
            AUGER_MODE = SLOW;
        } else if (food_size_mode == DOG) {
            AUGER_MODE = FAST;
        }

        if (begin_feed == 1) {
            fillFoodBowls();
        }

        if (begin_water == 1) {
            fillWaterBowl();
        }

        // Loop reset
        begin_feed = 0;
        begin_water = 0;
        food_stuck = 0;
    }   
}

/* Manual Override Button */
ISR(INT0_vect) {
    // Manual override button pressed
    begin_feed = 1;
    EIFR = 0b00000001;
}

void fillFoodBowls(void) {
    // Single animal logic, for now
    int pet_num = 1;

    while (pet_num > 0) {
        while (food_bowl_level < food_bowl_thresh) {
            // Read food level
            readFoodLevels();
			            
            // If auger set to move and stuck, jiggle
            if (AUGER_MODE > OFF && food_stuck) {
                augerCommand(JIGGLE); // Will retain old auger mode
            } else {
                augerCommand(AUGER_MODE);
            }
        }
        augerCommand(OFF);
        // OFF Signal
        PORTB &= 0b11101111; // Turn LED1 (red) ON
        wait(200);
        PORTB |= 0b00010000; // Turn LED1 (red) OFF
        wait(200);
        PORTB &= 0b11101111; // Turn LED1 (red) ON
        wait(200);
        PORTB |= 0b00010000; // Turn LED1 (red) OFF

        // First pet feed, load in next, if applicable
        pet_num--;
        food_stuck = 0;

        if (pet_num > 0) {
            // flip chute gate
        } // else, exit while loop
    }
}

void fillWaterBowl(void) {
    // do stuff
}

void augerCommand(enum AugerMode mode) {
    switch (mode) {
        case OFF:
        OCR0A = 0x00; // Stop motor
        PORTD &= 0b01111101; // Clear M1 and M2 in H-bridge
		break;
        case SLOW:
        OCR0A = 175; // Rotate motor slowly
        PORTD &= 0b11111101; // Clear M1
        PORTD |= 0b10000000; // Set M2
        break;
        case FAST:
        OCR0A = 255; // Rotate motor quickly
        PORTD &= 0b11111101; // Clear M1
        PORTD |= 0b10000000; // Set M2
        break;
        case JIGGLE:
        OCR0A = 200;
        PORTD &= 0b11111101; // Clear M1
        PORTD |= 0b10000000; // Set M2
        wait(300);
        PORTD &= 0b01111111; // Clear M1
        PORTD |= 0b00000001; // Set M2
        wait(300);
        PORTD &= 0b11111101; // Clear M1
        PORTD |= 0b10000000; // Set M2
        wait(300);
        PORTD &= 0b01111111; // Clear M1
        PORTD |= 0b00000001; // Set M2
        wait(300);
        break;
    }
}

void readFoodLevels(void) {
    // Read ADC
    // Start ADC conversion
    ADCSRA |= 0b01000000; // Set ADSC bit (start conversion)

    // Wait for conversion to finish
    // while (ADCSRA & (1 << ADSC)); // Polling ADSC until cleared (conversion finished)
    //                               // Not using INT because INT1 needed for other functionality
    while ((ADCSRA & 0b00010000) == 0) // While ADIF bit not set
                                        // (INT flag when conversion complete)
    food_bowl_level = ADCH; // Read in simulated "food level" from potentiometer
    food_supply_level = food_supply_level; // placeholder for when reading in data via ADC
}

void wait(volatile int msec) {
    /**
     * @brief wait for a number of milliseconds, assuming a 1MHz clock-cycle
     * 
     */

    char count_limit = 125;


    while (msec > 0) {
        // Put timer into normal mode
		// Only using TC1 temporarily for this simulation. In reality, TC1 will be used for clock time approx.
        TCCR1A = 0x00; // Set bits 0-1 (WGM00, WGM01) to low
        TCNT1 = 0x00; // Preload timer to 0

        // Set timer mode (Set CS00, CS01, CS02)
        // Set prescaler of 8 because 1MHz / (8 * 125) = 1KHz; T = 1ms
        TCCR1B = 0b00000010; 

        while (TCNT1 < count_limit);

        // Stop TIMER1
        TCCR1B = 0x00;
        msec--;
    }
}
