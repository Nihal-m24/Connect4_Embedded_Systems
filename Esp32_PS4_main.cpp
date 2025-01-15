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
#include <PS4Controller.h>
#include <WiFi.h>

#define NUM_TASKS 1 //TODO: Change to the number of tasks being used

//Defin the pins being used to input and output
const int Bit0PIN = 16;  //OUTPUT
const int Bit1PIN = 17; //OUTPUT
const int Bit2PIN = 18; //OUTPUT
const int BTNPIN = 19; //OUTPUT
const int PLYRTRNPIN = 21; // INPUT
const int GAMESTATUSPIN = 22; //INPUT

//GLOBAL VARIABLES
bool player1Turn = true;
bool gameStarted = true;
int chipPosition = 0;
int fullRows[7] = {-1, -1, -1, -1, -1, -1, -1};
int Bit0 = 0;
int Bit1 = 0;
int Bit2 = 0;


typedef struct _task{
	signed 	 char state; 		//Task's current state
	unsigned long period; 		//Task period
	unsigned long elapsedTime; 	//Time elapsed since last task tick
	int (*TickFct)(int); 		//Task tick function
} task;

const unsigned long GCD_PERIOD = 200;//TODO:Set the GCD Period

task tasks[NUM_TASKS]; 

enum JoystickStates {JOYSTICKINIT, JOYSTICKOFF, JOYSTICKREAD} JoystickState;
int JoystickTick(int state);

enum LEDStates {LEDINIT, LEDOFF, LEDON} LEDState;
int LEDTick(int state);

enum TransferPositionStates {TPINIT, TPOFF, TPON} TransferPositionState;
int TransferPositiionTick(int state);

enum BTNStates {BTNINIT, BTNON} BTNState;
int BtnTick(int state);

enum PLYRTRNReadStates {PLYRTRNREADINIT, PLYRTRNREADON} PLYRTRNReadState;
int PLYRTRNReadTick(int state);

enum GameStatusReadStates {GAMESTATUSREADINIT, GAMESTATUSREADON} GameStatusReadState;
int GameStatusReadTick(int state);

void TimerISR() {
	for ( unsigned int i = 0; i < NUM_TASKS; i++ ) {                   // Iterate through each task in the task array
		if ( tasks[i].elapsedTime == tasks[i].period ) {           // Check if the task is ready to tick
			tasks[i].state = tasks[i].TickFct(tasks[i].state); // Tick and set the next state for this task
			tasks[i].elapsedTime = 0;                          // Reset the elapsed time for the next tick
		}
		tasks[i].elapsedTime += GCD_PERIOD;                        // Increment the elapsed time by GCD_PERIOD
	}
}

int map(int8_t x) {
    if(x <= -85){
        return -1;
    } else if (x > -85 && x <= 85){
        return 0;
    } else {
       return 1;
    }
}

uint8_t newMAC[] = {0xAC, 0x49, 0xDB, 0x79, 0xF4, 0x5E};

int curJoystickState = JOYSTICKINIT;
int curLEDState = LEDINIT;
int currTransferPositionState = TPINIT;
int currBTNState = BTNINIT;
int currPLYRTRNReadState = PLYRTRNREADINIT;
int currGameStatusReadState = GAMESTATUSREADINIT;

void setup() {
    Serial.begin(9600);

    if (esp_base_mac_addr_set(newMAC) == ESP_OK) {
    } else {
    }

    pinMode(Bit0PIN, OUTPUT);
    pinMode(Bit1PIN, OUTPUT);
    pinMode(Bit2PIN, OUTPUT);
    pinMode(BTNPIN, OUTPUT);
    pinMode(PLYRTRNPIN, INPUT);
    pinMode(GAMESTATUSPIN, INPUT);

    PS4.begin("AC:49:DB:79:F4:60");
   // removePairedDevices();
}

void loop() {
    curJoystickState = JoystickTick(curJoystickState);
    curLEDState = LEDTick(curLEDState);
    currTransferPositionState = TransferPositiionTick(currTransferPositionState);
    currBTNState = BtnTick(currBTNState);
    currPLYRTRNReadState = PLYRTRNReadTick(currPLYRTRNReadState);
    currGameStatusReadState = GameStatusReadTick(currGameStatusReadState);
    delay(GCD_PERIOD);
}

void setBits(){
    int chipVal = chipPosition;

    Bit0 = chipVal % 2;
    chipVal = chipVal - Bit0;
    chipVal = chipVal / 2;

    Bit1 = chipVal % 2;
    chipVal = chipVal - Bit1;
    chipVal = chipVal / 2;

    Bit2 = chipVal % 2;
    chipVal = chipVal - Bit2;
    chipVal = chipVal / 2;
}

