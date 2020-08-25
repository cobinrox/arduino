/***************************************************
 Proud Mary Fish v200815_03
 Play an mp3 file on SD card of DFPlayer.
 Check for start/pause/restart button/logic press.
 Begins playing when button is pressed first time.
 Stops playing when button is pressed next time.
 Starts playing from beginning if pressed again, and so on.
 Move Mouth/Tail/Body motors based on pre-defined pulses.
 Currently pulses are for Proud Mary.
 
 SDCard has one folder, mp3, with one file: 0001.mp3.
 Card should be formatted as FAT32.  In Win10, you can format via
 FileExplorer, RMC, and choose Format and the type of format.
 
 Based in part on
 Durch Wolfgang Ewald modifizierte Version des FullFunction.ino Beispielsketches der DFRobotDFPlayerMini Bibliothek:
 
 DFPlayer - A Mini MP3 Player For Arduino
 <https://www.dfrobot.com/product-1121.html>

 0.1 initial
 0.2 renumber motor pins
 0.3 short timing, add quiet time for mouth
**/

#include "Arduino.h"
#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"
# define ACTIVATED LOW

// motor pins
int TAIL_MOTOR  = 7;
int MOUTH_MOTOR = 5;
int BODY_MOTOR  = 6;

// mp3 player pins
SoftwareSerial mySoftwareSerial(10,11); // RX, TX
DFRobotDFPlayerMini myDFPlayer;

// button pins
int buttonPause = 3;

// global flags
boolean isPlaying = false;
unsigned long gStartTime;  // when button is pressed to ON state

// Start mouth moving at MOUTH_START_AT from beginning of song,
// then alternate on/off every MOUTH_BEAT msec
unsigned long MOUTH_START_AT = 6250;
unsigned long MOUTH_BEAT     = 300; // ms
boolean       gMouthState    = false;
unsigned long gLastMouthTime = millis();
unsigned long mouthStartAt    = MOUTH_START_AT;
const int NUM_MOUTH_QUIET_TIMES = 1*2;
unsigned long MOUTH_QUIET_TIMES[NUM_MOUTH_QUIET_TIMES] = {
                               
                               38000,   11750    // music interlude here
                             };

// Start tail moving at TAIL_START_AT from beginning of song,
// then alternate on/off every TAIL_BEAT msec
unsigned long TAIL_START_AT = 1500; // 5000;
unsigned long TAIL_BEAT     = 200; // ms
boolean       gTailState    = false;
unsigned long gLastTailTime = millis();
unsigned long tailStartAt   = TAIL_START_AT;

// Turn on body at BODY_PULSES[even] times from start of song for
// BODY_PULSES[odd] number of mseconds
boolean       gBodyState    = false;
unsigned long gLastBodyTime = millis();
const int NUM_BODY_PULSES = 12*2;

// For body, turn it on at the pre-determined times for a pre-determined durations
// For Proud Mary, body moves when chorus kicks in
// This is a dual array, the even values represent start time from beginning
// of song (in ms), odd values correspond to corresponding duration (in ms)
unsigned long BODY_PULSES[NUM_BODY_PULSES] = {
                             19750,   800, // chorus rollin
                             22500,   800, // chorus burnin
                             24500,   800, // rollin
                             26750,   800, //
                             29000,   800, //
                             31000,   800,
                             35000,  4000, //deedoop doop doop doop
                             50000,  2000, // cleaned alota plates
                             63000,   800, // rollin chorus
                             66000,800,
                             68000,800,
                             75000,   2000
                             };
// at run time, we update the body_pulses array start time values
// to keep in jive with real time, so this is the variable version
// of the pre-defined BODY_PULSES that you provided above                           
unsigned long  bodyPulses[NUM_BODY_PULSES];
// after each body movement, this idx gets bumped to point to the
// next movement (in the bodyPulses array) in line
int currentBodyIdx = 0;


