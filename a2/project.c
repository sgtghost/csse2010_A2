/*
 * project.c
 *
 * Main file
 *
 * Authors: Peter Sutton, Luke Kamols, Jarrod Bennett
 * Modified by Tie Wang s4621539
 */ 

#include <stdio.h>
#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#define F_CPU 8000000L
#include <util/delay.h>

#include "game.h"
#include "display.h"
#include "ledmatrix.h"
#include "buttons.h"
#include "serialio.h"
#include "terminalio.h"
#include "timer0.h"

// Function prototypes - these are defined below (after main()) in the order
// given here
void initialise_hardware(void);
void start_screen(void);
void new_game(void);
void play_game(void);
void handle_game_over(void);


// Game pause status
uint8_t pause;
/////////////////////////////// main //////////////////////////////////
int main(void) {
	// Setup hardware and call backs. This will turn on 
	// interrupts.
	initialise_hardware();
	
	// Show the splash screen message. Returns when display
	// is complete
	start_screen();
	
	// Loop forever,
	while(1) {
		new_game();
		play_game();
		handle_game_over();
	}
}

void initialise_hardware(void) {
	ledmatrix_setup();
	init_button_interrupts();
	// Setup serial port for 19200 baud communication with no echo
	// of incoming characters
	init_serial_stdio(19200,0);
	
	// Setup up the output ports
	// For Valid movement detection display, AVR port C pin 0
	// is connected to LED L0 to output the result
	
	/* Make all bits of port A and the least significant
	** bit of port C be output bits.
	*/
	DDRA = 0xFF;
	DDRC = 0x01;
	PORTC = 0;
	
	/* Set up timer/counter 1 so that we get an 
	** interrupt 100 times per second, i.e. every
	** 10 milliseconds.
	*/
	OCR1A = 9999; /* Clock divided by 8 - count for 10000 cycles */
	TCCR1A = 0; /* CTC mode */
	TCCR1B = (1<<WGM12)|(1<<CS11); /* Divide clock by 8 */

	/* Enable interrupt on timer on output compare match 
	*/
// 	TIMSK1 = (1<<OCIE1A);

	/* Ensure interrupt flag is cleared */
	TIFR1 = (1<<OCF1A);
	
	init_timer0();
	
	// Turn on global interrupts
	sei();
}

void start_screen(void) {
	// Clear terminal screen and output a message
	clear_terminal();
	move_terminal_cursor(10,10);
	printf_P(PSTR("Teeko"));
	move_terminal_cursor(10,12);
	printf_P(PSTR("CSSE2010/7201 project by Tie Wang s4621539"));
	
	// Output the static start screen and wait for a push button 
	// to be pushed or a serial input of 's'
	start_display();
	
	// Wait until a button is pressed, or 's' is pressed on the terminal
	while(1) {
		// First check for if a 's' is pressed
		// There are two steps to this
		// 1) collect any serial input (if available)
		// 2) check if the input is equal to the character 's'
		char serial_input = -1;
		if (serial_input_available()) {
			serial_input = fgetc(stdin);
		}
		// If the serial input is 's', then exit the start screen
		if (serial_input == 's' || serial_input == 'S') {
			break;
		}
		// Next check for any button presses
		int8_t btn = button_pushed();
		if (btn != NO_BUTTON_PUSHED) {
			break;
		}
	}
}

void new_game(void) {
	// Clear the serial terminal
	clear_terminal();
	
	// Initialise the game and display
	initialise_game();
	
	// Clear a button push or serial input if any are waiting
	// (The cast to void means the return value is ignored.)
	(void)button_pushed();
	clear_serial_input_buffer();
	
	// Setup pause state
	pause = 0;
}

