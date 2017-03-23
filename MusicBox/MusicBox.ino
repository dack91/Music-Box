/*
 Name:		MusicBox.ino
 Created:	2/10/2017 10:24:18 PM
 Author:	Delainey Ackerman

 Make your own music box Arduino project for Physical Computer 15h credit at Goldsmiths University of London
*/
// Musical note frequencies
#include <LiquidCrystal.h>
#include <eeprom.h>

const int A = 220;
const int B = 247;
const int C = 262;
const int D = 294;
const int E = 330;
const int F = 349;
const int G = 392;

const int Bflat = 466;

// Max Length of melody
const int MAX_NOTES = 33;

typedef struct {
	int trackZone;				// zone for corresponding pot position, custom tracks are zones 3-5
	int notes[MAX_NOTES];		// array of frequencies played for track
	int noteDurs[MAX_NOTES];	// array of durations for each stored frequency
} CustomTrack;

// Play and Record light analog pins
const int redLEDPin = 9;
const int greenLEDPin = 10;

// Play and Record digital input pins
const int playPausePin = 6;
const int recordPin = 7;

// Piezo speaker output pin
const int soundPin = 8;

// Frequency of note to preview during recording input
int curRecNote = 0;

// Play and Record light colors
int redValue = 0;
int greenValue = 0;

// Melody Picker
const int potPin = A0;
int potVal;
int potZone;
int lastPotZone = 15;
int noteIndex;
int durationIndex;

int melodies[12][MAX_NOTES] = { 
	// Jurassic Park preset
	{ B, B, A, B, B, A, B, C, C, E, E, D, B, C, A, 175, D, B, C, F, B, E, D, D, C, C, 0, 0, 0, 0, 0, 0, 0 }, 
		{ 12, 4, 4, 12, 4, 4, 8, 4, 8, 4, 12, 4, 4, 8, 4, 8, 4, 4, 12, 4, 4, 8, 4, 8, 4, 12, 0, 0, 0, 0, 0, 0, 0 },

	// Lord of the Rings preset
	{ C, D, E, G, E, D, C, E, G, 440, 523, 493, G, E, F, E, D, C, D, E, G, E, D, E, D, C, E, G, 440, 440, G, E, D },
		{ 4, 4, 8, 8, 8, 4, 12, 4, 4, 8, 4, 4, 4, 8, 4, 4, 8, 4, 4, 8, 4, 4, 8, 2, 2, 12, 4, 4, 8, 4, 4, 4, 16 },

	// How to Save a Life preset
	{ Bflat, F, D, F, F, D, F, 523, F, D, F, F, D, F, 587, F, D, F, F, D, F, 440, F, D, F, 523, F, D, F, Bflat, 0, 0, 0 },
		{ 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 16, 0, 0, 0 },

	// Recordable track 1
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },

	// Recordable track 2
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },

	// Recordable track 3
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
};

// Currently loaded tune notes and note duractions
int notes[MAX_NOTES];
int noteDurations[MAX_NOTES];

// Play switch states
int onOffPlaySwitchState = 0;
int previousOnOffPlaySwitchState = 0;

// Record switch states
int onOffRecordSwitchState = 0;
int previousOnOffRecordSwitchState = 0;

// monitor playing and recording modes
bool playEnabled = false;
bool recordEnabled = false;
int recordNoteIndex = 0;

// Recording intervals
const long blinkInterval = 1000;
unsigned long prevMillis = 0;
unsigned long currNoteMillis = 0;
unsigned long lastNoteMillis = 0;
const int MILLIS_TO_NOTE_DUR = 20;		// scale milliseconds of button push to note duration in stored array
bool isKeyDown = false;

// Display Screen
LiquidCrystal lcd(13, 12, 5, 4, 3, 2);

// the setup function runs once when you press reset or power the board
void setup() {
	Serial.begin(9600);
	lcd.begin(16, 2);		// 16, 2 is screen size

	// Sound speaker
	pinMode(soundPin, OUTPUT);

	// Play and Pause button
	pinMode(playPausePin, INPUT);

	// Record button
	pinMode(recordPin, INPUT);

	// Play and Record light
	pinMode(redLEDPin, OUTPUT);
	pinMode(greenLEDPin, OUTPUT);

	// Load in saved custom tracks from persistent memory
	loadCustomTrackFromMemory();
}

