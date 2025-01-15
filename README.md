# Connect 4 - Embedded Systems

## Demo
The entire build and gameplay demo can be found on [Youtube](https://www.youtube.com/watch?v=SvRot21eqtc).

## Overview
For my Embedded Systems class I was tasked with creating a game with the following requirements: 
- Have a soft reset button
- Display a score
- Have a way for the player to win

The game was to be implemented using the TimerISR and breaking it down into different states. We were not allowed to use the Arduino Library for any part of the implementation of the game.
The game also needed to contain 3 "build-upon". The "build-upons" were either hardware implementations that we did not cover in class or software implementations such as a non-trivial second player (AI).

Seeing these requirements, I decided to create my favorite childhood game: Connect 4. Connect 4 is a two-player game where players take turns dropping different colored chips in a 7x6 grid. The first player to connect 4 chips together in either a horizontal, vertical, or diagonal manner wins. 

### My Build-Upons
- Bluetooth to connect the ESP32 with the PS4 controller
- Passive Buzzer to play a small tune when a player wins
- Using a separate microcontroller (besides Arduino)
- 8X8 Led RGB Matrix for the game board
  
### Challenges 
As I conquered this project, there were a few challenges that arose. The first challenge that I faced was that the library used to control the LED matrix and the Bluetooth library to connect the PS4 controller were interfering with each other and the TimerISR() class I modified to work on the ESP32. To fix the issue, I connected one more ESP32 to the system - requiring more coding and overhead for transmitting data. 
Another challenge I faced was transmitting data from the Arduino Uno to the ESP32. I tried multiple methods such as I2C, UART, and others but none of them worked. So I had to use digital signals to transmit the data, which in turn created another problem. The Arduino outputs 5V while the ESP32 only takes in 3V, this meant that if I connected the wires directly, I could end up burning my ESP32. I tried the Bi-Directional 5V and 3V converter but that required soldiering, something I had no experience in. So I used resistors in series to allow the connects to safely transmit data. 

## Parts Used
- Arduino Uno
- ESP32 - WROOM (38 Pins)
- WS2812B 8x8 Led Matrix (3 of them)
- PS4 Controller


## Libraries Used
- [FastLED](https://github.com/FastLED/FastLED)
- [PS4 Controller Library](https://github.com/pablomarquez76/PS4_Controller_Host)

## Interesting Things I learned
This project showed me a lot of interesting things. One of the interesting things I learned through this project is about MAC addresses and how to manipulate them on the ESP32. I learned that each MAC Address is built into the chip of each device and is generally unchangeable. However, in order to connect the PS4 controller to the ESP32 I had to first connect it to my phone and fake the ESP32's MAC Address to that of my phone. 