void moveChip(int direction){
    int currentChipPosition = chipPosition;

    if(direction == 1){
        currentChipPosition++;
    } else if (direction == -1)
    {
       currentChipPosition += -1;
    } else {
       currentChipPosition += 0;
    }

    if(currentChipPosition < 1){
        currentChipPosition = 6;
    } else if (currentChipPosition > 6){
        currentChipPosition = 1;
    }

    bool placedChip = false;

    while (!placedChip)
    {
        for(unsigned int i = 0; i < 8; i++){
            if (fullRows[i] != currentChipPosition){
                placedChip = true;
                break;
            }
        }

        if(!placedChip){
            currentChipPosition++;
        }
    }

    if(!gameStarted){
        currentChipPosition = 0;
    }
    
    chipPosition = currentChipPosition;
}

int JoystickTick(int state){
    int8_t rightJYSTCK = 0;
    int mappedVal = 0;
    switch (state)
    {
    case JOYSTICKINIT:
        state = JOYSTICKOFF;
        break;
    case JOYSTICKOFF:
        if(PS4.isConnected()){
            state = JOYSTICKREAD;
        } else {
            state = JOYSTICKOFF;
        }
        break;
    case JOYSTICKREAD:
        if(PS4.isConnected()){
            state = JOYSTICKREAD;
        } else {
            state = JOYSTICKOFF;
        }

        break;
    default:
        break;
    }

    switch (state)
    {
    case JOYSTICKINIT:
        break;
    case JOYSTICKOFF:
        break;
    case JOYSTICKREAD:
        rightJYSTCK = PS4.RStickX();
        mappedVal = map(rightJYSTCK);
        if(mappedVal != 0){
            moveChip(mappedVal);
        }
        break;
    default:
        break;
    }

    return state;
}

int LEDTick(int state){
    switch (state)
    {
    case LEDINIT:
        state = LEDOFF;
        break;
    case LEDOFF:
        if(PS4.isConnected()){
            state = LEDON;
        } else {
            state = LEDOFF;
        }
        break;
    case LEDON:
        if(PS4.isConnected()){
            state = LEDON;
        } else {
            state = LEDOFF;
        }
        break;
    default:
        state = LEDINIT;
        break;
    }

    switch (state)
    {
    case LEDINIT:
        break;
    case LEDOFF:
        break;
    case LEDON:
        if(gameStarted){
            if(player1Turn){
                 PS4.setLed(255, 0, 0);
            } else {
                PS4.setLed(0, 0, 255);
            }
        } else {
            PS4.setLed(0, 255, 0);
        }
        PS4.sendToController();
        break;
    default:
        break;
    }
    return state;
}

int TransferPositiionTick(int state){
    switch (state)
    {
    case TPINIT:
        state = TPOFF;
        break;
    case TPOFF:
        if(gameStarted){
            setBits();
            state = TPON;
        } else {
            state = TPOFF;
        }
        break;
    case TPON:
        if(gameStarted){
            setBits();
            state = TPON;
        } else {
            state = TPOFF;
        }

        break;
    default:
        state = TPINIT;
        break;
    }

    switch (state)
    {
    case TPINIT:
        break;
    case TPOFF:
        break;
    case TPON:
        digitalWrite(Bit0PIN, Bit0);
        digitalWrite(Bit1PIN, Bit1);
        digitalWrite(Bit2PIN, Bit2);
        break;
    default:
        break;
    }
    return state;
}

int BtnTick(int state){
    switch (state)
    {
    case BTNINIT:
        state = BTNON;
        break;
    case BTNON:
        if(PS4.isConnected()){
            digitalWrite(BTNPIN, PS4.Cross() ? 1 : 0);
        }
        break;
    default:
        state = BTNINIT;
        break;
    }

    switch (state)
    {
    case BTNINIT:
        break;
    case BTNON:
        break;
    default:
        break;
    }

    return state;
}

int PLYRTRNReadTick(int state){
    switch (state)
    {
    case PLYRTRNREADINIT:
        state = PLYRTRNREADON;
        break;
    case PLYRTRNREADON:
        state = PLYRTRNREADON;
        break;
    default:
        state = PLYRTRNREADINIT;
        break;
    }

    switch (state)
    {
    case PLYRTRNREADINIT:
        break;
    case PLYRTRNREADON:
        player1Turn = digitalRead(PLYRTRNPIN) == 1 ? true : false;
        break;
    default:
        break;
    }

    return state;
}

int GameStatusReadTick(int state){
    switch (state)
    {
    case GAMESTATUSREADINIT:
        state = GAMESTATUSREADON;
        break;
    case GAMESTATUSREADON:
        state = GAMESTATUSREADON;
        break;
    default:
        state = GAMESTATUSREADINIT;
        break;
    }

    switch (state)
    {
    case GAMESTATUSREADINIT:
        break;
    case GAMESTATUSREADON:
        gameStarted = digitalRead(GAMESTATUSPIN) == 1 ? true : false;
        break;
    default:
        break;
    }

    return state;
}