boolean DEBUG  = true;
boolean MDEBUG = false;
void setup()
{
  // some arduino nano clones require this delay at start
  // up when running off of stand alone power supply or the
  // remainder of the program won't run unless the restart
  // button is hit
  // reference:
  // https://forum.arduino.cc/index.php?topic=515895.0
  delay(1000);
  
  mySoftwareSerial.begin(9600);
  Serial.begin(9600);
  pinMode(buttonPause, INPUT);
  digitalWrite(buttonPause,HIGH);

  pinMode     (TAIL_MOTOR,  OUTPUT);
  digitalWrite(TAIL_MOTOR,  LOW);
  pinMode     (MOUTH_MOTOR, OUTPUT);
  digitalWrite(MOUTH_MOTOR, LOW);
  pinMode     (BODY_MOTOR,  OUTPUT);
  digitalWrite(BODY_MOTOR,  LOW);

  Serial.println();
  Serial.println(F("Greetings. Initializing DFPlayer ... (May take 3~5 seconds)"));
  
  if (!myDFPlayer.begin(mySoftwareSerial)) {  //Use softwareSerial to communicate with mp3.
    Serial.println(F("Unable to begin:"));
    Serial.println(F("1.Please recheck the connection!"));
    Serial.println(F("2.Please insert the SD card!"));
    while(true);
  }
  Serial.println(F("DFPlayer Mini online."));
  
  myDFPlayer.setTimeOut(500); //Set serial communictaion time out 500ms
  
  //----Set different EQ---- (doesn't seem to make any difference)
  //myDFPlayer.EQ(DFPLAYER_EQ_NORMAL);
  //  myDFPlayer.EQ(DFPLAYER_EQ_POP);
    myDFPlayer.EQ(DFPLAYER_EQ_ROCK);
  //  myDFPlayer.EQ(DFPLAYER_EQ_JAZZ);
  //  myDFPlayer.EQ(DFPLAYER_EQ_CLASSIC);
  //  myDFPlayer.EQ(DFPLAYER_EQ_BASS);
  
  //----Set device we use SD as default----
  myDFPlayer.outputDevice(DFPLAYER_DEVICE_SD);

  // set volume
  myDFPlayer.volume(30);
  delay(500);

  // we don't actually start playing the song until user hits
  // the start/stop/continue button

}

