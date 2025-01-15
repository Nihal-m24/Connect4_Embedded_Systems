/*         Your Name & E-mail: Muhammad Memon mmemo005@ucr.edu
 *         Discussion Section: 24
 *         Assignment: Custom Project Part 2
 *
 *         I acknowledge all content contained herein, excluding template or example code, is my own original work.
 *
 *         Full Game Play: https://www.youtube.com/watch?v=SvRot21eqtc
 *         Build Upon#1: https://www.youtube.com/watch?v=TMe5bN8cUdQ
 *         Build Upon#2: https://www.youtube.com/watch?v=CajaHFgYCWA
 *         Build Upon#3: https://www.youtube.com/watch?v=tv2kqMu10bA
 *         Build Upon#4: https://www.youtube.com/watch?v=lT29cijZHZk

 */


#include <Arduino.h>
#include "mmemo005_esp32_timerISR.h"
#include "FastLED.h"
#include "Wire.h"

#define NUM_TASKS 9 

#define NUM_LEDS 192          // Total number of LEDs in the matrix

#define O 0
#define W 1
#define R 2
#define B 3

//GPIO PINS
#define DATA_PIN 23     // OUTPUT Pin connected to the LED strip data line
const int Bit0PIN = 16; // INPUT
const int Bit1PIN = 17; // INPUT
const int Bit2PIN = 18; // INPUT
const int BTNPIN = 19; // INPUT
const int PLYRTRNPIN = 27; // OUTPUT
const int GAMESTATUSPIN = 26; //OUTPUT
const int SYSSTATUSRECIEVEPIN = 21; // input
const int GAMESTATUSRECIEVEPIN = 22; // INPUT
const int GAMEWONARDUINOPIN = 25; //OUTPUT
const int PLYRTURNARDUINOPIN = 33; //OUTPUT

//GLOBAL VARIABLES
bool systemIsOn = false;
bool gameStarted = false;
unsigned char player1Score = 0;
unsigned char player2Score = 0;
bool player1Turn = true;
CRGB leds[NUM_LEDS];
unsigned char chipPosition = 0;
int Bit0 = 0;
int Bit1 = 0;
int Bit2 = 0;
int chipsDropped = 0;
int dropChipBtnCount = 0;
bool gameWon = false;
int resetCount = 0;

int player1ScoreLEDMatrix[64] = {
    O, O, O, O, O, O, O, O,
    O, O, O, O, O, O, O, O,
    O, O, O, O, O, O, O, O,
    O, O, O, O, O, O, O, O,
    O, O, O, O, O, O, O, O,
    O, O, O, O, O, O, O, O,
    O, O, O, O, O, O, O, O,
    O, O, O, O, O, O, O, O
};

int gameBoardLEDMatrix[64] = {
    O, O, O, O, O, O, O, O,
    O, O, O, O, O, O, O, O,
    O, O, O, O, O, O, O, O,
    O, O, O, O, O, O, O, O,
    O, O, O, O, O, O, O, O,
    O, O, O, O, O, O, O, O,
    O, O, O, O, O, O, O, O,
    O, O, O, O, O, O, O, O
};

int player2ScoreLEDMatrix[64] = {
    O, O, O, O, O, O, O, O,
    O, O, O, O, O, O, O, O,
    O, O, O, O, O, O, O, O,
    O, O, O, O, O, O, O, O,
    O, O, O, O, O, O, O, O,
    O, O, O, O, O, O, O, O,
    O, O, O, O, O, O, O, O,
    O, O, O, O, O, O, O, O
};

int player1Score2D[8][8] = {
    {O, O, O, O, O, O, O, O},
    {O, O, O, O, O, O, O, O},
    {O, O, O, O, O, O, O, O},
    {O, O, O, O, O, O, O, O},
    {O, O, O, O, O, O, O, O},
    {O, O, O, O, O, O, O, O},
    {O, O, O, O, O, O, O, O},
    {O, O, O, O, O, O, O, O}};

int player2Score2D[8][8] = {
    {O, O, O, O, O, O, O, O},
    {O, O, O, O, O, O, O, O},
    {O, O, O, O, O, O, O, O},
    {O, O, O, O, O, O, O, O},
    {O, O, O, O, O, O, O, O},
    {O, O, O, O, O, O, O, O},
    {O, O, O, O, O, O, O, O},
    {O, O, O, O, O, O, O, O}};

int gameBoard2D[8][8] = {
    {O, O, O, O, O, O, O, O},
    {O, O, O, O, O, O, O, O},
    {O, O, O, O, O, O, O, O},
    {O, O, O, O, O, O, O, O},
    {O, O, O, O, O, O, O, O},
    {O, O, O, O, O, O, O, O},
    {O, O, O, O, O, O, O, O},
    {O, O, O, O, O, O, O, O}};

int rowsFilledStatus[8] = {8, 6, 6, 6, 6, 6, 6, 8} ;

int winningChips[4][2];

typedef struct _task{
	signed 	 char state; 		
	unsigned long period; 		
	unsigned long elapsedTime; 	
	int (*TickFct)(int); 		
} task;

const unsigned long GCD_PERIOD = 100;
const unsigned long BOARD_PERIOD = 200;
const unsigned long JYSTCK_PERIOD = 200;
const unsigned long BTN_PERIOD = 200;
const unsigned long PLYR_TRN_SEND_PERIOD = 200;
const unsigned long GAME_WON_CHECK_PERIOD = 700;
const unsigned long UPDATE_GAME_BOARD_PERIOD = 200;
const unsigned long GAME_STATUS_SEND_PERIOD = 200;
const unsigned long SYSTEM_STATUS_READ_PERIOD = 200;
const unsigned long RESET_BTN_PERIOD = 200;

