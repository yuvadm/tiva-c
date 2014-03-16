//*****************************************************************************
//
// tictactoe.c - Provides additional functionality for the qs-cloud example.
//
// Copyright (c) 2013-2014 Texas Instruments Incorporated.  All rights reserved.
// Software License Agreement
// 
// Texas Instruments (TI) is supplying this software for use solely and
// exclusively on TI's microcontroller products. The software is owned by
// TI and/or its suppliers, and is protected under applicable copyright
// laws. You may not combine this software with "viral" open-source
// software in order to form a larger program.
// 
// THIS SOFTWARE IS PROVIDED "AS IS" AND WITH ALL FAULTS.
// NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT
// NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. TI SHALL NOT, UNDER ANY
// CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
// DAMAGES, FOR ANY REASON WHATSOEVER.
// 
// This is part of revision 2.1.0.12573 of the EK-TM4C1294XL Firmware Package.
//
//*****************************************************************************

#include <stdint.h>
#include <stdbool.h>
#include "utils/ustdlib.h"
#include "utils/uartstdio.h"
#include "stats.h"
#include "commands.h"
#include "qs_iot.h"
#include "tictactoe.h"

//*****************************************************************************
//
// Definitions related to the representation of the game state.
//
//*****************************************************************************
#define PLAYER_BIT              0x80000000  // Indicates current player number.
#define REMOTE_PLAYER           0x40000000  // Setting allows remote play.

//*****************************************************************************
//
// Information relating to the current TicTacToe game state
//
//*****************************************************************************
uint32_t g_ui32BoardState = 0;
uint32_t g_ui32LastState = 0;
uint32_t g_ui32Row = 0;
uint32_t g_ui32Col = 0;
uint32_t g_ui32Player = 0;
uint32_t g_ui32Mode = 0;

//*****************************************************************************
//
// State variable for keeping track of the game flow
//
//*****************************************************************************
enum
{
    NEW_GAME,
    SET_MODE,
    PLAY_TURN,
    GET_ROW,
    GET_COLUMN,
    REMOTE_PLAY
}
g_ui32GameState;

//*****************************************************************************
//
// Input buffer for UART input to the TicTacToe game.
//
//*****************************************************************************
#define GAME_INPUT_SIZE         10
char g_pcGameInput[GAME_INPUT_SIZE];

//*****************************************************************************
//
// Global array to track all possible winning configurations of tic-tac-toe.
//
//*****************************************************************************
uint32_t g_ui32WinConditions[] =
{
    0x7,
    0x38,
    0x1C0,
    0x49,
    0x92,
    0x124,
    0x111,
    0x54
};

#define NUM_WIN_CONDITIONS      (sizeof(g_ui32WinConditions)/sizeof(uint32_t))

//*****************************************************************************
//
// Turn prompts a user to play a single turn of tic-tac-toe, and updates the
// global game state variable accordingly. Turn will prevent collisions between
// two separate players on individual squares of the game board, and will
// re-prompt the same player in the event of invalid input.
//
//*****************************************************************************
bool
ProcessTurn(void)
{
    uint32_t ui32Move;

    //
    //
    // If the chosen coordinates are out of range, try asking for a new set
    // of coordinates.
    //
    if(g_ui32Row > 2 || g_ui32Col > 2)
    {
        UARTprintf("Invalid, try again.\n");
        return false;
    }

    //
    // Otherwise, convert the coordinates to the format used by the global
    // state variable.
    //
    ui32Move = 1;
    ui32Move = ui32Move << (g_ui32Row * 3);
    ui32Move = ui32Move << (g_ui32Col * 1);

    //
    // If this space was already occupied, prompt the player for a
    // different move.
    //
    if((ui32Move & g_ui32BoardState) ||
       ((ui32Move << 16) & g_ui32BoardState))
    {
        UARTprintf("Invalid, try again (space occupied).\n");
        return false;
    }
    else
    {
        //
        // Otherwise, the move is valid. Add it to the global state.
        //
        g_ui32BoardState |= (ui32Move << (g_ui32Player * 16));

        //
        // Also flip the player bit, to indicate that the next player should
        // move.
        //
        g_ui32BoardState ^= PLAYER_BIT;
        g_ui32Player = (g_ui32BoardState & PLAYER_BIT) ? 1 : 0;

        return true;
    }
}

