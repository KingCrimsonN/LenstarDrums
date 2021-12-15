// MIDIUSB - Version: Latest 
#include <MIDIUSB.h>

// PADS
const int Npads = 6;
const int padsPins[Npads] = {A11, A10, A9, A8, A7, A6};

const int piezoThreshold = 50;
//const int tempoThreshold = 188;

int padCState[Npads] = {0}; // Current state of the pot; delete 0 if 0 pots
int padPState[Npads] = {0}; // Previous state of the pot; delete 0 if 0 pots
bool padPlaying[Npads] = {0};

//SWITCHES
const int Nswitch = 2;
const int switchPins[Nswitch] = {22,23};
int switchCState[Nswitch] = {0};
int switchPState[Nswitch] = {0};

int switchValue = 0;
const int switchOffset = 12;


// MIDI setup
byte midiChannel = 1;
byte note = 36;
byte cc = 1;

// POTENTIOMETERS
const int NPots = 4; //*** total number of pots (knobs and faders)
const int potPin[NPots] = {A0,A1,A2,A3}; //*** define Analog Pins connected from Pots to Arduino; Leave nothing in the array if 0 pots {}

int potCState[NPots] = {0}; // Current state of the pot; delete 0 if 0 pots
int potPState[NPots] = {0}; // Previous state of the pot; delete 0 if 0 pots
int potVar = 0; // Difference between the current and previous state of the pot

int midiCState[NPots] = {0}; // Current state of the midi value; delete 0 if 0 pots
int midiPState[NPots] = {0}; // Previous state of the midi value; delete 0 if 0 pots

const int TIMEOUT = 300; //* Amount of time the potentiometer will be read after it exceeds the varThreshold
const int varThreshold = 10; //* Threshold for the potentiometer signal variation
boolean potMoving = true; // If the potentiometer is moving
unsigned long PTime[NPots] = {0}; // Previously stored time; delete 0 if 0 pots
unsigned long timer[NPots] = {0}; // Stores the time that has elapsed since the timer was reset; delete 0 if 0 pots

// MIDI output
void noteOn(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOn = {0x09, 0x90 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOn);
}

void noteOff(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOff = {0x08, 0x80 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOff);
}

void controlChange(byte channel, byte control, byte value) {
  midiEventPacket_t event = {0x0B, 0xB0 | channel, control, value};
  MidiUSB.sendMIDI(event);
}

void setup() {
    pinMode(22,INPUT_PULLUP);
    pinMode(23,INPUT_PULLUP);
    Serial.begin(9600);
}

void loop() {
// current time
    pads();
    switches();
    potentiometers();
}

void switches() {
  for (int i=0; i<Nswitch; i++){
    switchCState[i] = digitalRead(switchPins[i]);
    if (switchCState[i] != switchPState[i])
      switchValue = switchCState[0] + switchCState[1] - 1; 
    switchPState[i]=switchCState[i];
  }
}

void pads(){
  for (int i = 0; i < Npads; i++) {
    padCState[i] = analogRead(padsPins[i]);
    //      Serial.println("padAnalogRead");
//      Serial.println(padCState[0]);
    if (padCState[i] > piezoThreshold && padPlaying[i] == false){
      const int diff = padCState[i] - padPState[i];
      if (diff > 30){
        noteOn(midiChannel, (note + i*5 + switchValue*switchOffset), map(padCState[i], 0, 1023, 0, 127));  // channel, note, velocity
        padPlaying[i] = true;
        MidiUSB.flush();
      } else {
        noteOff(midiChannel, (note + i*5 + switchValue*switchOffset), 0);  // channel, note, velocity
        padPlaying[i] = false;
        MidiUSB.flush();
      }
    } else if (padPlaying[i] == true){
        noteOff(midiChannel, (note + i*5 + switchValue*switchOffset), 0);  // channel, note, velocity
        padPlaying[i] = false;
        MidiUSB.flush();
    }
    padPState[i] = padCState[i];
  } 
}

void potentiometers() {


  for (int i = 0; i < NPots; i++) { // Loops through all the potentiometers

    potCState[i] = analogRead(potPin[i]); // reads the pins from arduino

    midiCState[i] = map(potCState[i], 0, 1023, 0, 127); // Maps the reading of the potCState to a value usable in midi

    potVar = abs(potCState[i] - potPState[i]); // Calculates the absolute value between the difference between the current and previous state of the pot

    if (potVar > varThreshold) { // Opens the gate if the potentiometer variation is greater than the threshold
      PTime[i] = millis(); // Stores the previous time
    }

    timer[i] = millis() - PTime[i]; // Resets the timer 11000 - 11000 = 0ms

    if (timer[i] < TIMEOUT) { // If the timer is less than the maximum allowed time it means that the potentiometer is still moving
      potMoving = true;
    }
    else {
      potMoving = false;
    }

    if (potMoving == true) { // If the potentiometer is still moving, send the change control
      if (midiPState[i] != midiCState[i]) {

        // Sends  MIDI CC 
        // Use if using with ATmega32U4 (micro, pro micro, leonardo...)
        controlChange(midiChannel, cc + i, midiCState[i]); //  (channel, CC number,  CC value)
        MidiUSB.flush();

        potPState[i] = potCState[i]; // Stores the current reading of the potentiometer to compare with the next
        midiPState[i] = midiCState[i];
      }
    }
  }
}
