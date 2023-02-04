/*
 * Arduino Midi Master Clock v0.2
 * MIDI master clock/sync/divider for MIDI instruments, Pocket Operators and Korg Volca.
 * by Eunjae Im https://ejlabs.net/arduino-midi-master-clock
 * 
 * Modified by CallPhysical as follows:
 * - show both BPM and PPQN on screen together
 * - enable fast BPM adjustment
 * - change PPQN display to absolute value
 * - add 3rd CV output
 * 
 * Required library
 *    TimerOne https://playground.arduino.cc/Code/Timer1
 *    Encoder https://www.pjrc.com/teensy/td_libs_Encoder.html
 *    MIDI https://github.com/FortySevenEffects/arduino_midi_library
 *    Adafruit SSD1306 https://github.com/adafruit/Adafruit_SSD1306
 *    
 *******************************************************************************
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 *******************************************************************************
 */

#include <Adafruit_SSD1306.h>
#include <TimerOne.h>
#include <EEPROM.h>
#include <Encoder.h>
#include <MIDI.h>

#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

#define LED_PIN1 7 // Tempo LED
#define SYNC_OUTPUT_PIN 6 // Audio Sync Digital Pin
#define SYNC_OUTPUT_PIN2 8 // 2nd Audio Sync Pin
#define SYNC_OUTPUT_PIN3 9 // 3rd Audio Sync Pin       // comment out if you only have 2 CV jacks
#define BUTTON_START 4 // Start/Stop Push Button
#define BUTTON_ROTARY 5 // Rotary Encoder Button

#define CLOCKS_PER_BEAT 24 // MIDI Clock Ticks
#define AUDIO_SYNC 12 // Audio Sync Ticks
#define AUDIO_SYNC2 12 // 2nd Audio Sync Ticks

#define MINIMUM_BPM 20
#define MAXIMUM_BPM 300

#define BLINK_TIME 4 // LED blink time

volatile int  blinkCount = 0,
              blinkCount2 = 0,
              AudioSyncCount = 0,
              AudioSyncCount2 = 0;

long intervalMicroSeconds,
      bpm,
      audio_sync2;

boolean playing = false,
      sync_editing = false;
//      display_update = false;

int sync_current;
  
Encoder myEnc(2, 3); // Rotary Encoder Pin 2,3 

MIDI_CREATE_DEFAULT_INSTANCE();

void setup(void) {
  MIDI.begin(); // MIDI init
  MIDI.turnThruOff();
// Serial.begin(9600);
// Serial.println("Starting MASTER SYNC!!!");
  bpm = EEPROMReadInt(0);
  if (bpm > MAXIMUM_BPM || bpm < MINIMUM_BPM) {
    bpm = 120;
  }
  audio_sync2 = EEPROMReadInt(3);
  if (audio_sync2 > 64 || audio_sync2 < 2) {
    audio_sync2 = 12;
  }

//  sync_current = (audio_sync2 - 12)*-1;  
  sync_current = audio_sync2;  
  
  Timer1.initialize(intervalMicroSeconds);
  Timer1.setPeriod(60L * 1000 * 1000 / bpm / CLOCKS_PER_BEAT);
  Timer1.attachInterrupt(sendClockPulse);  

  pinMode(BUTTON_START,INPUT_PULLUP);
  pinMode(BUTTON_ROTARY,INPUT_PULLUP);

  // set up initial display
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  // display the BPM value
  display.setTextColor(WHITE);  
  display.setTextSize(3);
  display.setCursor(5,10);
  display.print(bpm); 
  display.setTextSize(2);  
  display.setCursor(70,15);
  display.print("BPM"); 
  // display 2nd sync ppqn value
  display.setTextSize(1);
  display.setCursor(10,1);
  display.print(sync_current);
  display.print(" PPQN");
  display.display();
}

void EEPROMWriteInt(int p_address, int p_value) // write a value to memory
     {
     byte lowByte = ((p_value >> 0) & 0xFF);
     byte highByte = ((p_value >> 8) & 0xFF);

     EEPROM.write(p_address, lowByte);
     EEPROM.write(p_address + 1, highByte);
     }

unsigned int EEPROMReadInt(int p_address) // read a value from memory
     {
     byte lowByte = EEPROM.read(p_address);
     byte highByte = EEPROM.read(p_address + 1);

     return ((lowByte << 0) & 0xFF) + ((highByte << 8) & 0xFF00);
}

void bpm_display() {  // update the BPM display
  updateBpm();
  EEPROMWriteInt(0,bpm);  

//  sync_current = (audio_sync2 - 12)*-1; 
  sync_current = audio_sync2;  // Changed to show absolute value
  // display the BPM in a large font
  display.setTextSize(3);
  display.setCursor(5,10);  
  display.setTextColor(WHITE, BLACK);
  display.print("            ");
  display.setCursor(5,10);
  
  display.print(bpm);
  display.setTextSize(2);  
  display.setCursor(70,15);
  display.print("BPM"); 
  
    // display seq2 ppqn in a small font above
  display.setTextSize(1);
  display.setCursor(10,1);  
  display.print("            ");
  display.setCursor(10,1);
  display.print(sync_current);
  display.print(" PPQN");
  
  display.display(); 
  
//  display_update = false;
}