void loop(){  
  unsigned long now = millis();
  if( !isPlaying ){
    Serial.print("DEBUG current time/isPlaying: ");Serial.print(isPlaying);Serial.print(" ");Serial.println(now);
  }
  else {
    unsigned long relTime = now - gStartTime;
    Serial.print("DEBUG current time/gtime/relTime ");Serial.print(isPlaying);Serial.print(" ");
    Serial.print(now);Serial.print(" ");
    Serial.print(gStartTime);Serial.print(" ");

    Serial.print(relTime);Serial.println(" ");

  }
  
  // as time goes by, run the motors, if needed
  runTailIfNeeded();
  runMouthIfNeeded();
  runBodyIfNeeded();

  // also check if song is finished
  if (myDFPlayer.available()) {
    int fin = getStatus(myDFPlayer.readType(), myDFPlayer.read());
    if( fin == 666) {
      Serial.println('DONE DONE DONE');
      ///////////myDFPlayer.next();
      isPlaying = false;
      stopAll();
    }
  } else
  {
    // and also check if pause/cont button pressed
    if (digitalRead(buttonPause) == ACTIVATED)
    {
      Serial.println("button press");
      if(isPlaying)
      {
        // if we're already playing the song, then
        // stop playing and stop the motors
        isPlaying = false;
        stopAll();
        
        myDFPlayer.pause();
        delay(500);    
      }else
      {
        // else w00t! start everything up, zero out counters
        // and reset start times relative to real time
        myDFPlayer.next();  //start the mp3
        delay(500);
                      
        unsigned long n = millis();
        gStartTime = n;
        gLastMouthTime = n;
        gLastTailTime  = n;
        gLastBodyTime  = n;
        mouthStartAt = n + MOUTH_START_AT;
        tailStartAt  = n + TAIL_START_AT;
        // for body pulses, update the start times relative
        // to the current time
        for (int i = 0; i < NUM_BODY_PULSES-1; i+=2) {
          bodyPulses[i] = n + BODY_PULSES[i];  // update start time
          bodyPulses[i+1] = BODY_PULSES[i+1];  // transfer over duration as is
        }
        // for debugging
        //for (int i = 0; i < NUM_BODY_PULSES; i++) {
        //  Serial.print(i);Serial.print(": ");Serial.println(bodyPulses[i]);
        //}
        isPlaying = true;
        currentBodyIdx = 0;

      }
      Serial.println("end button logic");
    }
    // if you want/have other buttons
    /*if (digitalRead(buttonNext) == ACTIVATED)
    {
      if(isPlaying)
      {
         myDFPlayer.next();  //Play next mp3
        delay(1000);
      }
    }
  
     if (digitalRead(buttonPrevious) == ACTIVATED)
    {
      if(isPlaying)
      {
        myDFPlayer.previous();  //Play previous mp3
        delay(1000);
      }
    }*/
  }

}
void stopAll() {
  Serial.println("STOPPING ALL");
  digitalWrite( BODY_MOTOR, LOW);
  digitalWrite( MOUTH_MOTOR, LOW);
  digitalWrite( TAIL_MOTOR, LOW);
  gBodyState = false;
  gTailState = false;
  gMouthState = false;
  
}

void runBodyIfNeeded( ) { 
 
  if( !isPlaying ) return;
  if( currentBodyIdx >= NUM_BODY_PULSES ) {
    Serial.println('outta body pulses turn off!');
    if( !gBodyState ) return;
    gBodyState = false;
    digitalWrite(BODY_MOTOR,LOW);mlog("boff");
    return;
  }
  unsigned long now = millis();
  unsigned long timeLapsed = now - gLastBodyTime;

  unsigned long start = bodyPulses[currentBodyIdx];
  unsigned long endt  = start + bodyPulses[currentBodyIdx+1];
  ////////////Serial.print("body i/s/e/n ");Serial.print(currentBodyIdx);Serial.print("/");Serial.print(start);Serial.print("/");Serial.print(endt);Serial.print("/");Serial.println(now);
  
  if( now < start){
    log('not ready for body');
    return;
  }
  
  if( now >= start && now < endt) {
    log("need body to be on");
    if( gBodyState ){ log("but already on!");return;} // already on
    gBodyState = true; 
    digitalWrite( BODY_MOTOR, HIGH);mlog("BON");
    return;
  }
  
  log("need body to be off");
  if(!gBodyState) { log("but already off!");return;}
  gBodyState = false;
  digitalWrite(BODY_MOTOR,LOW);mlog("boff");
  currentBodyIdx+=2;
  return;

}

/**
 * Open and close mouth at specified start time, but also check
 * if we are in the middle of a quiet time for the mouth
 */
void runMouthIfNeeded( ) {
  unsigned long now = millis();
  if( !isPlaying ) return;
  if( now < mouthStartAt) return;
  unsigned long timeLapsed = now - gLastMouthTime;
  //Serial.print("DEBUG timeLapsed: "); Serial.print(timeLapsed);Serial.print(" now: ");Serial.print(now);Serial.print( " last: ");Serial.println(gLastMouthTime);

  if( timeLapsed >= MOUTH_BEAT) {
     //Serial.print('BEAT > timeLapsed');Serial.println(timeLapsed);
     if( gMouthState ){ gMouthState = false;}
     else             { gMouthState = true; }
     if  ( !gMouthState || isQuietMouthTime()  ){digitalWrite( MOUTH_MOTOR, LOW);mlog("moff");}
     else{digitalWrite( MOUTH_MOTOR, HIGH);mlog("MON");}

     gLastMouthTime = now;
  }
}

