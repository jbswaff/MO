/*
  Pin 3   =   I, Motion Detector
  Pin 4   =   I, 12mm x 12mm Pushbutton
  Pin 6   =   O, Hood LED's - optional
  Pin 7   =   O, Red LED
  Pin 8   =   O, Red LED
  Pin 9   =   O, Servo
  Pin 11  =   O, Audio Player
  Pin 12  =   O, Buzzer (Used during testing - buzzer activates when not in standby)
  Pin 13  =   O, Onboard LED

  Naming conventions:
  d_*     =   delay
  t_*     =   time / timer
  m_*     =   mode
  _Stat   =   Status - used for variables that read or write I/O

  Code developed by Josh S. for M-O project.

  Code is still a work in progress. Fuctionality not guaranteed, and bugs are likely to become
  evident as it is used more. Please report bugs on thingiverse: https://www.thingiverse.com/thing:3319967
  3D Printable parts can also be download at this link.

  Custization of code should be fairly easy even with no Arduino programming experience.
  Variables below are split into two categories: ones that you should not change and ones that can
  be changed to customize functionality. Default values of these variables have been noted for your
  convenience in case something gets broke. Additional servo positions can easily be added: see
  servo control section for instructions.

  Use serial monitor for debugging. Status gets printed to serial when standby is entered, run mode is
  started, a pushbutton is pressed, or motion is sensed.

*/


#include <Servo.h>

// Set up Pins
const int motion = 3;                     // Pin 3  - Motion
const int pb = 4;                         // Pin 4  - Pushbutton
const int hoodLED = 6;                    // Pin 6  - Hood LEDs     **Must be PWM**
const int redLED1 = 7;                    // Pin 7  - Red LED 1
const int redLED2 = 8;                    // Pin 8  - Red LED 2
const int servo = 9;                      // Pin 9  - Servo         **Must be PWM**
const int plbk_power = 10;                // Pin 10 - Power to playback device
const int playback = 11;                  // Pin 11 - Speaker
const int buzzer = 12;                    // Pin 12 - Buzzer (For testing purposes only)
const int obLED = 13;                     // Pin 13 - Onboard LED

// Set up variables
// Changing these values could cause bugs
int motion_Stat = 0;                      // Stores value of motion detector
int pb_Stat = 0;                          // Stores value of pb
int m_run = 0;                            // Run mode
int ok2run = 0;                           // Okay to run (used for cycle timer)
int ActivePrintSent = 0;                  // Determines if 'active' print has been sent
int hoodBrightness = 0;                   // Brightness of fading hood lights
int redLED1_Stat = 0;                     // Used to set status of red LED 1
int redLED2_Stat = 0;                     // Used to set status of red LED 2
int plbkPulse_Stat = 0;                   // Used to set status of playback pulse
int plbkSent = 0;                         // Playback has been sent this cycle
int armState = 0;                         // Arm state used for future expansion with more than just two arm positions
int curArmPos = 0;                        // Current arm position
int motion_StatPrinted = 0;               // Printed motion detected message to serial
int pb_StatPrinted = 0;                   // Printed pb pressed message to serial
int motionEnPrinted = 0;                  // Printed motion enabled message to serial
int indicator_Stat = 0;                   // Sets status for
int firstRun = 0;                         //
int plbk_Stat = 0;                        // Playback value
int stbyResetAll = 0;


// Set up variables cont'd
// Adjust these parameters as desired
int hoodFadeAmt = 1;                      // Amount of hood light fade per fade cycle                         ==Default 1==
int d_flashRed = 100;                     // Delay between alternating red LEDs (ms)                          ==Default 200==
int d_Fade = 5;                           // Delay for next fade for hood LEDs (ms)                           ==Default 5==
int d_obLEDoff = 9800;                    // Onboard LED flash (standby mode): Time LED is off (ms)           ==Default 9800==
int d_obLEDnext = 10000;                  // Onboard LED flash (standby mode): Total cycle time (ms)          ==Default 10000==
int d_nextServo = 150;                    // Delay before moving to next arm position (ms)                    ==Default 150==
int d_startServo = 3;                     // Delay before starting servos in cycle (s)                        ==Default 3==
int cycleTime = 6;                        // Total Cycle time in run mode (s)                                 ==Default 6==
int plbkPulseTime = 1000;                 // Duration of pulse sent to audio playback unit (ms)               ==Default 1000==
int armPos1 = 35;                         // Arm position 1                                                   ==Default 35==
int armPos2 = 75;                         // Arm position 2                                                   ==Default 75==
int armPos3 = 75;                         // To use additional arm positions, uncomment servo position code.  ==Default 75==
int armHome = 35;                         // Arm home position. This should be below motion sensor.           ==Default 35==
int d_stbySrvOff = 150;                   // Delay before turning servo off in standby mode (ms)              ==Default 150==
long d_stbyMotion = 10;                   // Disable motion in standby mode (for x minutes)                   ==Default 10==
long d_firstRun = 30;                     // Delay motion after first start (s)                               ==Default 60==
int contMotion = 0;                       // 0 = Cycle once even if motion continues; 1 = Cycle until motion stops
int stbyAudioPlayer = 1;                  // 0 = Never turn off audio player; 1 = Turn off audio player in standby



