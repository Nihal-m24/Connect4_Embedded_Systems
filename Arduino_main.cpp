/*         Your Name & E-mail: Muhammad Memon mmemo005@ucr.edu
 *         Discussion Section: 24
 *         Assignment: Custom Project Part 3
 *
 *         I acknowledge all content contained herein, excluding template or example code, is my own original work.
 *
 *         Full Game Play: https://www.youtube.com/watch?v=SvRot21eqtc
 *         Build Upon#1: https://www.youtube.com/watch?v=TMe5bN8cUdQ
 *         Build Upon#2: https://www.youtube.com/watch?v=CajaHFgYCWA
 *         Build Upon#3: https://www.youtube.com/watch?v=tv2kqMu10bA
 *         Build Upon#4: https://www.youtube.com/watch?v=lT29cijZHZk

 */

#include "timerISR.h"
#include "helper.h"
#include "periph.h"
#include "i2c.h"

#define NUM_TASKS 5 

// Music Notes
#define NOTE_C4  261
#define NOTE_D4  294
#define NOTE_E4  329
#define NOTE_F4  349
#define NOTE_G4  392
#define NOTE_A4  440
#define NOTE_B4  493
#define NOTE_C5  523
#define NOTE_D5  587
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_G5  784
#define NOTE_A5  880
#define NOTE_B5  987
#define NOTE_C6  1046

//GLOBAL VARIABLES
bool systemIsOn = false;
bool gameStarted = false;
bool player1Turn = true;
bool gameWon = false;
int startGameCounter = 0;

int winMelody[] = {
    NOTE_E5, NOTE_E5, NOTE_E5, NOTE_G5, NOTE_A5, NOTE_B5, NOTE_C6, NOTE_B5,
    NOTE_A5, NOTE_G5, NOTE_F5, NOTE_E5, NOTE_G5, NOTE_A5, NOTE_B5, NOTE_A5,
    NOTE_E5, NOTE_E5, NOTE_E5, NOTE_F5, NOTE_G5, NOTE_A5, NOTE_B5, NOTE_C6,
    NOTE_A5, NOTE_G5, NOTE_F5, NOTE_E5, NOTE_E5, NOTE_E5, NOTE_A5, NOTE_B5 
};

//Task struct for concurrent synchSMs implmentations
typedef struct _task{
	signed 	 char state; 		
	unsigned long period; 		
	unsigned long elapsedTime; 	
	int (*TickFct)(int); 		
} task;


// Define Periods for each task
const unsigned long GCD_PERIOD = 100;//TODO:Set the GCD Period
const unsigned long START_BTN_PERIOD = 200;
const unsigned long RGB_LED_PERIOD = 500;
const unsigned long BUZZER_PERIOD = 300;
const unsigned long SEND_SYSTEM_INFO_PERIOD = 400;
const unsigned long READ_SYSTEM_INFO_PERIOD = 200;

task tasks[NUM_TASKS]; 

// tasks' function and their states
enum BTNStates {BTNINIT, BTNWAIT, BTNPRESS} BtnState;
int BtnTick(int state);

enum RGBLEDStates {RGBLEDINIT, RGBLEDOFF, RGBLEDON, RGBLEDP1, RGBLEDP2} RGBLEDState;
int RGBLEDTick(int state);

enum BuzzerStates {BUZZERINIT, BUZZEROFF, BUZZERON} BuzzerState;
int BuzzerTick(int state);

enum SendSystemInoStates {SENDSYSINFOINIT, SENDSYSINFOWAIT, SENDSYSINFOSEND, GAMEINFOSEND} SendSystemInoState;
int SendSystemInoTick(int state);

enum ReadSystemStates {READSYSTEMINIT, READSYSTEMON} ReadSystemState;
int ReadSystemTick(int state);


void TimerISR() {
	for ( unsigned int i = 0; i < NUM_TASKS; i++ ) {           
		if ( tasks[i].elapsedTime == tasks[i].period ) {       
			tasks[i].state = tasks[i].TickFct(tasks[i].state); 
			tasks[i].elapsedTime = 0;                          
		}
		tasks[i].elapsedTime += GCD_PERIOD;                    
	}
}