// the loop function runs over and over again until power down or reset
void loop() {
	// While in record mode, play a preview of the note being recorded
	if (curRecNote != 0) {
		tone(soundPin, curRecNote, 900);
		Serial.print("Rec Note: ");
		Serial.println(curRecNote);
	}

	// Read potentiometer to determine current track
	potVal = analogRead(potPin);

	potZone = map(potVal, 0, 1023, 0, 179);
	//Serial.print("angle: ");
	//Serial.print(potZone);
	potZone /= 30;						// Adjust angle to zone 0 - 5;
	

	// Only clear and change LCD screen if zone changed
	if (potZone != lastPotZone) {
		noteIndex = potZone * 2;			// Each zone has 2 indices, one each for notes and noteDurations arrays
		durationIndex = noteIndex + 1;		// noteDuractions array stored one index past notes array
		lastPotZone = potZone;				// update zone change

		// Display potZone to lcd screen
		lcd.clear();
		lcd.setCursor(0, 0);
		lcd.print("Play Song: ");
		lcd.setCursor(0, 1);

		// Display tune associated with current zone
		switch (potZone)
		{
		case 0:
			lcd.print("Jurassic Park");
			break;
		case 1:
			lcd.print("LoTR: The Shire");
			break;
		case 2:
			lcd.clear();
			lcd.setCursor(0, 0);
			lcd.print("Play Song: How");
			lcd.setCursor(0, 1);
			lcd.print("to Save a Life");
			break;
		case 3:
			lcd.print("Custom Tune #1");
			break;
		case 4:
			lcd.print("Custom Tune #2");
			break;
		case 5:
			lcd.print("Custom Tune #3");
			break;
		default:
			break;
		}

		// Load current melody
		loadCurTrack();
	}

	Serial.print("zone: ");
	Serial.println(potZone);

	//Serial.print("record enabled: ");
	//Serial.println(recordEnabled);

	//Serial.print(", noteI: ");
	//Serial.print(noteIndex);

	//Serial.print(", durI: ");
	//Serial.println(durationIndex);

	// If current track is recordable (not the first three preset tracks)
	if (potZone > 2) {
		// Check if record button pressed
		checkRecordOnOff();

		// If in record mode
		if (recordEnabled) {
			unsigned long currMillis = millis();

			// Blink light while recording
			if (currMillis - prevMillis >= blinkInterval) {
				prevMillis = currMillis;	// Increment timer

				// Turn on light
				if (redValue != 255) {
					redValue = 255;			// set light to record color

					// Turn on record light
					analogWrite(redLEDPin, redValue);
					analogWrite(greenLEDPin, greenValue);
				}
				// Turn off light
				else {
					redValue = 0;			// reset light color

					// Turn off record light
					analogWrite(redLEDPin, redValue);
					analogWrite(greenLEDPin, greenValue);
				}
			}

			// If notes array is not full, record frequency of corresponding button press
			if (recordNoteIndex < MAX_NOTES) {
				int keyVal = analogRead(A1);

				// Read note based on input frequency from resistor ladder
				if (keyVal >= 1020) {
					curRecNote = G;
					recordNote(G);
				}
				else if (keyVal >= 990 && keyVal <= 1010) {
					curRecNote = F;
					recordNote(F);
				}
				else if (keyVal >= 960 && keyVal <= 980) {
					curRecNote = E;
					recordNote(E);
				}
				else if (keyVal >= 920 && keyVal <= 940) {
					curRecNote = D;
					recordNote(D);
				}
				else if (keyVal >= 680 && keyVal <= 700) {
					curRecNote = C;
					recordNote(C);
				}
				else if (keyVal >= 500 && keyVal <= 520) {
					curRecNote = B;
					recordNote(B);
				}
				else if (keyVal >= 5 && keyVal <= 15) {
					curRecNote = A;
					recordNote(A);
				}
				else {
					// On key up
					if (isKeyDown) {
						// Store duration of key press for the note just released
						melodies[durationIndex][recordNoteIndex] = currNoteMillis / MILLIS_TO_NOTE_DUR;
						recordNoteIndex++;

						//Serial.print("Note Dur: ");
						//Serial.println(currNoteMillis / MILLIS_TO_NOTE_DUR);

						currNoteMillis = 0;		// reset note duration timer
						isKeyDown = false;
						curRecNote = 0;			// Exiting record node, no input note to preview
					}
				}
			}
		}
		else {
			redValue = 0;		// reset light color

			// Turn off record light
			analogWrite(redLEDPin, redValue);
			analogWrite(greenLEDPin, greenValue);
		}
	}

	else {
		recordEnabled = false;
		redValue = 0;

		// reset light
		analogWrite(redLEDPin, redValue);
		analogWrite(greenLEDPin, greenValue);
	}

	// Check if play button pressed
	checkPlayOnOff();

	// Play melody if not currently recording
	if (playEnabled) {
		greenValue = 255;	// set light to play color

		// Turn on play light
		analogWrite(redLEDPin, redValue);
		analogWrite(greenLEDPin, greenValue);

		// Play melody
		for (int i = 0; i < MAX_NOTES; i++) {
			tone(soundPin, notes[i], noteDurations[i] * 50 * 1.1);	// play note with length
			
			delay(1.3 * 50 * noteDurations[i]);					// time between notes
			noTone(soundPin);

			// Check if play should be stopped
			checkPlayOnOff();	
			if (!playEnabled) {
				break;
			}
		}
		playEnabled = false;
		greenValue = 0;			// reset play light color
	}
	else {
		// Stop playing
		noTone(soundPin);

		// Turn off play light
		analogWrite(redLEDPin, redValue);
		analogWrite(greenLEDPin, greenValue);
	}

	previousOnOffPlaySwitchState = onOffPlaySwitchState;
	previousOnOffRecordSwitchState = onOffRecordSwitchState;
}

