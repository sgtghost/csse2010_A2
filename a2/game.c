/*
 * game.c
 *
 * Contains functions relating to the play of the game Teeko
 *
 * Authors: Luke Kamols, Jarrod Bennett
 */ 

#include "game.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "display.h"
#include "terminalio.h"

// Start pieces in the middle of the board
#define CURSOR_X_START ((int)(WIDTH/2))
#define CURSOR_Y_START ((int)(HEIGHT/2))

uint8_t board[WIDTH][HEIGHT];
// cursor coordinates should be /* SIGNED */ to allow left and down movement.
// All other positions should be unsigned as there are no negative coordinates.
int8_t cursor_x;
int8_t cursor_y;
int8_t last_cursor_x;
int8_t last_cursor_y;
uint8_t current_cursor;
uint8_t cursor_visible;
uint8_t current_player;

// Pieces & Phase record
uint8_t player1_pieces;
uint8_t player2_pieces;
uint8_t phase;

void initialise_game(void) {
	
	// initialise the display we are using
	initialise_display();
	
	// initialise the board to be all empty
	for (uint8_t x = 0; x < WIDTH; x++) {
		for (uint8_t y = 0; y < HEIGHT; y++) {
			board[x][y] = EMPTY_SQUARE;
		}
	}
	
	// set the starting player
	current_player = PLAYER_1;

	// also set where the cursor starts
	cursor_x = CURSOR_X_START;
	cursor_y = CURSOR_Y_START;
	cursor_visible = 0;
	current_cursor = CURSOR;
	
	// Last position of cursor
	last_cursor_x = CURSOR_X_START;
	last_cursor_y = CURSOR_Y_START;
	
	// and the pieces & phase
	player1_pieces = 0;
	player2_pieces = 0;
	phase = 1;
	
	// Print the starting turn message
	move_terminal_cursor(10, 10);
	printf("Current player: 1, green");
}

uint8_t get_piece_at(uint8_t x, uint8_t y) {
	// check the bounds, anything outside the bounds
	// will be considered empty
	if (x < 0 || x >= WIDTH || y < 0 || y >= WIDTH) {
		return EMPTY_SQUARE;
	} else {
		//if in the bounds, just index into the array
		return board[x][y];
	}
}

void flash_cursor(void) {
	if (cursor_visible) {
		// we need to flash the cursor off, it should be replaced by
		// the colour of the piece which is at that location
		uint8_t piece_at_cursor = get_piece_at(cursor_x, cursor_y);
		update_square_colour(cursor_x, cursor_y, piece_at_cursor);
		
	} else {
		// we need to flash the cursor on
		update_square_colour(cursor_x, cursor_y, current_cursor);
	}
	cursor_visible = 1 - cursor_visible; //alternate between 0 and 1
}

//check the header file game.h for a description of what this function should do
// (it may contain some hints as to how to move the cursor)
void move_display_cursor(int8_t dx, int8_t dy) {
	//YOUR CODE HERE
	/*suggestions for implementation:
	 * 1: remove the display of the cursor at the current location
	 *		(and replace it with whatever piece is at that location)
	 * 2: update the positional knowledge of the cursor, this will include
	 *		variables cursor_x, cursor_y and cursor_visible. Make sure you
	 *		consider what should happen if the cursor moves off the board.
	 * 3: display the cursor at the new location
	 * 4: reset the cursor flashing cycle. See project.c for how the cursor
	 *		is flashed.
	 */
	// 1: Replace the display of the cursor at its old location with whatever is there right now
	uint8_t piece_at_cursor = get_piece_at(cursor_x, cursor_y);
	update_square_colour(cursor_x, cursor_y, piece_at_cursor);
	// 2: update the positional knowledge & visibility of the cursor
	cursor_x = cursor_x + dx;
	if (cursor_x >= 0) {
		cursor_x %= WIDTH;
	} else {
		cursor_x += WIDTH;
	}
	 
	cursor_y = cursor_y + dy;
	if (cursor_y >= 0) {
		cursor_y %= HEIGHT;
		} else {
		cursor_y += HEIGHT;
	}
	cursor_visible = 0;
	// 3: display the cursor at the new location
	flash_cursor();
}

// attempt to place a piece at the current position. If successful, the
// active player is switched.
void piece_placement(void) {
	if(board[cursor_x][cursor_y] == EMPTY_SQUARE) {
		board[cursor_x][cursor_y] = current_player;
		update_square_colour(cursor_x, cursor_y, current_player);
		if (current_player == PLAYER_1) {
			player1_pieces += 1;
			current_player = PLAYER_2;
		} else {
			player2_pieces += 1;
			current_player = PLAYER_1;
		}
		if (current_player == PLAYER_1) {
			clear_terminal();
			move_terminal_cursor(10, 10);
			printf("Current player: 1, green");
		} else {
			clear_terminal();
			move_terminal_cursor(10, 10);
			printf("Current player: 2, red");
		}
	}
	if (player1_pieces == 4 && player2_pieces == 4) {
		phase = 2;
		clear_terminal();
		move_terminal_cursor(10, 10);
		printf("Current Phase: %d, Current Player : %d", phase, current_player);
	}
}

void remove_piece(void) {
	if(board[cursor_x][cursor_y] != EMPTY_SQUARE && current_player == board[cursor_x][cursor_y]) {
		board[cursor_x][cursor_y] = EMPTY_SQUARE;
		update_square_colour(cursor_x, cursor_y, EMPTY_SQUARE);
		if (current_player == PLAYER_1) {
			player1_pieces -= 1;
		} else {
			player2_pieces -= 1;
		} 
		current_cursor = PICK_CURSOR;
		last_cursor_x = cursor_x;
		last_cursor_y = cursor_y;
		phase = 3;
	}
}


uint8_t check_valid_move(uint8_t phase) {
	if (phase == 1) {
		if (board[cursor_x][cursor_y] == EMPTY_SQUARE) {
			return 1;
		}
	} else if (phase == 2) {
		if (board[cursor_x][cursor_y] != EMPTY_SQUARE) {
			if (board[cursor_x][cursor_y] == current_player) {
				return 1;
			}
		}
	} else if (phase == 3) {
		if (abs(cursor_x - last_cursor_x) <= 1 && abs(cursor_y - last_cursor_y) <= 1) {
			if (cursor_x != last_cursor_x || cursor_y != last_cursor_y) {
				if (board[cursor_x][cursor_y] == EMPTY_SQUARE) {
					return 1;
				}
			}
		}
	}
	
	return 0;
}

uint8_t check_phase() {
	return phase;
}


uint8_t is_game_over(void) {
	// YOUR CODE HERE
	// Detect if the game is over i.e. if a player has won.
	return 0;
}