//*****************************************************************************
//
// ShowBoard prints an ASCII representation of the current tic-tac-toe board to
// the UART
//
//*****************************************************************************
void
ShowBoard(void)
{
    uint32_t ui32RowNum;
    uint32_t ui32ColNum;
    uint32_t ui32MaskX;
    uint32_t ui32MaskO;

    //
    // Clear the terminal
    //
    UARTprintf("\033[2J\033[H");

    UARTprintf("'%c' Player's turn.\n\n", (g_ui32Player ? 'O' : 'X'));

    //
    // Print out column numbers
    //
    UARTprintf("   0 1 2\n");

    //
    // Loop over rows, starting with zero
    //
    for(ui32RowNum = 0; ui32RowNum < 3; ui32RowNum++)
    {
        //
        // Print the row number
        //
        UARTprintf(" %d ", ui32RowNum);

        //
        // Loop thorugh the columns
        //
        for(ui32ColNum = 0; ui32ColNum < 3; ui32ColNum++)
        {
            //
            // Convert the row/column number into the format used by the global
            // game-state variable.
            //
            ui32MaskX = 1 << (ui32RowNum * 3);
            ui32MaskX = ui32MaskX << (ui32ColNum * 1);
            ui32MaskO = ui32MaskX << 16;

            //
            // If a player has a token in this row and column, print the
            // corresponding symbol
            //
            if(g_ui32BoardState & ui32MaskX)
            {
                UARTprintf("X");
            }
            else if(g_ui32BoardState & ui32MaskO)
            {
                UARTprintf("O");
            }
            else
            {
                UARTprintf(" ");
            }

            //
            // Print column separators where necessary.
            //
            if(ui32ColNum < 2)
            {
                UARTprintf("|");
            }
        }

        //
        // End this row.
        //
        UARTprintf("\n", ui32RowNum);

        //
        // Add a row separator if necessary.
        //
        if(ui32RowNum < 2)
        {
            UARTprintf("   -+-+-\n");
        }

    }

    //
    // Print an extra empty line after the last row.
    //
    UARTprintf("\n");
}

//*****************************************************************************
//
// CheckWinner checks the global state variable to see if either player has
// won, or if the game has ended in a tie. Returns a 1 if the game is over, or
// a 0 if the game should continue.
//
//*****************************************************************************
bool
CheckWinner(void)
{
    uint32_t ui32Idx;
    uint32_t ui32WinMask0;
    uint32_t ui32WinMask1;
    uint32_t ui32CatCheck;
    uint32_t ui32QuitCheck;

    //
    // Loop through the table of win-conditions.
    //
    for(ui32Idx = 0; ui32Idx < NUM_WIN_CONDITIONS; ui32Idx++)
    {
        //
        // Get a winning board configuration from the global table, and create
        // bit masks for each player corresponding to that win condition.
        //
        ui32WinMask0 = g_ui32WinConditions[ui32Idx];
        ui32WinMask1 = g_ui32WinConditions[ui32Idx] << 16;

        //
        // If a player's pieces line up with the winning configuration, count
        // this as a win.
        //
        if((g_ui32BoardState & ui32WinMask0) == ui32WinMask0)
        {
            UARTprintf("'X' Wins!\n", (g_ui32Player ? 'O' : 'X'));
            return 1;
        }
        else if((g_ui32BoardState & ui32WinMask1) == ui32WinMask1)
        {
            UARTprintf("'O' Wins!\n", (g_ui32Player ? 'O' : 'X'));
            return 1;
        }
    }

    //
    // AND together the position bits for both players to see how many spaces
    // are occupied.
    //
    ui32CatCheck = ((g_ui32BoardState | (g_ui32BoardState >> 16)) & 0x1FF);

    //
    // The server will signify a "quit" request by setting all of a single
    // player's bits high. Check for one of these states, and print a message
    // if it is found.
    //
    ui32QuitCheck = (g_ui32BoardState & 0x01FF);
    if(ui32QuitCheck == 0x01FF)
    {
        UARTprintf("Game ended by other player.\n");
        return 1;
    }

    ui32QuitCheck = (g_ui32BoardState & 0x01FF0000);
    if(ui32QuitCheck == 0x01FF0000)
    {
        UARTprintf("Game ended by other player.\n");
        return 1;
    }

    //
    // If all spaces are full, and no winner was detected, declare this a tie.
    //
    if(ui32CatCheck == 0x1FF)
    {
        UARTprintf("It's a tie.\n");
        return 1;
    }

    //
    // If the player's pieces do not line up with a known winning
    // configuration, return a zero, indicating that no winner was found.
    //
    return 0;
}