int main(void) {
    //Initialize all your inputs and ouputs
    DDRC = 0x00; //All port C are inputs
    PORTC = 0xFF;
    
    DDRD = 0xFF; //All port D are outputs
    PORTD = 0x00;
    
    DDRB = 0xFF; //All port B are outputs
    PORTB = 0x00;

    ADC_init();   // initializes ADC

    //TODO: Initialize the buzzer timer/pwm(timer0)
    TCCR1A |= (1 << WGM11) | (1 << COM1A1); //COM1A1 sets it to channel A
    TCCR1B |= (1 << WGM12) | (1 << WGM13) | (1 << CS12);

    i2c_init();

    //Tasks
    tasks[0].period = START_BTN_PERIOD;
    tasks[0].state = BTNINIT;
    tasks[0].elapsedTime = tasks[0].period;
    tasks[0].TickFct = BtnTick;

    tasks[1].period = RGB_LED_PERIOD;
    tasks[1].state = RGBLEDINIT;
    tasks[1].elapsedTime = tasks[1].period;
    tasks[1].TickFct = RGBLEDTick;

    tasks[2].period = BUZZER_PERIOD;
    tasks[2].state = BUZZERINIT;
    tasks[2].elapsedTime = tasks[2].period;
    tasks[2].TickFct = BuzzerTick;

    tasks[3].period = SEND_SYSTEM_INFO_PERIOD;
    tasks[3].state = SENDSYSINFOINIT;
    tasks[3].elapsedTime = tasks[3].period;
    tasks[3].TickFct = SendSystemInoTick;

    tasks[4].period = READ_SYSTEM_INFO_PERIOD;
    tasks[4].state = READSYSTEMINIT;
    tasks[4].elapsedTime = tasks[4].period;
    tasks[4].TickFct = ReadSystemTick;


    TimerSet(GCD_PERIOD);
    TimerOn();

    while (1) {}

    return 0;
}

