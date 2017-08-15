/***********************************************************************.
 *  FivePotatoe.ino
 *  
 *     This project takes an off-the-shelf robot, Balboa, from Pololu and
 *     adds software mostly derived from the TwoPotatoe self-balancing
 *     robot: http://twopotatoe.net/?page_id=69.  Some of this code 
 *     is directly cribbed from the Pololu Balboa libraries.  
 *     The Balboa libraries are not used directly here since I was never able 
 *     to get them to work with the current FivePotatoe code.
 ***********************************************************************/
#include <avr/interrupt.h>
#include <Arduino.h>
#include <Wire.h>
#include <LSM6.h>

#define BALBOA_32U4_BUTTON_A 14
#define BALBOA_32U4_BUTTON_B 14 //IO_D5
#define BALBOA_32U4_BUTTON_C 17
#define BUTTON_D 5
#define LED_PIN 14

// ------------------------- Tuning variables ------------------------
//  Any changes in things like gear ratios, motors, wheel size, weight distribution, 
//  or changes in performance characteristics, stability, battery voltage etc. will
//  require changes in one or more of the following variables.
const float TICKS_PER_FOOT = 1500.0;
float motorDeadZone = 0.04;    // +- range where motor does not move.  Probably not important.
float motorFpsToInput = 0.163; // Ration of change in speedR/speedL to fps.
float cosFactor = -0.9;         // May need to be changed when there is a change of weight distribution.
float cosTc = 0.98;             // Shouldn't need to be changed.
float angleFactor = 10.0;       // Speed error to pitch angle of FivePotatoe
float fpsFactor = 0.2;          // Angle error to speed adjustment.
float pitchError = 7.0;        // Adjust to have no drift forward or backward.
float motorGainMax = 4.0;       // As high as possible without instability or motor overheating.
float motorGainMin = 0.5;       // Motor gain at zero fps
float motorGainFpsLim = 0.5;    // Start reducing gain below this speed

const float ENC_FACTOR = (float) (1000000 / TICKS_PER_FOOT);  // Change pulse width to milli-fps speed
const long ENC_FACTOR_M = (long) (ENC_FACTOR * 1000.0);  // Change pulse width to milli-fps speed

char message[100] = "";
unsigned long timeMicros = 0UL;
unsigned long timeMillis = 0UL;
volatile long tickPositionRight = 0L;
volatile long tickPositionLeft = 0L;
long tickPosition = 0L;

// FivePotatoe variables
boolean isRunning = false;
boolean isUp = true;
boolean isFast = true;
unsigned long exTime = 0;
int debugInt = 0;
int regStat = 0;

// Controller variables
float controllerX = 0.0;
float controllerY = 0.0;
boolean isRoute = false;
boolean isBluePass = false;

// Imu variables
LSM6 lsm6;
float gyroPitchDel = 0.0;
float timeDriftPitch = 50.0;
float timeDriftRoll = 0.0;
float timeDriftYaw = 15.0;
int gyroReRead = 0;
float aRoll = 0.0;
float aPitch = 0.0;
float gRoll = 0.0;
float yAccel = 0.0;
float gPitch = 0.0;
float gaRoll = 0.0;
float gaPitch = 0.0;
float gyroHeading = 0.0;
float gYaw = 0.0;
float lpfAPitch = 0.0;
float xAccelComp = 20.0;

// Motor variables
volatile unsigned long tickTimeRight = 0UL;
volatile unsigned long tickTimeLeft = 0UL;
volatile long tickPeriodRight = 0L;
volatile long tickPeriodLeft = 0L;
volatile long intrSumRight =0L;
volatile long intrSumLeft =0L;
volatile unsigned int intrCountRight = 0;
volatile unsigned int intrCountLeft = 0;
long wMFpsRight = 0L;
long wMFpsLeft = 0L;
float wFpsRight = 0.0;
float wFpsLeft = 0.0;
float wFps = 0.0;

// TwoPotatoe variables
float lpfCoFps = 0.0;
float lpfCoFpsAccel = 0.0;
float targetWFpsRight = 0.0;
float targetWFpsLeft = 0.0;

// Tmp variables used during testing and development.
float remoteFloat = 0.0;
float debugFloat = 0.0;
int sumX = 0;
int sumY = 0;
int sumZ = 0;
int sumCount = 0;

/***********************************************************************.
 *  setup()
 ***********************************************************************/
void setup() {
  pinMode(13, OUTPUT);
  pinMode(BALBOA_32U4_BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_D, INPUT_PULLUP);
  pinMode(A1, INPUT);
  Serial.begin(115200);
  Serial1.begin(115200);
//  while (!Serial || (millis() < 500));
  beep();
  motorInit();
//testMotors();
  imuInit();
}


/***********************************************************************.
 *  loop()
 ***********************************************************************/
void loop() {  
  static boolean isFirst = true;
  static unsigned long startMicro = 0UL; 
  if (isFirst) {
    isFirst = false;
    startMicro = micros();
  }
  timeMicros = micros() - startMicro;
  timeMillis = timeMicros / 1000;

  if (imu()) {
    if (isUp) twoPotatoe();
    else runDown();
  }
  control();
  logFp();
}



/***********************************************************************.
 *  beep() 
 ***********************************************************************/
void beep() {
  tone(6, 500);
  delay(200);
  tone(6,600);
  delay(200);
  noTone(6);
}



/***********************************************************************.
 *  logFp() All logging and debugging code goes here.  Called ~208/sec
 ***********************************************************************/
void logFp() {
  static unsigned long logTrigger1 = 0UL;
  static char serialChar = 0;
//  avgGyro();

  if (timeMillis > logTrigger1) {
    logTrigger1 += 100; // 10/sec

//    Serial.print(controllerX); Serial.print("\t"); Serial.println(controllerY);
//    Serial.print(gaPitch); Serial.print("\t"); Serial.print(lpfCoFpsAccel); Serial.print("\t"); Serial.println(gYaw);
//    Serial.print(lpfCoFps); Serial.print("\t");Serial.print(lpfCoFpsAccel); Serial.print("\t");Serial.print(gaPitch); Serial.print("\t"); Serial.println();
  }
}