void play_game(void) {
	
	uint32_t last_flash_time, current_time;
	uint8_t btn; //the button pushed
	char serial_input; //the keyboard input
	
	last_flash_time = get_current_time();
	
	// We play the game until it's over
	while(!is_game_over()) {
		// We need to check if any button has been pushed, this will be
		// NO_BUTTON_PUSHED if no button has been pushed
		if (!pause) {
					btn = button_pushed();
					serial_input = -1;
					if (btn == BUTTON3_PUSHED) {
						// If button 3 is pushed, move left,
						// i.e decrease x by 1 and leave y the same
						move_display_cursor(-1, 0);
						last_flash_time = get_current_time();
						} else if (btn == BUTTON2_PUSHED) {
						// If button 2 is pushed, move right,
						// i.e increase x by 1 and leave y the same
						move_display_cursor(1, 0);
						last_flash_time = get_current_time();
						} else if (btn == BUTTON1_PUSHED) {
						// If button 1 is pushed, move up,
						// i.e increase y by 1 and leave x the same
						move_display_cursor(0, 1);
						last_flash_time = get_current_time();
						} else if (btn == BUTTON0_PUSHED) {
						// If button 0 is pushed, move down,
						// i.e decrease y by 1 and leave x the same
						move_display_cursor(0, -1);
						last_flash_time = get_current_time();
					}
					
					if (serial_input_available()) {
						serial_input = fgetc(stdin);
						if (serial_input == 'a' || serial_input == 'A') {
							// If the serial input is 'a/A', move left,
							// i.e decrease x by 1 and leave y the same
							move_display_cursor(-1, 0);
							if (check_phase() == 3) {
								check_valid_move(3);
							}
							} else if (serial_input == 'd' || serial_input == 'D') {
							// If the serial input is 'd/D', move right,
							// i.e increase x by 1 and leave y the same
							move_display_cursor(1, 0);
							if (check_phase() == 3) {
								check_valid_move(3);
							}
							} else if (serial_input == 'w' || serial_input == 'W') {
							// If the serial input is 'w/W', move up,
							// i.e increase y by 1 and leave x the same
							move_display_cursor(0, 1);
							if (check_phase() == 3) {
								check_valid_move(3);
							}
							} else if (serial_input == 's' || serial_input == 'S') {
							// If the serial input is 's/S', move down,
							// i.e decrease y by 1 and leave x the same
							move_display_cursor(0, -1);
							if (check_phase() == 3) {
								check_valid_move(3);
							}
							} else if (serial_input == ' ') {
							// If the serial input is ' ', place or remove the pieces
							// based on the phase.
							if (check_phase() == 1) {
								if (check_valid_move(1) != 1) {
									if (PORTC ^ 1) {
										PORTC ^= 1;
									}
									continue;
								}
								if (!(PORTC ^ 1)) {
									PORTC ^= 1;
								}
								piece_placement();
								// Note: The original phase 2 is split into 2 phases here:
								// The new 'Phase 2' represents the stage in OG phase 2 where players
								// pick up their pieces.
								} else if (check_phase() == 2){
								if (check_valid_move(2) != 1) {
									if (PORTC ^ 1) {
										PORTC ^= 1;
									}
									continue;
								}
								remove_piece();
								if (!(PORTC ^ 1)) {
									PORTC ^= 1;
								}
								// And the new 'Phase 3' represents the stage in OG phase 2 where player put down
								// their pick-up pieces.
								} else if (check_phase() == 3){
								if (check_valid_move(3) != 1) {
									if (PORTC ^ 1) {
										PORTC ^= 1;
									}
									continue;
								}
								piece_placement();
								if (!(PORTC ^ 1)) {
									PORTC ^= 1;
								}
							}
							} else if (serial_input == 'p' || serial_input == 'P') {
							// If the serial input is 'p/P', pause or unpause the game
							pause ^= 1;
						}
					}
		} else {
			if (serial_input_available()) {
				serial_input = fgetc(stdin);
				if (serial_input == 'p' || serial_input == 'P') {
					// If the serial input is 'p/P', pause or unpause the game
					pause ^= 1;
				}
			}
		}
		
		current_time = get_current_time();
		if(current_time >= last_flash_time + 500) {
			// 500ms (0.5 second) has passed since the last time we
			// flashed the cursor, so flash the cursor
			flash_cursor();
			
			// Update the most recent time the cursor was flashed
			last_flash_time = current_time;
		}
	}
	// We get here if the game is over.
}

void handle_game_over() {
	move_terminal_cursor(10,14);
	printf_P(PSTR("GAME OVER"));
	move_terminal_cursor(10,15);
	printf_P(PSTR("Winner is %d"), current_player);
	move_terminal_cursor(10,16);
	printf_P(PSTR("Press a button to start again"));
	while(button_pushed() == NO_BUTTON_PUSHED) {
		 // wait
	}
}