int BtnTick(int state){
    unsigned char A0 = (PINC >> 0) & 0x01;
    static int btnPressCount = 0;
    switch (state)
    {
    case BTNINIT:
        state = BTNWAIT;
        break;
    case BTNWAIT:
        if(A0){
            state = BTNPRESS;
        } else{
            state = BTNWAIT;
        }
        break;
    case BTNPRESS:
        btnPressCount++;
        if(A0){
            state = BTNPRESS;
        } else {
            if(systemIsOn){
                if(btnPressCount < 5){
                    gameStarted = true;
                    startGameCounter++;
                } else {
                    systemIsOn = false;
                }
            } else {
                systemIsOn = !systemIsOn;
                gameStarted = false;
                startGameCounter++;
            }
            btnPressCount = 0;
            state = BTNWAIT;
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
    case BTNWAIT:
        break;
    case BTNPRESS:
        break;
    default:
        break;
    }

    return state;
}

int RGBLEDTick(int state){
    switch (state)
    {
    case RGBLEDINIT:
        state = RGBLEDOFF;
        break;
    case RGBLEDOFF:
        if(systemIsOn){
            state = RGBLEDON;
        } else {
            state = RGBLEDOFF;
        }
        break;
    case RGBLEDON:
        if(systemIsOn){
           if(gameWon){
               state = RGBLEDON;
           } else if(gameStarted){
               if(player1Turn){
                   state = RGBLEDP1;
               } else {
                   state = RGBLEDP2;
               }
           } else {
               state = RGBLEDON;
           }
        } else {
            state = RGBLEDOFF;
        }
        break;
    case RGBLEDP1:
        if(systemIsOn){
            if(gameWon){
                state = RGBLEDON;
            } else if(gameStarted){
                if(player1Turn){
                    state = RGBLEDP1;
                } else {
                    state = RGBLEDP2;
                }
            } else {
                state = RGBLEDON;
            }
        } else {
            state = RGBLEDOFF;
        }
        break;
    case RGBLEDP2:
        if(systemIsOn){
            if(gameWon){
                state = RGBLEDON;
            } else if(gameStarted){
                if(player1Turn){
                    state = RGBLEDP1;
                } else {
                    state = RGBLEDP2;
                }
            } else {
                state = RGBLEDON;
            }
        } else {
            state = RGBLEDOFF;
        }

        break;
    default:
        break;
    }

    switch (state)
    {
    case RGBLEDINIT:
        break;
    case RGBLEDOFF:
        PORTD = SetBit(PORTD, 5, 0); // RED RGB LED
        PORTD = SetBit(PORTD, 6, 0); // GREEN RGB LED
        PORTD = SetBit(PORTD, 7, 0); // BLUE RGB LED
        break;
    case RGBLEDON:
        PORTD = SetBit(PORTD, 5, 0); // RED RGB LED
        PORTD = SetBit(PORTD, 6, 1); // GREEN RGB LED
        PORTD = SetBit(PORTD, 7, 0); // BLUE RGB LED
        break;
    case RGBLEDP1:
        PORTD = SetBit(PORTD, 5, 1); // RED RGB LED
        PORTD = SetBit(PORTD, 6, 0); // GREEN RGB LED
        PORTD = SetBit(PORTD, 7, 0); // BLUE RGB LED
        break;
    case RGBLEDP2:
        PORTD = SetBit(PORTD, 5, 0); // RED RGB LED
        PORTD = SetBit(PORTD, 6, 0); // GREEN RGB LED
        PORTD = SetBit(PORTD, 7, 1); // BLUE RGB LED
        break;
    default:
        break;
    }

    return state;
}

int BuzzerTick(int state){
    static bool played = false;
    static int noteToPlay = 0;
    switch (state)
    {
    case BUZZERINIT:
        state = BUZZEROFF;
        break;
    case BUZZEROFF:
        if(!played && gameWon){
            state = BUZZERON;
        } else{
            if(!gameWon){
                played = false;
            }
            state = BUZZEROFF;
        }
        ICR1 = 0;
        OCR1A = 65535;
        break;
    case BUZZERON:
        if (!played) {
            ICR1 =  winMelody[noteToPlay];
            OCR1A = ICR1 / 2;

            noteToPlay++;
            if(noteToPlay > 31){
                played = true;
                noteToPlay = 0;
            }
            state = BUZZERON;
        } else {
            state = BUZZEROFF;
        }
        break;
    default:
        state = BUZZERINIT;
        break;
    }

    switch (state)
    {
    case BUZZERINIT:
        break;
    case BUZZEROFF:
        
        break;
    case BUZZERON:
        break;
    default:
        break;
    }


    return state;
}

int SendSystemInoTick(int state){
    static bool localSystemIsOn = false;
    static bool updatedGamemOn = false;
    static int counter = 0;
    static int gameStartCntr = 0;

    switch (state)
    {
    case SENDSYSINFOINIT:
        state = SENDSYSINFOWAIT;
        break;
    case SENDSYSINFOWAIT:
        if(gameStartCntr != startGameCounter){
            gameStartCntr = startGameCounter; 
            updatedGamemOn = false;
        }
        if(localSystemIsOn != systemIsOn){
            state = SENDSYSINFOSEND;
        } else if (gameStarted && !updatedGamemOn){
            state = GAMEINFOSEND;
        }
        else {
             state = SENDSYSINFOWAIT;
        }

        break;
    case SENDSYSINFOSEND:
        PORTD = SetBit(PORTD, 4, systemIsOn ? 1 : 0);
        localSystemIsOn = systemIsOn;
        state = SENDSYSINFOWAIT;
        break;

    case GAMEINFOSEND:
        if(counter < 3){
            PORTD = SetBit(PORTD, 3, 0);
            state = GAMEINFOSEND;
            counter++;
        } else {
            updatedGamemOn = true;
            PORTD = SetBit(PORTD, 3, 1);
            counter = 0;
            state = SENDSYSINFOWAIT;
        }
        break;
    default:
        break;
    }

    switch (state)
    {
    case SENDSYSINFOINIT:
        break;
    case SENDSYSINFOWAIT:
        break;
    case SENDSYSINFOSEND:
        break;
    case GAMEINFOSEND:
        break;
    default:
        break;
    }

    return state;
}

int ReadSystemTick(int state){
    unsigned char A1 = (PINC >> 1) & 0x01;
    unsigned char A2 = (PINC >> 2) & 0x01;
    switch (state)
    {
    case READSYSTEMINIT:
        state = READSYSTEMON;
        break;
    case READSYSTEMON:
        state = READSYSTEMON;
        break;
    default:
        break;
    }

    switch (state)
    {
    case READSYSTEMINIT:
        break;
    case READSYSTEMON:
        player1Turn = A1 == 1 ? true : false;
        gameWon = A2 == 1 ? true : false;
        break;
    default:
        break;
    }

    return state;
}