void checkPlayOnOff() {
	// If not currently recording, check if play button is pushed
	if (!recordEnabled) {
		onOffPlaySwitchState = digitalRead(playPausePin);		// read input

		// If play button pressed, enable play
		if (onOffPlaySwitchState != previousOnOffPlaySwitchState) {
			Serial.println("PLAY PRESSED");
			if (onOffPlaySwitchState == HIGH) {
				playEnabled = !playEnabled;
				Serial.print("play enabled? ");
				Serial.println(playEnabled);
			}
		}
	}
}

void checkRecordOnOff() {
	onOffRecordSwitchState = digitalRead(recordPin);			// read input

	// If record button pressed, enable recording
	if (onOffRecordSwitchState != previousOnOffRecordSwitchState) {
		if (onOffRecordSwitchState == HIGH) {
			Serial.println("RECORD PRESSED");
			// Clear track on recording start
			if (!recordEnabled) {
				//Serial.println("clear track");
				clearTrack(noteIndex);
			}
			// Save track to persistent memory on finish
			else {
				// Load recording as current melody
				loadCurTrack();

				CustomTrack newTrack{
					potZone,	// track zone
					0,			// array of note frequencies
					0			// array of note durations
				};

				// Fill arrays for notes and durations
				for (int i = 0; i < MAX_NOTES; i++) {
					newTrack.notes[i] = notes[i];
					newTrack.noteDurs[i] = noteDurations[i];
				}


				// Store custom track in memory location corresponding with potZone
				EEPROM.put((potZone - 3) * sizeof(CustomTrack), newTrack);
			}

			recordEnabled = !recordEnabled;		// toggle recording mode on/off	
			Serial.print("record enabled? ");
			Serial.println(recordEnabled);
			recordNoteIndex = 0;				// reset note recording index
		}
	}
}

// Record note in custom track and increment timer determining note duration
void recordNote(int note) {
	// On key down
	if (!isKeyDown) {
		isKeyDown = true;
		lastNoteMillis = millis();							// store note start time

		melodies[noteIndex][recordNoteIndex] = note;		// Save note
	}
	// While key is pressed
	else {
		currNoteMillis += millis() - lastNoteMillis;		// increment by difference between last loop and now
		lastNoteMillis = millis();							// store current time
	}
}

// Clear stored notes from custom track when recording initialized
void clearTrack(int track) {
	for (int i = 0; i < MAX_NOTES; i++) {
		melodies[track][i] = 0;			// clear notes
		melodies[track+1][i] = 0;		// clear durations
	}
}

// Load current melody
void loadCurTrack() {
	for (int i = 0; i < MAX_NOTES; i++) {
		notes[i] = melodies[noteIndex][i];
		noteDurations[i] = melodies[durationIndex][i];
	}
}

void loadCustomTrackFromMemory() {
	CustomTrack track1;
	CustomTrack track2;
	CustomTrack track3;

	// Read from memory
	// Look in first CustomTrack bucket for potZone 3 track
	if (EEPROM.read(0) != 255) {
		EEPROM.get(0, track1);
	}
	// Look in first CustomTrack bucket for potZone 4 track
	if (EEPROM.read(sizeof(CustomTrack)) != 255) {
		EEPROM.get(sizeof(CustomTrack), track2);
	}
	// Look in first CustomTrack bucket for potZone 5 track
	if (EEPROM.read(2 * sizeof(CustomTrack)) != 255) {
		EEPROM.get(2 * sizeof(CustomTrack), track3);
	}

	// If CustomTrack was found in persistent memory, load into playable 2D array containing all tracks
	for (int i = 0; i < MAX_NOTES; i++) {
		// PotZone 3
		if (EEPROM.read(0) != 255) {
			melodies[track1.trackZone * 2][i] = track1.notes[i];
			melodies[track1.trackZone * 2 + 1][i] = track1.noteDurs[i];
		}

		// PotZone 4
		if (EEPROM.read(sizeof(CustomTrack)) != 255) {
			melodies[track2.trackZone * 2][i] = track2.notes[i];
			melodies[track2.trackZone * 2 + 1][i] = track2.noteDurs[i];
		}

		// PotZone 5
		if (EEPROM.read(2 * sizeof(CustomTrack)) != 255) {
			melodies[track3.trackZone * 2][i] = track3.notes[i];
			melodies[track3.trackZone * 2 + 1][i] = track3.noteDurs[i];
		}
	}
}


