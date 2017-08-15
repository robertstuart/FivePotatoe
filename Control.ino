
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


void control() {
  readSerial();
  blinkLeds();
  fall();
  switches();
  state();
  sendBlu();
}

/**************************************************************************.
 * readSerial()
 *   
 **************************************************************************/
void readSerial() {
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
 * state()
 *         If change to running state, set pitch to current reading from
 *         the accelerometer to have stable state at button press.  
 **************************************************************************/
void state() {
  static boolean oldIsRunning = false;
  if (isRunning && !oldIsRunning) {
    gPitch = gaPitch = lpfAPitch;
  }
  oldIsRunning = isRunning;
}


#define BOUNCE_TIME 50
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

  if (timeMilliseconds <= BOUNCE_TIME) return;
  
  // Debounce A
  boolean buttonA = digitalRead(BALBOA_32U4_BUTTON_A) == LOW;
  if (buttonA) timerA = timeMilliseconds;
  if ((timeMilliseconds - timerA) > BOUNCE_TIME) stateA = false;
  else stateA = true;

  // Debounce D
  boolean buttonD = digitalRead(BUTTON_D) == LOW;
  if (buttonD) timerD = timeMilliseconds;
  if ((timeMilliseconds - timerD) > BOUNCE_TIME) stateD = false;
  else stateD = true;

  // A press transition
  if (stateA && (!oldStateA)) {
    tone(6, 1000);
    delay(100);
    noTone(6);
    delay(100);
    tone(6,1000);
    delay(100);
    noTone(6);
    
  }

  
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
  if (timeMilliseconds > blinkTrigger) {
    blinkTrigger = timeMilliseconds + 200;
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
    isUpTime = timeMilliseconds;
    isUp = true;
  }
  if ((timeMilliseconds - isUpTime) > 100) {
    if (isUp == true) isRunning = false; 
    isUp = false;
  }
}



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



void sendBlu() {
  static unsigned long bluTrigger = 0UL;
  if (isRunning) buttonStatus |= (1 << 0); 
  else           buttonStatus &= ~(1 << 0);
  if (isFast)    buttonStatus |= (1 << 1); 
  else           buttonStatus &= ~(1 << 1);
  if (timeMilliseconds > bluTrigger) {
    bluTrigger = timeMilliseconds + 250;
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

