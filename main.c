/*
 * main.c
 *
 *  Created on: Jun 17, 2013
 *      Author: mes
 */

#include <avr/io.h>
#include "Pololu3pi.h"

////////////////////////////////////////////////////////////////////////
// Configuration
#define SPEED 60
#define MAXSTEER 30		// Maximum change in motor speed for steering
#define DT 10			// time step in ms, at least 2ms to be safe

// Data for generating the characters used in load_custom_characters
// and bargraph_sensors.  By reading levels[] starting at various
// offsets, we can generate all of the 7 extra characters needed for a
// bargraph.  This is also stored in program space.
const char levels[] PROGMEM = { 
	0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 
	0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111 
};

// 10 levels of bar graph characters
const char bar_graph_characters[10] = { ' ', 0, 0, 1, 2, 3, 3, 4, 5, 255 };

unsigned int sensors[5];

void init();
void line_follow();
void load_custom_characters();
void bargraph_sensors(unsigned int sensors[5]);

unsigned char button;

////////////////////////////////////////////////////////////////////////
// Main routine
int main() {

	init();

    // Display calibrated values as a bar graph.
    while(!button_is_pressed(BUTTON_B)) {
        // Read the sensor values and get the position measurement.
        unsigned int position = read_line(sensors,IR_EMITTERS_ON);
 
        // Display the position measurement, which will go from 0
        // (when the leftmost sensor is over the line) to 4000 (when
        // the rightmost sensor is over the line) on the 3pi, along
        // with a bar graph of the sensor readings.  This allows you
        // to make sure the robot is ready to go.
        clear();
        print_long(position);
        lcd_goto_xy(0,1);
        bargraph_sensors(sensors);
 
        delay_ms(100);
    }
    wait_for_button_release(BUTTON_B);
    
    delay_ms(200);

	line_follow();
	
 }

////////////////////////////////////////////////////////////////////////
// Initialize everything
void init() {
	pololu_3pi_init_disable_emitter_pin(2000); // should be 500-3000usec
	load_custom_characters();
	clear(); // clear LCD
	lcd_goto_xy(0, 0); // top left
	print("3piLine");
	
    // Display battery voltage and wait for button press
    while(!button_is_pressed(BUTTON_B))
    {
        int bat = read_battery_millivolts();
 
        clear();
        print_long(bat);
        print("mV");
        lcd_goto_xy(0,1);
        print("Press B");
 
        delay_ms(100);
    }
	
   // Always wait for the button to be released so that 3pi doesn't
    // start moving until your hand is away from it.
    wait_for_button_release(BUTTON_B);
    delay_ms(1000);
    
    int counter;
 
    // Auto-calibration: turn right and left while calibrating the
    // sensors.
    for (counter = 0; counter < 80; counter++) {
		
        if (counter < 20 || counter >= 60) {
            set_motors(40,-40);
        } else {
            set_motors(-40,40);
		}
 
        // This function records a set of sensor readings and keeps
        // track of the minimum and maximum values encountered.  The
        // IR_EMITTERS_ON argument means that the IR LEDs will be
        // turned on during the reading, which is usually what you
        // want.
        calibrate_line_sensors(IR_EMITTERS_ON);
 
        // Since our counter runs to 80, the total delay will be
        // 80*20 = 1600 ms.
        delay_ms(20);
    }
    set_motors(0,0);	
}


////////////////////////////////////////////////////////////////////////
// This function performs a single step of line following
//
void line_follow() {
	long now = 0;
	long when;

	clear();
	lcd_goto_xy(0, 0); // top left
	print("GO!");
	// do the run
	time_reset(); // rollover every ~49 days
	when = get_ms() + DT;
	while (1) {
		now = get_ms();

		if (now >= when) { // rollover every ~49 days
			when = now + DT;

			// Read Sensors (2000 is the center, so offset the reading)
			int position = read_line(sensors, IR_EMITTERS_ON) - 2000;

			if (position < -1000) {
				// We're way off the line to the left, so steer hard right
				set_motors(SPEED, 0);
			} else if (position > 1000) {
				// We're way off the line to the right, so steer hard left
				set_motors(0,SPEED);
			} else {
				// We're in the ball park so use PID loop

				// Proportional
				int steer = (position * MAXSTEER) / 1000;
		
				set_motors(SPEED+steer,SPEED-steer);
				left_led(1);
				right_led(1);
			}

			bargraph_sensors(sensors);
			button = get_single_debounced_button_press(ANY_BUTTON);
			delay_ms(1); // ensure we don't run twice.
		}
	}
	set_motors(0,0);
	emitters_off();
}

////////////////////////////////////////////////////////////////////////
// Display bargraph
//
void bargraph_sensors(unsigned int sensors[5]) {
	unsigned int i;
	lcd_goto_xy(0,1);
	print_character('[');
	for (i = 0; i < 5; i++) {
		// Initialize the array of characters that we will use for the
		// graph.  Using the space, an extra copy of the one-bar
		// character, and character 255 (a full black box), we get 10
		// characters in the array.

		// The variable c will have values from 0 to 9, since
		// values are in the range of 0 to 2000, and 2000/201 is 9
		// with integer math.
		char c = bar_graph_characters[sensors[i] / 101];

		// Display the bar graph characters.
		print_character(c);
	}
	print_character(']');
}

////////////////////////////////////////////////////////////////////////
// This function loads custom characters into the LCD.  Up to 8
// characters can be loaded; we use them for 6 levels of a bar graph
//
void load_custom_characters() {
	lcd_load_custom_character(levels + 0, 0); // no offset, e.g. one bar
	lcd_load_custom_character(levels + 1, 1); // two bars
	lcd_load_custom_character(levels + 2, 2); // etc...
	lcd_load_custom_character(levels + 4, 3); // skip level 3
	lcd_load_custom_character(levels + 5, 4);
	lcd_load_custom_character(levels + 6, 5);
	clear(); // the LCD must be cleared for the characters to take effect
}