void sync_display() {
  EEPROMWriteInt(3,audio_sync2);
  
//  int sync_current;

//  sync_current = (audio_sync2 - 12)*-1;
  sync_current = audio_sync2;
   // display seq2 ppqn in a large font    
  display.setTextSize(3);
  display.setCursor(5,10);
  display.setTextColor(WHITE, BLACK);
  display.print("            ");  
  display.setCursor(5,10);
  display.print(sync_current);
  display.setTextSize(1);  
  display.setCursor(70,10);
  display.print("Sync 2"); 
  display.setCursor(70,20);
  display.print(" ppqn"); 
  
   // display bpm in a small font above
  display.setTextSize(1);
  display.setCursor(10,1);  
  display.print("             ");
  display.setCursor(10,1);
  display.print(bpm);
  display.print(" BPM"); 
  
  display.display();
}

void startOrStop() {
  if (!playing) {
    MIDI.sendRealTime(midi::Start);
  } else {
    all_off();
    MIDI.sendRealTime(midi::Stop);
  }
  playing = !playing;
}

int oldPosition;

void loop(void) {
  byte i = 0;
  byte p = 0;
  
  if (digitalRead(BUTTON_START) == LOW) {
    startOrStop();
    delay(300); // ugly but just make life easier, no need to check debounce
  } else if (digitalRead(BUTTON_ROTARY) == LOW) {    
    p = 1; // adjust second CV ppqn
    delay(200);
    
  }
  
  int newPosition = (myEnc.read()/4);
  if (newPosition != oldPosition) {    
    if (oldPosition < newPosition) {
      i = 2;
    } else if (oldPosition > newPosition) {
      i = 1;
    }
    if (abs(oldPosition - newPosition) > 3) { // controller is being turned quickly
      i = i * 10;
    }
      
    oldPosition = newPosition;
  }
  
  if (!sync_editing) {      
      if (i == 2) {
        bpm++;
        if (bpm > MAXIMUM_BPM) {
          bpm = MAXIMUM_BPM;
        }
        bpm_display();          
      } else if (i == 1) {
        bpm--;
        if (bpm < MINIMUM_BPM) {
          bpm = MINIMUM_BPM;
        }
        bpm_display();
      } else if (i == 20) { // controller was turned quickly so increment by 10
        bpm = bpm + 10;
        if (bpm < MINIMUM_BPM) {
          bpm = MINIMUM_BPM;
        }
        bpm_display();
      } else if (i == 10) {  // controller was turned quickly so decrement by 10
        bpm = bpm - 10;
        if (bpm < MINIMUM_BPM) {
          bpm = MINIMUM_BPM;
        }
        bpm_display();
      } else if (p == 1) {
        //rotary.resetPush();
        sync_display();
        sync_editing = true;
        
      }
  } else  { // 2nd jack audio sync ppqn
      if (p == 1) {      
//        Serial.println("Button pushed");
        bpm_display();
        sync_editing = false;
      } else if (i == 1) {      
        audio_sync2--;
        if (audio_sync2 > 64) { audio_sync2 = 64; }
        sync_display();
      } else if (i == 2) {
        audio_sync2++;
        if (audio_sync2 < 2) { audio_sync2 = 2; }
        sync_display();
      }      
  }
}

void all_off() { // make sure all sync, led pin stat to low
  digitalWrite(SYNC_OUTPUT_PIN, LOW); // Main CV
  digitalWrite(SYNC_OUTPUT_PIN2, LOW); // 2nd CV
  digitalWrite(SYNC_OUTPUT_PIN3, LOW); // 3rd CV       // comment out if you only have 2 CV jacks
  digitalWrite(LED_PIN1, LOW);
}

void sendClockPulse() {  

  MIDI.sendRealTime(midi::Clock); // sending midi clock
  
  if (playing) {  
  
  blinkCount = (blinkCount + 1) % CLOCKS_PER_BEAT;
  blinkCount2 = (blinkCount2 + 1) % (CLOCKS_PER_BEAT / 2);
  AudioSyncCount = (AudioSyncCount + 1) % AUDIO_SYNC;
  AudioSyncCount2 = (AudioSyncCount2 + 1) % audio_sync2;

  if (AudioSyncCount == 0) { // Main CV
      digitalWrite(SYNC_OUTPUT_PIN, HIGH); 
      
  } else {        
    if (AudioSyncCount == 1) {     
      digitalWrite(SYNC_OUTPUT_PIN, LOW);
    }
  }  

  if (AudioSyncCount2 == 0) { // 2nd and 3rd CV
      digitalWrite(SYNC_OUTPUT_PIN2, HIGH);
      digitalWrite(SYNC_OUTPUT_PIN3, HIGH);      // comment out if you only have 2 CV jacks
  } else {        
    if (AudioSyncCount2 == 1) {
      digitalWrite(SYNC_OUTPUT_PIN2, LOW);
      digitalWrite(SYNC_OUTPUT_PIN3, LOW);       // comment out if you only have 2 CV jacks
    }
  }
  
  if (blinkCount == 0) {
      digitalWrite(LED_PIN1, HIGH);      
  } else {
     if (blinkCount == BLINK_TIME) {
       digitalWrite(LED_PIN1, LOW);      
     }
  }

  
  } // if playing
}

void updateBpm() { // update BPM function (on the fly)
  long interval = 60L * 1000 * 1000 / bpm / CLOCKS_PER_BEAT;  
  Timer1.setPeriod(interval);
}
