
const int MAX_IN_BUF = 40;
const int MAX_BLU_BUF = 6;
const char STX = 0x02;
const char ETX = 0x03;

char bluBuf[MAX_BLU_BUF];
int bluBufPtr = 0;
char inBuf[MAX_IN_BUF];
int inBufPtr = 0;

byte buttonStatus = 0;
String displayStatus = "xxxx";           // message to Android device


/**************************************************************************.
 * control()  Called frequently.  There are some 2ms gaps whenever
 *            the IMU is being read.
 **************************************************************************/
void control() {
  readSerial();
  blinkLeds();
  fall();
  switches();
  playBeep();
  state();
  sendBlu();
}

/**************************************************************************.
 * readSerial() 1. Reads bluetooth to get commands from the Android app
 *                 "Joystick bluetooth Commander":
 *                 https://play.google.com/store/apps/details?id=org.projectproto.btjoystick&hl=en
 *              2. Reads input from the USB seriol port to get values entered there.
 *                 Currently, values are entered to adjust tuning parameters.   
 **************************************************************************/
void readSerial() {

  // Read the bluetooth to get command from the Andriod tablet app 
  while (Serial1.available() > 0) {
    static boolean isMsgInProgress = false;
    char b = Serial1.read();
    if (isBluePass) { //Bluetooth passthrough?
      Serial.write(b);
    }  else {
      if (isMsgInProgress) {
        if (b == ETX) {
          if (bluBufPtr == 1) {
            setButtonState(bluBuf[0]);
          } else if (bluBufPtr == 6) {
            int x = (bluBuf[0]-48)*100 + (bluBuf[1]-48)*10 + (bluBuf[2]-48);   // obtain the Int from the ASCII representation
            int y = (bluBuf[3]-48)*100 + (bluBuf[4]-48)*10 + (bluBuf[5]-48);
            if ((x >= 100) && (x <= 300) && (y >= 100) && (y <= 300)) {
              controllerX = ((float) (x - 200)) / 100.0;
              controllerY = ((float) (y - 200)) / 100.0;
            }
          } else {
            Serial.print(b, HEX); Serial.print(" "); // Error
          }
          isMsgInProgress = false;
        } else if (bluBufPtr < 7) {
          bluBuf[bluBufPtr++] = b;
        } else {
          isMsgInProgress = false;
          Serial.print("x1"); Serial.print(" "); // Error
       }
      } else if (b ==STX){
        isMsgInProgress = true;
        bluBufPtr = 0;
      } else {
        Serial.print("-"); Serial.print(b, HEX); Serial.print(" "); // Error
      }
    }
  }
  
  // Read the serial input from the USB port
  while (Serial.available() > 0) {
    char b = Serial.read();
    if (isBluePass) { //Bluetooth passthrough?
      Serial1.write(b);
    } else {
      if (((int) b) == 10) {
        inBuf[inBufPtr] = 0;
        doMsg();
        inBufPtr = 0;
      } else {
        inBuf[inBufPtr] = b;
        if (inBufPtr < MAX_IN_BUF) inBufPtr++;
      }
    }
  }
} 

void doMsg() {
  remoteFloat = atof(inBuf);
  Serial.print(remoteFloat); Serial.println("X");
  xAccelComp = remoteFloat;
}



/**************************************************************************.
 * state() If change to running state, set pitch to current reading from
 *         the accelerometer to have stable state at button press.  
 **************************************************************************/
void state() {
  static boolean oldIsRunning = false;
  if (isRunning && !oldIsRunning) {
    gPitch = gaPitch = lpfAPitch;
  }
  oldIsRunning = isRunning;
}


static int BOUNCE_TIME = 50;
/**************************************************************************.
 * switches()
 *   
 **************************************************************************/
void switches() {
  static unsigned long timerA = 0;
  static boolean stateA = false;
  static boolean oldStateA = false;
  
  static unsigned long timerD = 0;
  static boolean stateD = false;
  static boolean oldStateD = false;

  if (timeMillis <= BOUNCE_TIME) return;
  
  // Debounce A
  boolean buttonA = digitalRead(BALBOA_32U4_BUTTON_A) == LOW;
  if (buttonA) timerA = timeMillis;
  if ((timeMillis - timerA) > BOUNCE_TIME) stateA = false;
  else stateA = true;

  // Debounce D
  boolean buttonD = digitalRead(BUTTON_D) == LOW;
  if (buttonD) timerD = timeMillis;
  if ((timeMillis - timerD) > BOUNCE_TIME) stateD = false;
  else stateD = true;

  // A press transition. Does nothing at the moment
  if (stateA && (!oldStateA)) {
    tone(6, 1000);
    delay(100);
    noTone(6);
    delay(100);
    tone(6,1000);
    delay(100);
    noTone(6);
  }

  // D press & release transition.  Change running state.
  if (stateD && !oldStateD) {         // D press transition
  }
  if (!stateD && oldStateD) {          // D release transition
    isRunning = !isRunning;
  }

  oldStateA = stateA;
  oldStateD = stateD;
}



