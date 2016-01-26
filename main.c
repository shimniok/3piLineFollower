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
#define SPEED 40		// speed for each motor
#define MAXSTEER 40		// maximum steering correction
#define DT 20			// time step in ms, at least 2ms to be safe

#define abs(x) ((x < 0) ? -x : x)

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

	while (1) {
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
	} // while

} // main

////////////////////////////////////////////////////////////////////////
// Initialize everything
void init() {
	pololu_3pi_init_disable_emitter_pin(2000); // should be 500-3000usec
	set_motors(0,0);
	load_custom_characters();
	clear(); // clear LCD
	lcd_goto_xy(0, 0); // top left
	print("3piLine");

  // Display battery voltage and wait for button press
  while(!button_is_pressed(BUTTON_B)) {
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
	// do the run
	time_reset(); // rollover every ~49 days
	when = get_ms() + DT;
	while(!button_is_pressed(BUTTON_B)) {
		now = get_ms();

		if (now >= when) { // rollover every ~49 days
			when = now + DT;

			// Read Sensors, output ranges from 0 to 4000, always
			// If off the line, function returns either 0 or 4000
			// LEFT = 4000
			// RIGHT = 0
			long position = read_line(sensors, IR_EMITTERS_ON);

			// Read_line returns 0 or 4000 if we're off the line.
			// Also, if we're on the line, at least one sensor will
			// read > 200.
			int i;
			char on_line = 0;
			for (i=0; i < 5; i++) {
				if (sensors[i] > 200) {
					on_line = 1;
				}
			}

			// The most reliable range is 1000 to 3000. So in our
			// Proportional control, set p/d to give us MAXSTEER when
			// outside this range
			if (position > 3000) { position = 3000; }
			if (position < 1000) { position = 1000; }
			long p = 50; // proportional
			long const d = 1000;

			// change steering only if we're on the line
			if (on_line) {
				long steer = (position * p) / d - 100;
				set_motors(SPEED+steer, SPEED-steer);

				left_led(steer < -5);
				right_led(steer > 5);
			}

			lcd_goto_xy(0,0);
			print_long(position);
			lcd_goto_xy(0,1);
			bargraph_sensors(sensors);

			button = get_single_debounced_button_press(ANY_BUTTON);
			delay_ms(1); // ensure we don't run twice.
		} // if

	} // while
	set_motors(0,0);
	left_led(0);
	right_led(0);
	emitters_off();
	wait_for_button_release(BUTTON_B);
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