task tasks[NUM_TASKS]; 

enum BoardStates {BOARDINIT, BOARDOFF, BOARDON, BOARDUPDATE} BoardState;
int BoardTick(int state);

enum JYSTCKStates {JYSTCKINIT, JYSTCKOFF, JYSTCKON} JYSTCKState;
int JYSTCKTick(int state);

enum BTNStates {BTNINIT, BTNWAIT, BTNPRESS} BTNState;
int BTNTick(int state);

enum PLYRTRNSendStates {PLYRTRNSENDINIT, PLYRTRNSENDON} PLYRTRNSendState;
int PLYRTRNSendTick(int state);

enum GameWonStatusStates {GAMEWONSTATINIT, GAMEWONSTATCHECK} GameWonStatusState;
int GameWonCheckTick(int state);

enum UpdateGameBoardStates {UPDATEGAMEBOARDINIT, UPDATEGAMEBOARDON} UpdateGameBoardState;
int UpdateGameBoardTick(int state);

enum GameStatusSendStates {GAMESTATUSSENDINIT, GAMESTATUSSENDON} GameStatusSendState;
int GameStatusSendTick(int state);

enum SystemStatusReadStates {SYSTEMSTATUSREADINIT, SYSTEMSTATUSREADON} SystemStatusReadState;
int SystemStatusReadTick(int state);

enum ResetBTNStates {RESETBTNINIT, RESETBTNWAIT, RESETBTNPRESS} ResetBTNState;
int ResetBTNTick(int state);


void TimerISR() {
	for ( unsigned int i = 0; i < NUM_TASKS; i++ ) {                   // Iterate through each task in the task array
		if ( tasks[i].elapsedTime == tasks[i].period ) {           // Check if the task is ready to tick
			tasks[i].state = tasks[i].TickFct(tasks[i].state); // Tick and set the next state for this task
			tasks[i].elapsedTime = 0;                          // Reset the elapsed time for the next tick
		}
		tasks[i].elapsedTime += GCD_PERIOD;                        // Increment the elapsed time by GCD_PERIOD
	}
}

bool recursiveRowCheck(int currGameState[8][8], int colorCheck, int row, int column, int &chipsConnectingCount){

    if(currGameState[row][column] != colorCheck){
        for(unsigned int i = 0; i < 4; i++){
            winningChips[i][0] = 0;
            winningChips[i][1] = 0;
        }

        chipsConnectingCount = 0;
        return false;
    }
    
    chipsConnectingCount++;

    winningChips[chipsConnectingCount][0] = row;
    winningChips[chipsConnectingCount][1] = column;

    if(chipsConnectingCount >= 3){
        return  true;
    }
    
    if(column == 6){
        for(unsigned int i = 0; i < 4; i++){
            winningChips[i][0] = 0;
            winningChips[i][1] = 0;
        }

        chipsConnectingCount = 0;
        return false;
    }
    
    
    return recursiveRowCheck(currGameState, colorCheck, row, column + 1, chipsConnectingCount);
}

bool recursiveColumnCheck(int currGameState[8][8], int colorCheck, int row, int column, int &chipsConnectingCount){

    if(currGameState[row][column] != colorCheck){
        chipsConnectingCount = 0;
        for(unsigned int i = 0; i < 4; i++){
            winningChips[i][0] = 0;
            winningChips[i][1] = 0;
        }

        return false;
    }
    
    if(row == 0){
        chipsConnectingCount = 0;
        for(unsigned int i = 0; i < 4; i++){
            winningChips[i][0] = 0;
            winningChips[i][1] = 0;
        }

        return false;
    }
    
    chipsConnectingCount++;
    winningChips[chipsConnectingCount][0] = row;
    winningChips[chipsConnectingCount][1] = column;

    
    if(chipsConnectingCount >= 3){
        return true;
    }
    
    return recursiveColumnCheck(currGameState, colorCheck, row - 1, column, chipsConnectingCount);
}

bool recursiveRightDiagnalCheck(int currGameState[8][8], int colorCheck, int row, int column, int &chipsConnectingCount){

    if(currGameState[row][column] != colorCheck){
        chipsConnectingCount = 0;
        for(unsigned int i = 0; i < 4; i++){
            winningChips[i][0] = 0;
            winningChips[i][1] = 0;
        }

        return false;
    }
    
    chipsConnectingCount++;
    winningChips[chipsConnectingCount][0] = row;
    winningChips[chipsConnectingCount][1] = column;

    
    if(chipsConnectingCount >= 3){
        return  true;
    }
    
    if(row == 0){
        chipsConnectingCount = 0;
        for(unsigned int i = 0; i < 4; i++){
            winningChips[i][0] = 0;
            winningChips[i][1] = 0;
        }

        return false;
    }
    
    if(column == 6){
        for(unsigned int i = 0; i < 4; i++){
            winningChips[i][0] = 0;
            winningChips[i][1] = 0;
        }

        chipsConnectingCount = 0;
        return false;
    }
   
    return recursiveRightDiagnalCheck(currGameState, colorCheck, row - 1, column + 1, chipsConnectingCount);
}