/**
 * Check mouth quiet time array and see if the current
 * time falls within one of the quiet periods, if so
 * returns true, else false
 */
boolean isQuietMouthTime() {
  unsigned long curTime = millis();
  for(int i =0; i < NUM_MOUTH_QUIET_TIMES; i+=2){
      unsigned long sTime = gStartTime + MOUTH_QUIET_TIMES[i];
      unsigned long eTime = sTime   + MOUTH_QUIET_TIMES[i+1];
      Serial.print("mouth quiet time? i/s/e/n");
      Serial.print(i);    Serial.print("/");
      Serial.print(sTime);Serial.print("/");
      Serial.print(eTime);Serial.print("/");
      Serial.println(curTime);

    if( curTime > sTime && curTime < eTime ){
      Serial.println("should be quiet");

      return true;
    }
  }
  return false;
}


/**
 * Turn tail on and off
 * 
 */
void runTailIfNeeded( ) {
  unsigned long now = millis();
  if( !isPlaying ) return;
  if( now < tailStartAt) return;
  unsigned long timeLapsed = now - gLastTailTime;
  //Serial.print("DEBUG timeLapsed: "); Serial.print(timeLapsed);Serial.print(" now: ");Serial.print(now);Serial.print( " last: ");Serial.println(gLastTailTime);

  if( timeLapsed >= TAIL_BEAT) {
    Serial.print('BEAT > timeLapsed');Serial.println(timeLapsed);
     if( gTailState ){ gTailState = false;}
     else            { gTailState = true; }
     if( gTailState ){digitalWrite( TAIL_MOTOR, HIGH);mlog("TON");}
     else            {digitalWrite( TAIL_MOTOR, LOW);mlog("toff");}
     gLastTailTime = now;
  }
}
void log(char *someStr) {
  if(DEBUG) {
    Serial.print("DEBUG ");Serial.println(someStr);
  }
}
void mlog(char *someStr) {
  if(MDEBUG) {
    Serial.print("MDEBUG ");Serial.println(someStr);
  }
}


/**
 * Return 666 if at end of song/file
 */
int getStatus(uint8_t type, int value){
  int ret = 0;
  switch (type) {
    case TimeOut:
      Serial.println(F("Time Out!"));
      break;
    case WrongStack:
      Serial.println(F("Stack Wrong!"));
      break;
    case DFPlayerCardInserted:
      Serial.println(F("Card Inserted!"));
      break;
    case DFPlayerCardRemoved:
      Serial.println(F("Card Removed!"));
      break;
//    case DFPlayerCardOnline:
//      Serial.println(F("Card Online!"));
//      break;
//    case DFPlayerUSBInserted:
//      Serial.println("USB Inserted!");
//      break;
//    case DFPlayerUSBRemoved:
//      Serial.println("USB Removed!");
//      break;
    case DFPlayerPlayFinished:
      Serial.print(F("Number:"));
      Serial.print(value);
      Serial.println(F(" Play Finished!"));
      ret = 666;
      break;
    case DFPlayerError:
      Serial.print(F("DFPlayerError:"));
      switch (value) {
        case Busy:
          Serial.println(F("Card not found"));
          break;
        case Sleeping:
          Serial.println(F("Sleeping"));
          break;
        case SerialWrongStack:
          Serial.println(F("Get Wrong Stack"));
          break;
        case CheckSumNotMatch:
          Serial.println(F("Check Sum Not Match"));
          break;
        case FileIndexOut:
          Serial.println(F("File Index Out of Bound"));
          break;
        case FileMismatch:
          Serial.println(F("Cannot Find File"));
          break;
        case Advertise:
          Serial.println(F("In Advertise"));
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }  
  return ret;
}
