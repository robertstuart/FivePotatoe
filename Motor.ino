#define LEFT_XOR   8
//#define LEFT_B     31  // IO_E2
#define RIGHT_XOR  7
#define RIGHT_B    23

#define PWM_L 10
#define PWM_R 9
#define DIR_L 16
#define DIR_R 15

volatile boolean oldLeftB;
volatile boolean oldRightB;

volatile boolean errorLeft;
volatile boolean errorRight;

/***********************************************************************.
 *  rightIsr() Right encoder interrupt service.
 ***********************************************************************/
void rightIsr() {
  boolean isFwd;
  boolean rightB = PINF & (1 << PINF0);    // RIGHT_B, Pin 23
  boolean rightX = PINE & (1 << PINE6);    // RIGHT_XOR, Pin 7

  if (rightB == oldRightB) {          // B state change?    
    if (rightX) isFwd = false;                  
    else        isFwd = true;
  } else {                                
    if (rightX)  isFwd = true;
    else         isFwd = false;
  }

  oldRightB = rightB;
  unsigned long lastTickTime = tickTimeRight;
  tickTimeRight = micros();
  if (isFwd) {
    tickPeriodRight = ((long) tickTimeRight) - ((long) lastTickTime);
    tickPositionRight++;
  } else {
    tickPeriodRight = ((long) lastTickTime) - ((long) tickTimeRight);
    tickPositionRight--;
  }
  intrSumRight += tickPeriodRight;
  intrCountRight++;
}

/***********************************************************************.
 *  ISR() Left encoder interrupt service.
 *        This is magic.
 ***********************************************************************/
ISR(PCINT0_vect) {
  boolean isFwd;
  boolean leftB = PINE & (1 << PINE2);   // LEFT_B, pin ?, 
  boolean leftX = PINB & (1 << PINB4);   // LEFT_XOR, Pin 8
  
  if (leftB == oldLeftB) {          // B state change?    
    if (leftX)  isFwd = false;                  
    else        isFwd = true;
  } else {                                
    if (leftX)   isFwd = true;
    else         isFwd = false;
  }
    
  oldLeftB = leftB;
  unsigned long lastTickTime = tickTimeLeft;
  tickTimeLeft = micros();
  if (isFwd) {
    tickPeriodLeft = ((long) tickTimeLeft) - ((long) lastTickTime);
    tickPositionLeft++;
  } else {
    tickPeriodLeft = ((long) lastTickTime) - ((long) tickTimeLeft);
    tickPositionLeft--;
  }
  intrSumLeft += tickPeriodLeft;
  intrCountLeft++;
}



/***********************************************************************.
 *  motorInit() 
 ***********************************************************************/
void motorInit() {
  pinMode(LEFT_XOR, INPUT_PULLUP);
  DDRE = DDRE | B00000100;    // LEFT_B data direction register
  PORTE = PORTE | B00000100;  // LEFT_B input pullup
  pinMode(RIGHT_XOR, INPUT_PULLUP);
  pinMode(RIGHT_B, INPUT_PULLUP);

  // Enable pin-change interrupt on PB4 for left encoder, and disable other
  // pin-change interrupts.
  PCICR = (1 << PCIE0);
  PCMSK0 = (1 << PCINT4);
  PCIFR = (1 << PCIF0);  // Clear its interrupt flag by writing a 1.
  
  attachInterrupt(digitalPinToInterrupt(7), rightIsr, CHANGE);

    pinMode(PWM_L, OUTPUT);
    pinMode(PWM_R, OUTPUT);
    pinMode(DIR_L, OUTPUT);
    pinMode(DIR_R, OUTPUT);
    digitalWrite(PWM_L, LOW);
    digitalWrite(PWM_R, LOW);
    digitalWrite(DIR_L, LOW);
    digitalWrite(DIR_R, LOW);

    // Timer 1 configuration
    // prescaler: clockI/O / 1
    // outputs enabled
    // phase-correct PWM
    // top of 400
    //
    // PWM frequency calculation
    // 16MHz / 1 (prescaler) / 2 (phase-correct) / 400 (top) = 20kHz
    TCCR1A = 0b10100000;
    TCCR1B = 0b00010001;
    ICR1 = 400;
    OCR1A = 0;
    OCR1B = 0;
}