// Set up timers
// Changing these values could cause bugs
unsigned long t_StandbyFlash = 0;         // Timer for Standby Flash
unsigned long t_lastFade = 0;             // Time of last fade (hood fade)
unsigned long lastStandby = 0;            // Time of last standby
unsigned long t_flashRed = 0;             // Timer for red flasher
unsigned long t_lastServo = 0;            // Timer for last servo movement
unsigned long t_lastRunTime = 0;          // Timer after entering standby mode



// Other
Servo drivearm;                           // Set servo as "drivearm"

void setup() {
  pinMode(motion, INPUT);
  pinMode(pb, INPUT);
  pinMode(redLED1, OUTPUT);
  pinMode(redLED2, OUTPUT);
  pinMode(hoodLED, OUTPUT);
  pinMode(plbk_power, OUTPUT);
  pinMode(playback, OUTPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(obLED, OUTPUT);
  drivearm.attach(servo);
  Serial.begin(9600);

  // *** Calculations ***
  d_stbyMotion *= 60000;                  // Convert d_stbyMotion from minutes to milliseconds
  cycleTime *= 1000;                      // Convert cycleTime from seconds to milliseconds
  d_startServo *= 1000;                   // Convert d_startServo from seconds to milliseconds
  d_firstRun *= 1000;                     // Convert d_firstRun from seconds to milliseconds
  firstRun = 1;                           // Set firstRun to 1
}

void loop() {

  // *** Set variables prior to standby ***
  if (contMotion == 1) motion_Stat = digitalRead(motion);     // If cont motion is enabled, read motion & save to motion_Stat
  else motion_Stat = 0;                                       // If cont motion is disabled, set motion_Stat to 0
  pb_Stat = digitalRead(pb);                                  // Read pb, save to pb_Stat
  t_lastRunTime = millis();                                   // Record last run time prior to entering standby
  motionEnPrinted = 0;                                        // Reset variable for 'motion enabled' print while in standby
  m_run = pb_Stat + motion_Stat + ok2run; 
  stbyResetAll = 1;

  if (stbyAudioPlayer == 0) {
    digitalWrite(plbk_power, HIGH);
  }


  // *** Print input status to serial ***
  if (motion_Stat == 1 && motion_StatPrinted == 0) {          // Print status when pb is pressed
    Serial.println("Motion Detected!");                       // ||
    motion_StatPrinted = 1;                                   // ||
  }                                                           // **
  if (pb_Stat == 1 && pb_StatPrinted == 0) {                  // Print status when motion is detected
    Serial.println("Pushbutton Pressed!");                    // ||
    pb_StatPrinted = 1;                                       // ||
  }                                                           // **



  // *************** Standby Mode ********************

  while (m_run == 0) {                                       // Begin standby loop if no input activity & 'cycleTime' has passed

    lastStandby = millis();                                  // Record last standby time
    drivearm.write(armHome);                                 // Return arm to home so that it doesn't block motion sensor
    if (millis() - t_lastRunTime >= d_stbySrvOff)
      drivearm.detach();                                     // Turn off servo pulses after 'd_stbySrvOff' has elapsed

    // *** Turn off outputs ***
    if (stbyResetAll = 1) {
      digitalWrite(redLED1, LOW);                            // Turn off Red LED 1
      digitalWrite(redLED2, LOW);                            // Turn off Red LED 2
      analogWrite(hoodLED, LOW);                             // Turn off hoodLED
      if (millis() - t_lastRunTime >= d_stbySrvOff)
        drivearm.detach();                                   // Turn off servo pulses after 'd_stbySrvOff' has elapsed
      if (stbyAudioPlayer == 1)
        digitalWrite(plbk_power, LOW);

      // *** Reset variables ***
      motion_StatPrinted = 0;                                // Reset print status
      pb_StatPrinted = 0;                                    // Reset print status
      ActivePrintSent = 0;                                   // Return ActivePrintSent to 0
      armState = 0;                                          // Reset arm state
      curArmPos = armHome;                                   // Set servo position to armHome
      stbyResetAll = 0;
    }


    // *** Read inputs from pb and motion ***
    if (millis() >= d_firstRun && firstRun == 1) {           // Enable motion after delay on first boot
      m_run = digitalRead(pb) + digitalRead(motion);         // Monitor pushbutton and motion
      if (motionEnPrinted == 0){
        Serial.print("First Run Elapsed: Motion Enabled. Time: ");
        Serial.println(millis());
        motionEnPrinted = 1;
      }
    }

    if (millis() - t_lastRunTime >= d_stbyMotion && firstRun == 0) {          // Delay motion sensor for sanity's sake
      m_run = digitalRead(pb) + digitalRead(motion);          //
      if (motionEnPrinted == 0) {
        Serial.print("Standby Delay Elapsed: Motion Enabled. Time: ");
        Serial.println(millis());
        motionEnPrinted = 1;
        indicator_Stat = 0;
      }
    }
    else if (firstRun == 0) {                                 // If motion delay has not passed and this is not the first run
      m_run = digitalRead(pb);                                // Monitor pushbutton
    }

    if (millis() <= d_firstRun && firstRun == 1) {            // If this is the first run and d_firstRun has not elapsed
      m_run = digitalRead(pb);                                // Monitor pushbutton
      indicator_Stat = 1;                                     // Turn hoodLEDs solid to indicate we're in startup
    }

    if (ok2run == 0) {                                        // Print status to serial after entering standby
      Serial.print("Entering Standby Mode: Time ");           // ||
      Serial.println(millis());                               // ||
      ok2run = 1;                                             // Set ok2run to 1
    }




    if (millis() >= d_firstRun) {                             // Wait until d_firstRun has passed
      if (millis() - t_StandbyFlash >= d_obLEDoff) {          // Pulse indicator_Stat
        indicator_Stat = 1;

        if (millis() - t_StandbyFlash >= d_obLEDnext) {
          indicator_Stat = 0;
          t_StandbyFlash = millis();
        }

      }

    }


    // *** Write outputs ***
    digitalWrite(hoodLED, indicator_Stat);
    digitalWrite(obLED, indicator_Stat);


  }                                                            // End of Standby Loop


  //*********** BEGIN MAIN LOOP ***********

  digitalWrite(plbk_power, HIGH);

  if (digitalRead(pb) == 1) {                                 // Reset loop timer if pb is pressed
    lastStandby = millis();                                   // ||
    Serial.print("Standby time updated! Time: ");             // ||
    Serial.println(millis());                                 // **
  }

  drivearm.attach(servo);                                     // Turn servo on
  //    drivearm.write(15);                                   // **

  if (ok2run == 1 && ActivePrintSent == 0) {                  //Print to serial when starting run mode
    Serial.print("Entering Active Mode: Time ");              // ||
    Serial.println(millis());                                 // ||
    ActivePrintSent = 1;                                      // **
  }

  if (millis() - lastStandby >= cycleTime) {                  // Run for 'cycleTime' seconds
    ok2run = 0;                                               // 0 = stop run, enable standby
  }                                                           // **



  // *** Set hood brightness ***
  if (millis() - t_lastFade >= d_Fade) {                      // Fade hood LEDs
    hoodBrightness = hoodBrightness + hoodFadeAmt;            // ||
    if (hoodBrightness <= 0 || hoodBrightness >= 255) {       // ||
      hoodFadeAmt = -hoodFadeAmt;                             // ||
    }                                                         // ||
    t_lastFade = millis();                                    // ||
  }                                                           // **


  // *** Flash Red LEDs, alternating ***
  if (millis() - t_flashRed >= d_flashRed) {                  // Alternate red LEDs at interval 'd_flashRed'
    redLED1_Stat = !redLED1_Stat;                             // ||
    redLED2_Stat = !redLED1_Stat;                             // ||
    t_flashRed = millis();                                    // ||
  }                                                           // **


  // Send pulse to playback audio
  if (millis() - lastStandby <= plbkPulseTime) {              // Send pulse to playback devide for duration of 'plbkPulseTime'
    plbk_Stat = 1;                                            // ||
  }                                                           // ||
  else {                                                      // After duration of 'plbkPulseTime', turn off output
    plbk_Stat = 0;                                            // ||
  }                                                           // **


  //************** Servo control ******************* //

  if (millis() - t_lastServo >= d_nextServo) {                // Delay next servo command to give servo time to move
    if (armState == 0 || armState == 2) {                     // Move to armPos1  ****  If you add more servo positions, adjust "armState == 2" to maximum state
      curArmPos = armPos1;                                    // || Set servo position to armPos1
      armState = 1;                                           // || Save current arm state
      goto setArmPos;                                         // || Jump to setArmPos
    }                                                         // ||
    if (armState == 1) {                                      // Move to armPos2
      curArmPos = armPos2;                                    // || Set servo position to armPos2
      armState = 2;                                           // || Save current arm state
      goto setArmPos;                                         // || Jump to setArmPos
    }                                                         // ||
    //        if (armState == 2){                                   // This logic can be scaled indefinitely for more servo positions.
    //          curArmPos = armPos3;                                // || Add 'armPosX = ##;' to list of integers with desired servo position
    //          armState = 3;                                       // || Set armStateX to incrimenting values
    //          goto setArmPos;                                          // || Adjust "if (armState == X);" accordingly
    //        }                                                     // ||
setArmPos:                                                    // setArmPos label
    t_lastServo = millis();                                   // Record last time servo was moved
  }                                                           // **





  //**************  Write Outputs ****************** //

  digitalWrite(buzzer, digitalRead(pb));                      // Turn on buzzer
  analogWrite(hoodLED, hoodBrightness);                       // Turn on fading hood lights
  digitalWrite(playback, plbk_Stat);                          // Send play signal to audio device
  digitalWrite(redLED1, redLED1_Stat);                        //
  digitalWrite(redLED2, redLED2_Stat);
  digitalWrite(obLED, HIGH);

  if (millis() - lastStandby >= d_startServo) {               // Delay servo 'd_startServo' seconds after beginning cycle
    drivearm.write(curArmPos);
  }

  firstRun = 0;







}