//*****************************************************************************
//
// SetGameMode reads the user input to determine whether TicTacToe will be
// played locally or online, and whether the local player will play first or
// second. This function will return a 1 if the user-selected mode setting was
// valid, or a 0 if the mode could not be selected.
//
//*****************************************************************************
bool
SetGameMode(void)
{
    uint32_t ui32InputMode;

    //
    // If there wasn't any user input, return immediately.
    //
    if(UARTPeek('\r') == -1)
    {
        return 0;
    }

    //
    // Pull the user input from the UART, and convert it to an integer.
    //
    UARTgets(g_pcGameInput, GAME_INPUT_SIZE);
    ui32InputMode = ustrtoul(g_pcGameInput, 0, 0);

    //
    // Check to make sure we have a valid mode selection.
    //
    if(ui32InputMode == 3)
    {
        //
        // If the selected mode is "online, remote player first", set the state
        // variables accordingly.
        //
        g_ui32Mode = ui32InputMode;

        //
        // Setting the REMOTE_PLAYER bit will alert the remote user interface
        // that they should make the first move. Setting the global variable
        // for the old state allows the state machine to detect when the remote
        // play has happened.
        //
        g_ui32LastState = REMOTE_PLAYER;
        g_ui32BoardState = REMOTE_PLAYER;
        g_sBoardState.eReadWriteType = READ_WRITE;

        return 1;
    }
    else if((ui32InputMode > 0) && (ui32InputMode < 4))
    {
        //
        // If the user entered a different valid choice, set up the game mode,
        // but don't request a play from the remote interface.
        //
        g_ui32Mode = ui32InputMode;
        g_ui32LastState = 0x0;
        g_ui32BoardState = 0x0;
        g_sBoardState.eReadWriteType = WRITE_ONLY;

        return 1;
    }
    else
    {
        //
        // Invalid input.
        //
        UARTprintf("Invalid input. Try again: ");

        return 0;
    }
}

