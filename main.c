/*
 * main.c
 *
 *  Created on: Jun 17, 2013
 *      Author: mes
 */

#include <avr/io.h>
#include "Pololu3pi.h"

#define SPEED 40
#define STEER3 30
#define STEER1 15
#define THRESH 300
#define DT 50 // time step in ms, at least 2ms to be safe

// Data for generating the characters used in load_custom_characters
// and display_readings.  By reading levels[] starting at various
// offsets, we can generate all of the 7 extra characters needed for a
// bargraph.  This is also stored in program space.
const char levels[] PROGMEM = { 0b00000, 0b00000, 0b00000, 0b00000, 0b00000,
		0b00000, 0b00000, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111,
		0b11111 };

// 10 levels of bar graph characters
const char bar_graph_characters[10] = { ' ', 0, 0, 1, 2, 3, 3, 4, 5, 255 };

void init();
void calibrate();
void line_follow();
void load_custom_characters();
void bargraph_sensors(unsigned int sensors[5]);

unsigned char button;

int main() {

	init();

	clear();
	lcd_goto_xy(0, 0); // top left
	print("3piLine");
	delay_ms(2000);

	// loop, repeatedly get ready to run, then run
	while (1) {
		clear();
		lcd_goto_xy(0, 0); // top left
		print("Calib:A");
		lcd_goto_xy(0, 1); // bottom left
		print("Start:B");
		// wait for button press
		do {
			button = get_single_debounced_button_press(ANY_BUTTON);
		} while (button == 0);
		if (button & BUTTON_A)
			calibrate();
		else if (button & BUTTON_B)
			line_follow();
	}
}

// Initialize everything
void init() {
	pololu_3pi_init_disable_emitter_pin(2000); // should be 500-3000usec
	load_custom_characters();
	clear(); // clear LCD
	lcd_goto_xy(0, 0); // top left
	print("3piLine");
}

void calibrate() {
	int i;

	lcd_goto_xy(0, 0); // bottom left
	print("cal... ");
	// Calibrate sensors
	for (i = 0; i < 250; i++) { // make the calibration take about 5 seconds
		calibrate_line_sensors(IR_EMITTERS_ON); // calibrate with LEDs on
		delay(20);
	}
}

// This function performs a single step of line following
//
void line_follow() {
	unsigned int sensors[5];
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
			// Read Sensors
			read_line(sensors, IR_EMITTERS_ON);

			if (sensors[0] > THRESH) {
				set_motors(SPEED-STEER3, SPEED+STEER3);
			} else if (sensors[4] > THRESH) {
				set_motors(SPEED+STEER3, SPEED-STEER3);
			} else if (sensors[3] > THRESH) {
				set_motors(SPEED+STEER1, SPEED-STEER1);
			} else if (sensors[1] > THRESH) {
				set_motors(SPEED-STEER1, SPEED+STEER1);
			} else {
				set_motors(SPEED, SPEED);
			}

			bargraph_sensors(sensors);
			button = get_single_debounced_button_press(ANY_BUTTON);
			if (button & BUTTON_B)
				break;
			delay_ms(1);
		}
	}
	set_motors(0,0);
	emitters_off();
}

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

// This function loads custom characters into the LCD.  Up to 8
// characters can be loaded; we use them for 6 levels of a bar graph
void load_custom_characters() {
	lcd_load_custom_character(levels + 0, 0); // no offset, e.g. one bar
	lcd_load_custom_character(levels + 1, 1); // two bars
	lcd_load_custom_character(levels + 2, 2); // etc...
	lcd_load_custom_character(levels + 4, 3); // skip level 3
	lcd_load_custom_character(levels + 5, 4);
	lcd_load_custom_character(levels + 6, 5);
	clear(); // the LCD must be cleared for the characters to take effect
}