bool recursiveLeftDiagnalCheck(int currGameState[8][8], int colorCheck, int row, int column, int &chipsConnectingCount){

    if(currGameState[row][column] != colorCheck){
        chipsConnectingCount = 0;
        for(unsigned int i = 0; i < 4; i++){
            winningChips[i][0] = 0;
            winningChips[i][1] = 0;
        }
        return false;
    }
    
    chipsConnectingCount++;
    winningChips[chipsConnectingCount][0] = row;
    winningChips[chipsConnectingCount][1] = column;

    
    if(chipsConnectingCount >= 3){
        return true;
    }
    
    if(row == 0){
        chipsConnectingCount = 0;
        for(unsigned int i = 0; i < 4; i++){
            winningChips[i][0] = 0;
            winningChips[i][1] = 0;
        }

        return false;
    }
    
    if(column == 1){
        chipsConnectingCount = 0;
        for(unsigned int i = 0; i < 4; i++){
            winningChips[i][0] = 0;
            winningChips[i][1] = 0;
        }

        return false;
    }
   
    return recursiveLeftDiagnalCheck(currGameState, colorCheck, row - 1, column - 1, chipsConnectingCount);
}

void fixGameBoardForChecking(int (&gameArray)[8][8], int oldArray[8][8]){

    for(unsigned int row = 0; row < 8; row++){
       for (unsigned int column = 0; column < 8; column++){

            gameArray[row][column] = oldArray[row][row%2 == 0 ? 7-column: column];
       }
    }
}

bool checkGameWon(bool playerOneTurn, int gameState[8][8]){
    bool playerToCheck = !playerOneTurn;
    int chipNumToCheck = playerToCheck ? R : B;
    int connectingChips = 0;
    int  currGameState[8][8];
    bool gameWasWon = false;
    fixGameBoardForChecking(currGameState, gameState);

    for(unsigned int row = 6; row > 0; row--){
        for (unsigned int column = 1; column < 7; column++) {

            if(currGameState[row][column] == chipNumToCheck){
                //CHECK ALL THE CHIPS CONNECTING IT
                winningChips[connectingChips][0] = row;
                winningChips[connectingChips][1] = column;

                gameWasWon = recursiveRowCheck(currGameState, chipNumToCheck, row, column + 1, connectingChips);
                if(gameWasWon){
                    return true;
                }

                winningChips[connectingChips][0] = row;
                winningChips[connectingChips][1] = column;

                gameWasWon = recursiveColumnCheck(currGameState, chipNumToCheck, row - 1, column, connectingChips);
                if(gameWasWon){
                    return true;
                }

                winningChips[connectingChips][0] = row;
                winningChips[connectingChips][1] = column;
               
                gameWasWon = recursiveRightDiagnalCheck(currGameState, chipNumToCheck, row - 1, column + 1, connectingChips);
                if(gameWasWon){
                    return true;
                }

                winningChips[connectingChips][0] = row;
                winningChips[connectingChips][1] = column;

                gameWasWon = recursiveLeftDiagnalCheck(currGameState, chipNumToCheck, row - 1, column - 1, connectingChips);
                if(gameWasWon){
                    return true;
                }

            } 
        }
    }
    
    return false;
}