//*****************************************************************************
//
// This function implements a state machine for the tic-tac-toe gameplay.
//
//*****************************************************************************
bool
AdvanceGameState(void)
{
    //
    // If the user has typed a Q, skip straight to ending the game.
    //
    if((UARTPeek('Q') >= 0) && (UARTPeek('\r') >=0))
    {
        //
        // Remove the Q from the buffer.
        //
        UARTgets(g_pcGameInput, GAME_INPUT_SIZE);

        //
        // This board state signals a 'quit' condition to the server.
        //
        g_ui32BoardState = 0x01FF01FF;
        g_sBoardState.eReadWriteType = WRITE_ONLY;

        //
        // Print a quit message.
        //
        UARTprintf("\nGame Over.\n");
        return 1;
    }

    //
    // This switch statement controls the main flow of the game.
    //
    switch(g_ui32GameState)
    {
        case NEW_GAME:
        {
            //
            // For a new game, the first step is to determine the game mode.
            // Prompt the user for a game mode via UART, and advance the state
            // to wait for the user's response.
            //
            UARTprintf("\033[2J\033[H");
            UARTprintf("New Game!\n");
            UARTprintf("  1 - play locally\n");
            UARTprintf("  2 - play online, local user starts\n");
            UARTprintf("  3 - play online, remote user starts\n");
            UARTprintf("  Q - Enter Q at any time during play to quit.\n\n");
            UARTprintf("Select an option (1-3 or Q): ");

            g_ui32GameState = SET_MODE;
            break;
        }

        case SET_MODE:
        {
            //
            // Only continue if we have input from the user.
            //
            if(UARTPeek('\r') != -1)
            {
                //
                // Attempt to use the user's input to set the game mode.
                //
                if(SetGameMode())
                {
                    //
                    // If the user input was valid, show the game board and
                    // advance the state to start the first turn.
                    //
                    ShowBoard();
                    g_ui32GameState = PLAY_TURN;
                }
            }

            break;
        }

        case PLAY_TURN:
        {
            //
            // Check to see if we need input from the local user. This will
            // always be true for a local game, and should be true for only a
            // single player's turns for an online game.
            //
            if(!(g_ui32BoardState & REMOTE_PLAYER))
            {
                //
                // If we're playing a local game, prompt for a row number and
                // advance the state to wait for a response.
                //
                UARTprintf("Enter Row: ");
                g_ui32GameState = GET_ROW;
            }
            else
            {
                //
                // If the local player is not supposed to move for this turn,
                // print a message to let the player know that we are waiting
                // on input from a remote player.
                //
                UARTprintf("Waiting for remote player....\n");
                g_ui32GameState = REMOTE_PLAY;
            }

            break;
        }

        case GET_ROW:
        {
            //
            // Only continue if we have input from the user.
            //
            if(UARTPeek('\r') != -1)
            {
                //
                // Convert the user's input to an integer, and store it as the
                // new row number.
                //
                UARTgets(g_pcGameInput, GAME_INPUT_SIZE);
                g_ui32Row = ustrtoul(g_pcGameInput, 0, 0);

                //
                // Prompt for a column number, and advance the state to wait
                // for a response.
                //
                UARTprintf("Enter Column: ");
                g_ui32GameState = GET_COLUMN;
            }

            break;
        }

        case GET_COLUMN:
        {
            //
            // Only continue if we have input from the user.
            //
            if(UARTPeek('\r') != -1)
            {
                //
                // Convert the user's input to an integer, and store it as the
                // new column number.
                //
                UARTgets(g_pcGameInput, GAME_INPUT_SIZE);
                g_ui32Col = ustrtoul(g_pcGameInput, 0, 0);

                //
                // Try to process the recorded row and column numbers as a
                // "move" for the current player.
                //
                if(ProcessTurn())
                {
                    //
                    // The user's input was successfully processed and added to
                    // the game state. Show the board with the new move
                    // applied.
                    //
                    ShowBoard();

                    //
                    // Check to see if this was a winning move.
                    //
                    if(CheckWinner())
                    {
                        //
                        // If so, return a 1 to signal the end of the game.
                        //
                        return 1;
                    }
                    else
                    {

                        //
                        // Otherwise, the game must go on. Check to see if we
                        // have a remote player.
                        //
                        if(g_ui32Mode != 1)
                        {
                            //
                            // We have a remote player, so toggle the bit to
                            // signal that the remote player should take their
                            // turn.
                            //
                            g_ui32BoardState ^= REMOTE_PLAYER;
                        }

                        //
                        // Remember the board state, so we can tell when it
                        // gets changed.
                        //
                        g_ui32LastState = g_ui32BoardState;

                        //
                        // Set the board state to sync with the server.
                        //
                        g_sBoardState.eReadWriteType = READ_WRITE;

                        //
                        // Finally, set the game state for the next turn.
                        //
                        g_ui32GameState = PLAY_TURN;
                    }
                }
                else
                {
                    //
                    // Something was wrong with the user's input. Try prompting
                    // them again.
                    //
                    UARTprintf("Enter Row: ");
                    g_ui32GameState = GET_ROW;
                }
            }

            break;
        }

        case REMOTE_PLAY:
        {
            //
            // If we are waiting on a remote player, check to see if the board
            // state variable has changed.
            //
            if(g_ui32BoardState != g_ui32LastState)
            {
                //
                // Set the board state to stop reading from the server.
                //
                g_sBoardState.eReadWriteType = WRITE_ONLY;

                //
                // Record the new state, so we know that it has already been
                // seen once. This is important to prevent an infinite loop if
                // the server doesn't clear the "REMOTE_PLAYER" bit.
                //
                g_ui32LastState = g_ui32BoardState;

                //
                // Make sure that the player variable is up-to-date.
                //
                g_ui32Player = (g_ui32BoardState & PLAYER_BIT) ? 1 : 0;

                //
                // If the state has changed, assume that the remote player has
                // made their move.
                //
                ShowBoard();

                //
                // Check to see if this was a winning move.
                //
                if(CheckWinner())
                {
                    //
                    // If so, return a 1 to signal the end of the game.
                    //
                    return 1;
                }
                else
                {
                    //
                    // Otherwise, update the last valid state, advance to the
                    // next turn.
                    //
                    g_ui32GameState = PLAY_TURN;
                }
            }

            break;
        }
    }

    //
    // The actions for the current state have been processed, and the game has
    // not met an ending condition. Return a zero to indicate that the game is
    // not yet finished.
    //
    return 0;
}

//*****************************************************************************
//
// Clears the game state, and prepares the global variables to start a new game
// of tic-tac-toe
//
//*****************************************************************************
void
GameInit(void)
{
    //
    // Set the global board state tStat variable to WRITE_ONLY, to make sure
    // that it doesn't get overwritten by content from the server side.
    //
    g_sBoardState.eReadWriteType = WRITE_ONLY;

    //
    // Empty the board, set the player value to zero (for 'X'), and set the
    // main state machine to start a new game on the next call to
    // AdvanceGameState().
    //
    g_ui32BoardState = 0;
    g_ui32Player = 0;
    g_ui32GameState = NEW_GAME;
}