/***********************************************************************.
 *  blinkLeds()
 ***********************************************************************/
 void blinkLeds() {
  static unsigned long blinkTrigger = 0UL;
  static boolean blinkToggle = true;
  if (timeMillis > blinkTrigger) {
    blinkTrigger = timeMillis + 200;
    blinkToggle = !blinkToggle;
    digitalWrite(13, (blinkToggle) ? HIGH : LOW);
  }
}



/***********************************************************************.
 *  fall() 
 ***********************************************************************/
void fall() { 
//  static boolean isUpOld = false;
  static unsigned long isUpTime = 0;
  if ((abs(gaRoll) < 65.0) && (abs(gaPitch) < 65.0)) {
    isUpTime = timeMillis;
    isUp = true;
  }
  if ((timeMillis - isUpTime) > 100) {
    if (isUp == true) isRunning = false; 
    isUp = false;
  }
}



/***********************************************************************.
 *  getButtonStatusString() 
 ***********************************************************************/
String getButtonStatusString()  {
  String bStatus = "";
  for(int i=0; i<6; i++) {
    if(buttonStatus & (B100000 >>i))   bStatus += "1";
    else                               bStatus += "0";
  }
  return bStatus;
}


/***********************************************************************.
 *  setButtonState() 
 ***********************************************************************/
void setButtonState(int bStatus) {
  switch (bStatus) {
     case 'A':        // --- BUTTON #1 ---
     isRunning = true;
     break;
   case 'B':
     isRunning = false;
     break;
   case 'C':          // --- BUTTON #2  --- 
     isFast = true;
     break;
   case 'D':
     isFast = false;
     break;
   case 'E':          // --- BUTTON #3 ---
     beep(honk);
     break;
   case 'F':
     break;
   case 'G':          // --- BUTTON # 4  ---
     break;
   case 'H':
    break;
   case 'I':           // --- BUTTON # 5 ---
     cosFactor += 0.1;
     break;
   case 'J':
     break;
   case 'K':           // ---  BUTTON #6  ---
     cosFactor -= 0.1;
     break;
   case 'L':
     break;
  }
}



/***********************************************************************.
 *  sendBlu() Send status to tablet 4/sec.
 ***********************************************************************/
void sendBlu() {
  static unsigned long bluTrigger = 0UL;
  
  if (timeMillis > bluTrigger) {
    bluTrigger = timeMillis + 250;

    if (isRunning) buttonStatus |= (1 << 0); 
    else           buttonStatus &= ~(1 << 0);
    if (isFast)    buttonStatus |= (1 << 1); 
    else           buttonStatus &= ~(1 << 1);

    // Data frame transmitted back from Arduino to Android device:
    // < 0X02   Buttons state   0X01   DataField#1   0x04   DataField#2   0x05   DataField#3    0x03 >  
    // < 0X02      "01011"      0X01     "120.00"    0x04     "-4500"     0x05  "Motor enabled" 0x03 >    // example

    Serial1.print(STX);                                                         // Start of Transmission
    Serial1.print(getButtonStatusString());  Serial1.print((char)0x1);          // buttons status feedback
//    Serial1.print(((float) tickPosition) / TICKS_PER_FOOT, 1);       Serial1.print((char)0x4);                    // datafield #1
    Serial1.print(cosFactor);       Serial1.print((char)0x4);                    // datafield #1
    Serial1.print(gYaw,0);    Serial1.print((char)0x5);                  // datafield #2
    Serial1.print(((float) analogRead(A1)) * 0.0145, 1); Serial1.print(" V");   // datafield #3
    Serial1.print(ETX);                                                         // End of Transmission
  }
}



/***********************************************************************.
 *  playBeep() Simple routine to play notes by the buzzer.  Maybe someday
 *             someone will adapt the Pololu buzzer routines to replace this.
 ***********************************************************************/
void playBeep() {
  static unsigned long playTrigger = 0;
  if (isBeeping && (timeMillis > playTrigger)) {

    // Play the next note
    int freq = beepSequence[beepPtr++];
    if (freq == 0) {
      isBeeping = false;
      noTone(BUZZER_PIN);  Serial.println("off");
      return;
    }
     playTrigger = timeMillis + beepSequence[beepPtr++];
     tone(BUZZER_PIN, freq);
     Serial.println(freq);
  }
}

void beep(int seq[]) {
  beepPtr = 0;
  beepSequence = seq;
  isBeeping = true;
}