long map(long x, long in_min, long in_max, long out_min, long out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void D2toD1Array(int (&D2Array)[8][8], int (&D1Array)[64]){
    for (unsigned int row = 0; row < 8; row++){
        for (unsigned int column = 0; column < 8; column++){
           D1Array[(row * 7) + (row + column)] = D2Array[row][column];
        }
    }

}

void resetGame(){
    gameStarted = true;
    for (unsigned int row = 0; row < 8; row++){
        for (unsigned int column = 0; column < 8; column++){
            if (column == 0 || column == 7 || row == 0 || row == 7){
                gameBoard2D[row][column] = W;
            } else if (row == 1 && column > 0 && column < 7){
               gameBoard2D[row][column] = O;
            } else {
                gameBoard2D[row][column] = O;
            }
        }
    }

    for(int i = 0; i < 7; i++){
        if(i == 0 || i == 7){
            rowsFilledStatus[i] = 8;
        } else {
            rowsFilledStatus[i] = 6;
        }
    }

    D2toD1Array(gameBoard2D, gameBoardLEDMatrix);
   // chipsDropped = 0;
    gameStarted = true;
}

void clearGameBoard(){
    for(unsigned int row = 0; row < 8; row++){
       for (unsigned int column = 0; column < 8; column++){
            gameBoard2D[row][column] = O;
       }
    }

    D2toD1Array(gameBoard2D, gameBoardLEDMatrix);
}

void blinkChip(){
    static int showChip = false;
    bool shouldShow = showChip && !gameWon && gameStarted;

    
    for(unsigned int column = 1; column < 7; column++){
        if(column == chipPosition && shouldShow){
            gameBoard2D[0][7 - column] = player1Turn ? R : B;
        } else {
            gameBoard2D[0][7 - column] = O;
        }
    }

    if(gameWon and gameStarted){
        for(unsigned int row = 0; row < 8; row++){
           for (unsigned int column = 0; column < 8; column++){
                for(unsigned int i = 0; i < 4; i++){
                    if(row == winningChips[i][0] && column == winningChips[i][1]){
                        if(showChip){
                            gameBoard2D[row][row%2 == 0 ? 7 - column : column] = player1Turn ? B : R; //OPPOSITE BECAUSE THE PLAYER GETS SWICTED AS SOON AS THE CHIP GETS DROPPED
                        } else {
                            gameBoard2D[row][row%2 == 0 ? 7 - column : column] = O;
                        }
                    }
                }
           }
        }
    }

    D2toD1Array(gameBoard2D, gameBoardLEDMatrix);
    showChip = !showChip;

}

void setScoreBoard(int (&scoreBoard)[8][8],int (&scoreMatrix)[64] ,unsigned char score, int playerNum){

    //CLEAR OUT THE MATRIX FIRST
    for (unsigned int row = 0; row < 8; row++){
        for (unsigned int column = 0; column < 8; column++){
           scoreMatrix[(row * 7) + (row + column)] = O;
           scoreBoard[row][column] = O;
        }
    }

    
    // UPDATE THE MATRIX BASED ON SCORE
    switch (score)
    {
    case 0:
        for (unsigned int row = 0; row < 8; row++){
            for (unsigned int column = 0; column < 8; column++){
                if (row == 0 || row == 7){
                    if(column >= 2 && column <= 5){
                        scoreBoard[row][column] = playerNum == 1 ? R : B;
                    } else {
                        scoreBoard[row][column] = O;
                    }
                    
                } else {
                    if(column == 2 || column == 5){
                        scoreBoard[row][column] = playerNum == 1 ? R : B;
                    } else {
                        scoreBoard[row][column] = O;
                    }
                }
        }
    }
        break;

    case 1:
        for (unsigned int row = 0; row < 8; row++){
            for (unsigned int column = 0; column < 8; column++){
                if(row%2 == 0){
                    if(row == 2 && column == 5){
                        scoreBoard[row][column] = playerNum == 1 ? R : B;
                    } 

                    if(column == 3){
                        scoreBoard[row][column] = playerNum == 1 ? R : B;
                    }
                    
                } else if (row%2 != 0){
                    if(row == 1 && column == 3){
                        scoreBoard[row][column] = playerNum == 1 ? R : B;
                    } 

                    if(column == 4){
                        scoreBoard[row][column] = playerNum == 1 ? R : B;
                    }
                } else {
                    scoreBoard[row][column] = O; 
                }
            }
        }
        break;

    case 2: 
        for(unsigned int row = 0; row < 8; row++){
            for (unsigned int column = 0; column < 8; column++){
               if((row == 0)){
                    if(column >= 2 && column <= 5){
                        scoreBoard[row][column] = playerNum == 1 ? R : B;
                    } else {
                        scoreBoard[row][column] = O; 
                    }
                    
               } else if (row >= 1 and row <= 2){
                    int columnNum = row % 2 == 0 ? 2 : 5;
                    if(column == columnNum){
                        scoreBoard[row][column] = playerNum == 1 ? R : B;
                    } else {
                        scoreBoard[row][column] = O; 
                    }
               } else if (row == 3){
                    if(column >= 2 && column <= 5){
                        scoreBoard[row][column] = playerNum == 1 ? R : B;
                    } else {
                        scoreBoard[row][column] = O; 
                    }

               } else if (row >= 4 and row <= 5){
                    int columnNum = row % 2 == 0 ? 5 : 2;
                    if(column == columnNum){
                        scoreBoard[row][column] = playerNum == 1 ? R : B;
                    } else {
                        scoreBoard[row][column] = O; 
                    }
               } else if (row == 6){
                    if(column >= 2 && column <= 5){
                        scoreBoard[row][column] = playerNum == 1 ? R : B;
                    } else {
                        scoreBoard[row][column] = O; 
                    }
               } else {
                    scoreBoard[row][column] = O; 
               }
            }
        }
        break;

        case 3: 
        for(unsigned int row = 0; row < 8; row++){
            for (unsigned int column = 0; column < 8; column++){
               if((row == 0)){
                    if(column >= 2 && column <= 5){
                        scoreBoard[row][column] = playerNum == 1 ? R : B;
                    } else {
                        scoreBoard[row][column] = O; 
                    }
                    
               } else if (row >= 1 and row <= 2){
                    int columnNum = row % 2 == 0 ? 2 : 5;
                    if(column == columnNum){
                        scoreBoard[row][column] = playerNum == 1 ? R : B;
                    } else {
                        scoreBoard[row][column] = O; 
                    }
               } else if (row == 3){
                    if(column >= 2 && column <= 5){
                        scoreBoard[row][column] = playerNum == 1 ? R : B;
                    } else {
                        scoreBoard[row][column] = O; 
                    }

               } else if (row >= 4 and row <= 5){
                    int columnNum = row % 2 == 0 ? 2 : 5;
                    if(column == columnNum){
                        scoreBoard[row][column] = playerNum == 1 ? R : B;
                    } else {
                        scoreBoard[row][column] = O; 
                    }
               } else if (row == 6){
                    if(column >= 2 && column <= 5){
                        scoreBoard[row][column] = playerNum == 1 ? R : B;
                    } else {
                        scoreBoard[row][column] = O; 
                    }
               } else {
                    scoreBoard[row][column] = O; 
               }
            }
        }
        break;

    case 4:
        for(unsigned int row = 0; row < 8; row++){
           for (unsigned int column = 0; column < 8; column++){
                if(row == 3){
                    if(column >= 2 && column <= 5){
                        scoreBoard[row][column] = playerNum == 1 ? R : B;
                    } else {
                        scoreBoard[row][column] = O;
                    }
                } else {
                    int columnNum = row % 2 == 0 ? 5 : 2;
                    if(row <= 2){
                        if(column == columnNum){
                            scoreBoard[row][column] = playerNum == 1 ? R : B;
                        } 
                    }
                    columnNum = row % 2 == 0 ? 2 : 5;
                     if(column == columnNum){
                        scoreBoard[row][column] = playerNum == 1 ? R : B;
                     }
                }
           }
        }
        break;

    case 5: 
        for(unsigned int row = 0; row < 8; row++){
            for (unsigned int column = 0; column < 8; column++){
               if((row == 0)){
                    if(column >= 2 && column <= 5){
                        scoreBoard[row][column] = playerNum == 1 ? R : B;
                    } else {
                        scoreBoard[row][column] = O; 
                    }
                    
               } else if (row >= 1 and row <= 2){
                    int columnNum = row % 2 == 0 ? 5 : 2;
                    if(column == columnNum){
                        scoreBoard[row][column] = playerNum == 1 ? R : B;
                    } else {
                        scoreBoard[row][column] = O; 
                    }
               } else if (row == 3){
                    if(column >= 2 && column <= 5){
                        scoreBoard[row][column] = playerNum == 1 ? R : B;
                    } else {
                        scoreBoard[row][column] = O; 
                    }

               } else if (row >= 4 and row <= 5){
                    int columnNum = row % 2 == 0 ? 2 : 5;
                    if(column == columnNum){
                        scoreBoard[row][column] = playerNum == 1 ? R : B;
                    } else {
                        scoreBoard[row][column] = O; 
                    }
               } else if (row == 6){
                    if(column >= 2 && column <= 5){
                        scoreBoard[row][column] = playerNum == 1 ? R : B;
                    } else {
                        scoreBoard[row][column] = O; 
                    }
               } else {
                    scoreBoard[row][column] = O; 
               }
            }
        }
        break;

    case 6:
        for(unsigned int row = 0; row < 8; row++){
           for (unsigned int column = 0; column < 8; column++){
                if(row == 0){
                    if(column >=2 && column <= 5){
                        scoreBoard[row][column] = playerNum == 1 ? R : B;
                    } else {
                        scoreBoard[row][column] = O; 
                    }
                } else if (row >=1 && row <= 3){
                    int columnNum = row % 2 == 0 ? 5 : 2;
                    if(column == columnNum){
                        scoreBoard[row][column] = playerNum == 1 ? R : B;
                    } else {
                        scoreBoard[row][column] = O; 
                    }
                } else if (row == 4){
                    if(column >=2 && column <= 5){
                        scoreBoard[row][column] = playerNum == 1 ? R : B;
                    } else {
                        scoreBoard[row][column] = O; 
                    }

                } else if (row == 5 || row == 6){
                    if(column == 5 || column == 2){
                         scoreBoard[row][column] = playerNum == 1 ? R : B;
                    } else {
                        scoreBoard[row][column] = O; 
                    }
                } else if (row == 7){
                    if (column >= 2 && column <= 5){
                        scoreBoard[row][column] = playerNum == 1 ? R : B;
                    } else {
                        scoreBoard[row][column] = O; 
                    }
                } else {
                        scoreBoard[row][column] = O; 
                }
           }
        }
        break;

    case 7:
        for(unsigned int row = 0; row < 8; row++){
           for (unsigned int column = 0; column < 8; column++){
                if(row == 0){
                    if(column >= 2 && column <= 5){
                        scoreBoard[row][column] = playerNum == 1 ? R : B;
                    } else {
                        scoreBoard[row][column] = O;
                    }
                } else{
                    int columnNum = row % 2 == 0 ? 2 : 5;
                    if(column == columnNum){
                        scoreBoard[row][column] = playerNum == 1 ? R : B;
                    } else {
                        scoreBoard[row][column] = O;
                    }
                }
           }
        }
        break;

        case 8:
            for(unsigned int row = 0; row < 8; row++){
               for (unsigned int column = 0; column < 8; column++){
                    if(row == 0){
                        if(column >=2 && column <= 5){
                            scoreBoard[row][column] = playerNum == 1 ? R : B;
                        } else {
                            scoreBoard[row][column] = O; 
                        }
                    } else if (row >=1 && row <= 3){
                        if(column == 5 || column == 2){
                             scoreBoard[row][column] = playerNum == 1 ? R : B;
                        } else {
                            scoreBoard[row][column] = O; 
                        }

                    } else if (row == 4){
                        if(column >=2 && column <= 5){
                            scoreBoard[row][column] = playerNum == 1 ? R : B;
                        } else {
                            scoreBoard[row][column] = O; 
                        }

                    } else if (row == 5 || row == 6){
                        if(column == 5 || column == 2){
                             scoreBoard[row][column] = playerNum == 1 ? R : B;
                        } else {
                            scoreBoard[row][column] = O; 
                        }
                    } else if (row == 7){
                        if (column >= 2 && column <= 5){
                            scoreBoard[row][column] = playerNum == 1 ? R : B;
                        } else {
                            scoreBoard[row][column] = O; 
                        }
                    } else {
                            scoreBoard[row][column] = O; 
                    }
               }
            }
            break;

    case 9:
            for(unsigned int row = 0; row < 8; row++){
               for (unsigned int column = 0; column < 8; column++){
                    if(row == 0){
                        if(column >=2 && column <= 5){
                            scoreBoard[row][column] = playerNum == 1 ? R : B;
                        } else {
                            scoreBoard[row][column] = O; 
                        }
                    } else if (row >=1 && row <= 2){
                        if(column == 5 || column == 2){
                             scoreBoard[row][column] = playerNum == 1 ? R : B;
                        } else {
                            scoreBoard[row][column] = O; 
                        }
    
                    } else if (row == 3){
                        if(column >=2 && column <= 5){
                            scoreBoard[row][column] = playerNum == 1 ? R : B;
                        } else {
                            scoreBoard[row][column] = O; 
                        }
    
                    } else if (row >= 4 && row <= 6){
                        int columnNum = row % 2 == 0 ? 2 : 5;
                        if(column == columnNum){
                             scoreBoard[row][column] = playerNum == 1 ? R : B;
                        } else {
                            scoreBoard[row][column] = O; 
                        }
                    } else if (row == 7){
                        if (column >= 2 && column <= 5){
                            scoreBoard[row][column] = playerNum == 1 ? R : B;
                        } else {
                            scoreBoard[row][column] = O; 
                        }
                    } else {
                            scoreBoard[row][column] = O; 
                    }
               }
            }
            break;



    default:
        break;
    }

    D2toD1Array(scoreBoard, scoreMatrix);
}

void showMessage(){
    static int message = 0;
    Serial.print("Showing message");
    Serial.println(message);

    if(message < 10){
            clearGameBoard();
            for(unsigned int row = 0; row < 8; row++){
                for (unsigned int column = 0; column < 8; column++){
                    //P
                    if(column == 0){
                        if(row > 2 && row < 5){
                            gameBoard2D[row][row % 2 == 0? 7 - column : column] = R;   
                        }
                    }
                    if(column < 3){
                        if(row > 0 && row < 3){
                            gameBoard2D[row][row % 2 == 0? 7 - column : column] = R;   
                        }
                    }  
                    // R
                    if(column == 4){
                        if(row > 2 && row < 5){
                            gameBoard2D[row][row % 2 == 0? 7 - column : column] = R;   
                        }
                    }
                    if(column > 3 && column < 7){
                        if(row > 0 && row < 3){
                            gameBoard2D[row][row % 2 == 0? 7 - column : column] = R;   
                        }

                        if(row == 3 && column == 5){
                            gameBoard2D[row][row % 2 == 0? 7 - column : column] = R;
                        }

                        if(row == 4 && column == 6){
                            gameBoard2D[row][row % 2 == 0? 7 - column : column] = R;
                        }

                    }   
 
            }
        }
            message++;
    }
    
    if(message >= 10){
        clearGameBoard();
        for(unsigned int row = 0; row < 8; row++){
           for (unsigned int column = 0; column < 8; column++){
                    //B
                    if(column == 0 || column == 2){
                        if(row > 1 && row < 6){
                            gameBoard2D[row][row % 2 == 0? 7 - column : column] = R;   
                        }
                    }
                    if(column < 3){
                        if(row % 2 == 1 && row < 6){
                            gameBoard2D[row][row % 2 == 0? 7 - column : column] = R;   
                        }
                    }  

                //T
                    if(column > 3){
                        if(row == 1){
                            gameBoard2D[row][row % 2 == 0? 7 - column : column] = R;  
                        } 
                    }

                    if(column == 5 || column == 6){
                        if(row > 0 && row < 6){
                            gameBoard2D[row][row % 2 == 0? 7 - column : column] = R; 
                        }
                    }
           }
        }
        message++;
        if(message > 20){
             message = 0;
        }
    }

    D2toD1Array(gameBoard2D, gameBoardLEDMatrix);
}

void dropChip(){
    int column = chipPosition;
    int tempRow = 6;

    int row = rowsFilledStatus[chipPosition];
    column = row % 2 == 0 ? 7 - column : column;

    if(!gameWon){
        //scoreBoard[row][column] = playerNum == 1 ? R : B;
        if(row > 0){
            gameBoard2D[row][column] = player1Turn == true ? R : B;
            player1Turn = !player1Turn;
            rowsFilledStatus[chipPosition] = rowsFilledStatus[chipPosition] - 1; 
            chipsDropped++;
        }
    }
}

CRGB getLEDColor(int colorNum){
        switch (colorNum)
            {
            case O:
                return CRGB::Black;
                break;

            case R:
                return CRGB::Red;
                break;

            case B:
                return CRGB::Blue;
                break;

            case W:
                return CRGB::White;
                break;

            default:
                return CRGB::Violet;
                break;
        }

}

void setChipPosition(){
    chipPosition = (Bit0 * 1) + (Bit1 * 2) + (Bit2 * 4);
}

uint8_t newMAC[] = {0xAC, 0x49, 0xDB, 0x79, 0xF4, 0x5E};

void setup() {
    FastLED.addLeds<WS2812, DATA_PIN, GRB>(leds, NUM_LEDS);
    Serial.begin(9600);

    //MAKE EVERYTHING OFF AT THE START OF THE GAME
    for (unsigned int row = 0; row < NUM_LEDS; row++){
        leds[row] = CRGB::Black; 
    }

    FastLED.show();

    pinMode(Bit0PIN, INPUT);
    pinMode(Bit1PIN, INPUT);
    pinMode(Bit2PIN, INPUT);
    pinMode(BTNPIN, INPUT);
    pinMode(PLYRTRNPIN, OUTPUT);
    pinMode(GAMESTATUSPIN, OUTPUT);
    pinMode(SYSSTATUSRECIEVEPIN, INPUT);
    pinMode(GAMESTATUSRECIEVEPIN, INPUT);
    pinMode(GAMEWONARDUINOPIN, OUTPUT);
    pinMode(PLYRTURNARDUINOPIN, OUTPUT);
    TimerSet(GCD_PERIOD);
    TimerOn();

    tasks[0].period = BOARD_PERIOD;
    tasks[0].state = BOARDINIT;
    tasks[0].elapsedTime = tasks[0].period;
    tasks[0].TickFct = BoardTick;

    tasks[1].period = JYSTCK_PERIOD;
    tasks[1].state = JYSTCKINIT;
    tasks[1].elapsedTime = tasks[1].period;
    tasks[1].TickFct = JYSTCKTick;

    tasks[2].period = BTN_PERIOD;
    tasks[2].state = BTNINIT;
    tasks[2].elapsedTime = tasks[2].period;
    tasks[2].TickFct = BTNTick;

    tasks[3].period = PLYR_TRN_SEND_PERIOD;
    tasks[3].state = PLYRTRNSENDINIT;
    tasks[3].elapsedTime = tasks[3].period;
    tasks[3].TickFct = PLYRTRNSendTick;

    tasks[4].period = GAME_WON_CHECK_PERIOD;
    tasks[4].state = GAMEWONSTATINIT;
    tasks[4].elapsedTime = tasks[4].period;
    tasks[4].TickFct = GameWonCheckTick;

    tasks[5].period = UPDATE_GAME_BOARD_PERIOD;
    tasks[5].state = UPDATEGAMEBOARDINIT;
    tasks[5].elapsedTime = tasks[5].period;
    tasks[5].TickFct = UpdateGameBoardTick;

    tasks[6].period = GAME_STATUS_SEND_PERIOD;
    tasks[6].state = GAMESTATUSSENDINIT;
    tasks[6].elapsedTime = tasks[6].period;
    tasks[6].TickFct = GameStatusSendTick;

    tasks[7].period = SYSTEM_STATUS_READ_PERIOD;
    tasks[7].state = SYSTEMSTATUSREADINIT;
    tasks[7].elapsedTime = tasks[7].period;
    tasks[7].TickFct = SystemStatusReadTick;

    tasks[8].period = RESET_BTN_PERIOD;
    tasks[8].state = RESETBTNINIT;
    tasks[8].elapsedTime = tasks[8].period;
    tasks[8].TickFct = ResetBTNTick;

}

void loop() {
 if (TimerFlag) {
        TimerFlag = 0;  // Reset TimerFlag
        TimerISR();         // Call ISR to handle tasks
    }
}

int BoardTick(int state){
    static unsigned char currentScore = 3;
    static int count = 0;
    switch (state)
    {
    case BOARDINIT:
        state = BOARDOFF;
        break;
    case BOARDOFF:
        FastLED.show();
        if(systemIsOn){
            state = BOARDUPDATE;
        } else {
            state = BOARDOFF;
        }
        break;
    case BOARDON:
        FastLED.setBrightness(25);
        FastLED.show();
        if(systemIsOn){
            state = BOARDUPDATE;
        } else {
            state = BOARDOFF;
        }
        break;
    case BOARDUPDATE:
        for(unsigned int i = 0; i < 128; i++ ){
            leds[i] = CRGB::Black;
        }

        if(systemIsOn){
            setScoreBoard(player2Score2D, player2ScoreLEDMatrix,player2Score, 2);
            for(unsigned int i = 0; i < 64; i++ ){
                leds[i] = getLEDColor(player2ScoreLEDMatrix[i]);
            }

            blinkChip();
            for(unsigned int i = 0; i < 64; i++ ){
                leds[64 + i] = getLEDColor(gameBoardLEDMatrix[i]);
            }


            setScoreBoard(player1Score2D, player1ScoreLEDMatrix,player1Score, 1);
            for(unsigned int i = 0; i < 64; i++ ){
                leds[128 + i] = getLEDColor(player1ScoreLEDMatrix[i]);
            }
            state = BOARDON;
        } else {
            state = BOARDOFF;
        }
        break;
    default:
        state = BOARDINIT;
        break;
    }

    switch (state)
    {
    case BOARDINIT:
        break;
    case BOARDOFF:
        for (unsigned int row = 0; row < NUM_LEDS; row++){
            leds[row] = CRGB::Black; 
        }
        break;
    case BOARDON:
        break;
    case BOARDUPDATE:
        break;
    default:
        break;
    }
    return state;
}

int JYSTCKTick(int state){
    switch (state)
    {
    case JYSTCKINIT:
        state = JYSTCKOFF;
        break;
    case JYSTCKOFF:
        
        if(gameStarted){
            state = JYSTCKON;
        } else {
            state = JYSTCKOFF;
        }
    case JYSTCKON:
        if(gameStarted){
            state = JYSTCKON;
        } else {
            state = JYSTCKOFF;
        }

    default:
        break;
    }

    switch (state)
    {
    case JYSTCKINIT:
        break;

    case JYSTCKOFF:
        break;

    case JYSTCKON:
        Bit0 = digitalRead(Bit0PIN);
        Bit1 = digitalRead(Bit1PIN);
        Bit2 = digitalRead(Bit2PIN);
        setChipPosition();
        break;
    
    default:
        break;
    }

    return state;
}

int BTNTick(int state){
    int P19 = digitalRead(BTNPIN);

    switch (state)
    {
    case BTNINIT:
        state = BTNWAIT;
        break;
    case BTNWAIT:
        if(P19){
            dropChipBtnCount++;
            state = BTNPRESS;
        } else{
            state = BTNWAIT;
        }
        break;
    case BTNPRESS:
        if(P19){
            state = BTNPRESS;
        } else{
            state = BTNWAIT;
        }

        break;
    default:
        break;
    }

    switch (state)
    {
    case BTNINIT:
        break;
    case BTNWAIT:
        break;
    case BTNPRESS:
        break;
    default:
        break;
    }

    return state;
}

int PLYRTRNSendTick(int state){
    switch (state)
    {
    case PLYRTRNSENDINIT:
        state = PLYRTRNSENDON;
        break;
    case PLYRTRNSENDON:
        state = PLYRTRNSENDON;
        break;
    default:
        state = PLYRTRNSENDINIT;
        break;
    }

    switch (state)
    {
    case PLYRTRNSENDINIT:
        break;
    case PLYRTRNSENDON:
        digitalWrite(PLYRTRNPIN, player1Turn == true ? 1 : 0);
        digitalWrite(PLYRTURNARDUINOPIN, player1Turn == true ? 1 : 0);
        break;
    default:
        break;
    }

    return state;
}

int GameWonCheckTick(int state){
    static bool updatedStatus = false;
    static int totalChipCount = 0;
    static int gameResetCount = 0;
    switch (state)
    {
    case GAMEWONSTATINIT:
        state = GAMEWONSTATCHECK;
        break;

    case GAMEWONSTATCHECK:
        if(gameResetCount != resetCount || !systemIsOn){
            gameResetCount = resetCount;
            updatedStatus = false;
            gameWon = false;
        }
        if(totalChipCount != chipsDropped){
            totalChipCount = chipsDropped;
            if(totalChipCount > 6){
                gameWon = checkGameWon(player1Turn, gameBoard2D);
                if(gameWon && !updatedStatus){
                    if(totalChipCount < 12){
                        delay(200);
                    }
                    if(player1Turn){
                        player2Score++;
                    } else {
                        player1Score++;
                    }
                    updatedStatus = true;
                }
            } else {
                gameWon = false;
            }
        } 

        state = GAMEWONSTATCHECK;
        break;

    default:
        state = GAMEWONSTATINIT;
        break;
    }

    switch (state)
    {
    case GAMEWONSTATINIT:
        break;

    case GAMEWONSTATCHECK:
        break;
        
    default:
        break;
    }

    return state;
}

int UpdateGameBoardTick(int state){
    static int xBtnPressCount = 0;
    static int gameResetLocalCount = 0;
    static int showMessageBool = false;
    switch (state)
    {
    case UPDATEGAMEBOARDINIT:
        state = UPDATEGAMEBOARDON;
        break;
    case UPDATEGAMEBOARDON:
        if(xBtnPressCount != dropChipBtnCount){
            xBtnPressCount = dropChipBtnCount;
            dropChip();
        }

        if(!systemIsOn){
            clearGameBoard();
            showMessageBool = true;
        }

        if(systemIsOn && gameResetLocalCount == resetCount && showMessageBool){
            showMessage();
        }

        if(gameResetLocalCount != resetCount){
            showMessageBool = false;
            gameResetLocalCount = resetCount;
            resetGame();
        }

        state = UPDATEGAMEBOARDON;    
        break;
    default:
        break;
    }

    switch (state)
    {
    case UPDATEGAMEBOARDINIT:
        break;
    case UPDATEGAMEBOARDON:
        break;
    default:
        break;
    }
    return state;

}

//CREATE A NEW TICK TO SEND DATA ABOUT GAMESTARTED TO PS4 AND UNO CODE
int GameStatusSendTick(int state){
    bool ps4Data = 0;
    switch (state)
    {
    case GAMESTATUSSENDINIT:
        state = GAMESTATUSSENDON;
        break;
    case GAMESTATUSSENDON:
        state = GAMESTATUSSENDON;
        break;
    default:
        state = PLYRTRNSENDINIT;
        break;
    }

    switch (state)
    {
    case GAMESTATUSSENDINIT:
        break;
    case GAMESTATUSSENDON:
        if(gameWon){
            ps4Data = 0;
        } else {
            ps4Data = gameStarted == true ? 1 : 0;
        }
        digitalWrite(GAMESTATUSPIN, ps4Data);
        digitalWrite(GAMEWONARDUINOPIN, gameWon == true ? 1 : 0);
        break;
    default:
        break;
    }

    return state;
}

int SystemStatusReadTick(int state){
    int PIN21Data = digitalRead(SYSSTATUSRECIEVEPIN);
    static bool wasCaught = false;
    static bool resetStats = true;
    switch (state)
    {
    case SYSTEMSTATUSREADINIT:
        state = SYSTEMSTATUSREADON;
        break;
    case SYSTEMSTATUSREADON:
        systemIsOn = PIN21Data == 1 ? true : false;
        break;
    default:
        state = SYSTEMSTATUSREADINIT;
        break;
    }

    switch (state)
    {
    case SYSTEMSTATUSREADINIT:
        break;
    case SYSTEMSTATUSREADON:
        break;
    default:
        break;
    }

    return state;
}

int ResetBTNTick(int state){
    int PIN22Data = digitalRead(GAMESTATUSRECIEVEPIN);
    switch (state)
    {
    case RESETBTNINIT:
        state = RESETBTNWAIT;
        break;
    case RESETBTNWAIT:
        if(PIN22Data){
            state = RESETBTNWAIT;
        } else {
            gameStarted = false;
            state = RESETBTNPRESS;
        }

        if(!systemIsOn){
            gameStarted = false;
        }
        break;
    case RESETBTNPRESS:
        if(PIN22Data){
            gameStarted = true;
            resetCount++;
            state = RESETBTNWAIT;
        } else {
            state = RESETBTNPRESS;
        }

        break;
    default:
        break;
    }

    switch (state)
    {
    case RESETBTNINIT:
        break;
    case RESETBTNWAIT:
        break;
    case RESETBTNPRESS:
        break;
    default:
        break;
    }
    return state;
}
