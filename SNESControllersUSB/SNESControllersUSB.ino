/*  DaemonBite (S)NES Controllers to USB Adapter
 *  Author: Mikael Norrgård <mick@daemonbite.com>
 *
 *  Copyright (c) 2020 Mikael Norrgård <http://daemonbite.com>
 *  
 *  GNU GENERAL PUBLIC LICENSE
 *  Version 3, 29 June 2007
 *  
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *  
 */

#include "Gamepad.h"

#define GAMEPAD_COUNT 2     // NOTE: No more than TWO gamepads are possible at the moment due to a USB HID issue.
#define GAMEPAD_COUNT_MAX 4 // NOTE: For some reason, can't have more than two gamepads without serial breaking. Can someone figure out why?
                            //       (It has something to do with how Arduino handles HID devices)
#define SNES 0
#define NES  1
#define GPTYPE NES          // NOTE: Set gamepad type here (NES or SNES)! :)
#define BUTTON_READ_DELAY 300 // Button read delay in µs

#define UP    0x01
#define DOWN  0x02
#define LEFT  0x04
#define RIGHT 0x08

// Wire it all up according to the following table:
//
// NES           SNES        Arduino Pro Micro
// --------------------------------------
// VCC                       VCC (All gamepads)
// GND                       GND (All gamepads)
// OUT0 (LATCH)              2   (PD1, All gamepads)
// CUP  (CLOCK)              3   (PD0, All gamepads)
// D1   (GP1: DATA)          A0  (PF7, Gamepad 1) 
// D1   (GP2: DATA)          A1  (PF6, Gamepad 2)
// D1   (GP3: DATA)          A2  (PF5, Gamepad 3)
// D1   (GP4: DATA)          A3  (PF4, Gamepad 4)

// Set up USB HID gamepads
Gamepad_ Gamepad[GAMEPAD_COUNT];

// Controllers
uint16_t buttons[GAMEPAD_COUNT_MAX] = {0,0,0,0};
uint16_t buttonsPrev[GAMEPAD_COUNT_MAX] = {0,0,0,0};
uint8_t gpBit[GAMEPAD_COUNT_MAX] = {B10000000,B01000000,B00100000,B00010000};
uint16_t btnBitsSnes[12] = {0x200,0x800,0x8000,0x4000,UP,DOWN,LEFT,RIGHT,0x100,0x400,0x1000,0x2000};
uint16_t btnBitsNes[8] =  {0x100,0x200,0x8000,0x4000,UP,DOWN,LEFT,RIGHT};
uint16_t *btnBits;
uint8_t gp = 0;
uint8_t gpType = GPTYPE;
uint8_t buttonCount = 0;

// Timing
long microsNow = 0;
long microsButtons = 0;

void setup()
{
  // Setup latch and clock pins (2,3 or PD1, PD0)
  DDRD  |=  B00000011; // output
  PORTD &= ~B00000011; // low

  // Setup data pins (A0-A3 or PF7-PF4)
  DDRF  &= ~B11110000; // inputs
  PORTF |=  B11110000; // enable internal pull-ups

  if(gpType == SNES) {
    buttonCount = 12;
    btnBits = btnBitsSnes;
  }
  else {
    buttonCount = 8;
    btnBits = btnBitsNes;
  }
}

void loop()
{
  // Get current time
  microsNow = micros();

  // See if enough time has passed since last button read
  if(microsNow > microsButtons+BUTTON_READ_DELAY)
  {    
    // Pulse latch
    sendLatch();

    for(uint8_t btn=0; btn<buttonCount; btn++)
    {
      for(gp=0; gp<GAMEPAD_COUNT; gp++)
        (PINF & gpBit[gp]) ? buttons[gp] &= ~btnBits[btn] : buttons[gp] |= btnBits[btn];
      sendClock();
    }

    microsButtons = microsNow+100;
  }
  
  for(gp=0; gp<GAMEPAD_COUNT; gp++)
  {
    // Has any buttons changed state?
    if (buttons[gp] != buttonsPrev[gp])
    {
      Gamepad[gp]._GamepadReport.buttons = buttons[gp] >> 8;
      Gamepad[gp]._GamepadReport.Y = ((buttons[gp] & DOWN) >> 1) - (buttons[gp] & UP);
      Gamepad[gp]._GamepadReport.X = ((buttons[gp] & RIGHT) >> 3) - ((buttons[gp] & LEFT) >> 2);
      buttonsPrev[gp] = buttons[gp];
      Gamepad[gp].send();
    }
  }
}

void sendLatch()
{
  // Send a latch pulse to (S)NES controller(s)
  PORTD |=  B00000010; // Set HIGH
  delayMicroseconds(12);
  PORTD &= ~B00000010; // Set LOW
  delayMicroseconds(6); 
}

void sendClock()
{
  // Send a clock pulse to (S)NES controller(s)
  PORTD |=  B10000001; // Set HIGH
  delayMicroseconds(6);
  PORTD &= ~B10000001; // Set LOW
  delayMicroseconds(6); 
}