void readFps() {
  readSpeedRight();
  readSpeedLeft();
  wFps = (wFpsLeft + wFpsLeft) / 2.0;
  tickPosition = (tickPositionLeft + tickPositionRight) / 2;
}



#define GAIN_FPS_LIM = 0.5;
#define GAIN
/**************************************************************************.
 * checkMotor????()  
 **************************************************************************/
void checkMotorRight() {
  float gain = motorGainMax;

  float wsError = (float) (targetWFpsRight - wFpsRight);       // Wheel speed error
  if (abs(targetWFpsRight) < motorGainFpsLim) {  // reduce gain below motorGainFpsLim
    gain = motorGainMin + (abs(targetWFpsRight) * (motorGainMax - motorGainMin) * (1.0 / motorGainFpsLim));
  }
  float wsTarget = targetWFpsRight + (wsError * gain);  // Target speed to correct error
  float pw = abs(wsTarget * motorFpsToInput) + motorDeadZone;            // Pw for the target.
  if (pw <= motorDeadZone) pw = 0; 
  if (wsTarget < 0.0)  pw = -pw;;
  setMotorRight(pw);
}

void checkMotorLeft() {
  float gain = motorGainMax;

  float wsError = (float) (targetWFpsLeft - wFpsLeft);       // Wheel speed error
  if (abs(targetWFpsRight) < motorGainFpsLim) {  // reduce gain below .5 fps
    gain = motorGainMin + (abs(targetWFpsLeft) * (motorGainMax - motorGainMin) * (1.0 / motorGainFpsLim));
  }
  float wsTarget = targetWFpsLeft + (wsError * gain);  // Target speed to correct error
  float pw = abs(wsTarget * motorFpsToInput) + motorDeadZone;            // Pw for the target.
  if (pw <= motorDeadZone) pw = 0; 
  if (wsTarget < 0.0)  pw = -pw;;
  setMotorLeft(pw);
}



/***********************************************************************.
 *  readSpeed????()
 ***********************************************************************/
void readSpeedRight() {
   noInterrupts();
  long sum = intrSumRight;
  long count = (long) intrCountRight;
  intrSumRight = 0L;
  intrCountRight = 0;
  interrupts();
  if (count == 0) {
    long newMFps = ENC_FACTOR_M / (micros() - tickTimeRight);
    if (wMFpsRight > 0) {
      if (newMFps < wMFpsRight) wMFpsRight = newMFps; // Set new if lower
    } else {
      if (newMFps > wMFpsRight) wMFpsRight = -newMFps; // Set new if lower
    } 
  } else {
    wMFpsRight = ENC_FACTOR_M / (sum / count);
  }
  wFpsRight = ((float) wMFpsRight) / 1000.0;
}
void readSpeedLeft() {
   noInterrupts();
  long sum = intrSumLeft;
  long count = (long) intrCountLeft;
  intrSumLeft = 0L;
  intrCountLeft = 0;
  interrupts();
  if (count == 0) {
    long newMFps = ENC_FACTOR_M / (micros() - tickTimeLeft);
    if (wMFpsLeft > 0) {
      if (newMFps < wMFpsLeft) wMFpsLeft = newMFps; // Set new if lower
    } else {
      if (newMFps > wMFpsLeft) wMFpsLeft = -newMFps; // Set new if lower
    } 
  } else {
    wMFpsLeft = ENC_FACTOR_M / (sum / count);
  }
  wFpsLeft = ((float) wMFpsLeft) / 1000.0;
}



/***********************************************************************.
 *  setMotor????() Set the pwm.  1.0 = max pwm, -1.0 = max pwm in reverse.
 ***********************************************************************/
void setMotorRight(float speedR) {
  if (!isRunning) speedR = 0.0;
  speedR = constrain(speedR, -1.0, 1.0);
  digitalWrite(DIR_R, (speedR > 0.0) ? LOW : HIGH);
  OCR1A = (int) (abs(speedR) * 399.9);
}
void setMotorLeft(float speedL) {
  if (!isRunning) speedL = 0.0;
  speedL = constrain(speedL, -1.0, 1.0);
  digitalWrite(DIR_L, (speedL > 0.0) ? LOW : HIGH);
  OCR1B = (int) (abs(speedL) * 399.9);
}


