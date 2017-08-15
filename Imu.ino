/***********************************************************************.
 * imuInit()
 ***********************************************************************/

void imuInit() {
  int success;
  Wire.begin();
  for (int i = 0; i < 100; i++) {
    success = lsm6.init(); 
    if (success) break; 
    Serial.println("IMU initialize failed!");
    resetIMU();
    Wire.begin();
  }
  if (success) Serial.println("IMU Initialized!****************************");

  lsm6.enableDefault();
  lsm6.writeReg(LSM6::INT1_CTRL, 0X02); // Accel data ready on INT1
  lsm6.writeReg(LSM6::INT2_CTRL, 0X01); // Gyro data ready on INT2
  lsm6.writeReg(LSM6::CTRL2_G, 0X5C);   // Gyro 2000fs, 208hz
  lsm6.writeReg(LSM6::CTRL1_XL, 0X50);  // Accel 2g, 208hz

  // Whis is the following necessary?
  delay(10);
  lsm6.readGyro();
  updateGyro();
  lsm6.readAcc();
  updateAccel();
  Serial.print("\t"); Serial.print(gPitch); Serial.print("\t"); Serial.print(gRoll); Serial.print("\t"); Serial.println(gYaw);
  gPitch = gRoll = gYaw = 0.0;
  Serial.print("\t"); Serial.print(gPitch); Serial.print("\t"); Serial.print(gRoll); Serial.print("\t"); Serial.println(gYaw);
}


#define IMU_PERIOD 4807
/***********************************************************************.
 * imu()
 ***********************************************************************/
boolean imu() {
  static unsigned long imuTrigger = 0UL;
  int ret = false;
  if (timeMicros > imuTrigger) { 
    imuTrigger += IMU_PERIOD;
    lsm6.readGyro();
    updateGyro();
    lsm6.readAcc();
    updateAccel();
//    isNewGyro = true;
    ret = true;
  }
  return ret;
}



const float GYRO_SENS = 0.0696;      // Multiplier to get degrees. 
/***********************************************************************.
 * updateGyro()
 ***********************************************************************/
void updateGyro() {
  // Pitch
  float gyroPitchRaw = ((float) lsm6.g.y) - timeDriftPitch;
  float gyroPitchRate = (((float) gyroPitchRaw) * GYRO_SENS);  // Rate in degreesChange/sec
  gyroPitchDel = -gyroPitchRate / 208.0; // degrees changed during period
  gPitch = gPitch + gyroPitchDel;   // Used for debugging
  gaPitch = gaPitch + gyroPitchDel;  // used in weighting final angle
  
  // Roll
  float gyroRollRaw = ((float) lsm6.g.z) - timeDriftRoll;
  float gyroRollRate = (((float) gyroRollRaw) * GYRO_SENS);
  float gyroRollDelta = gyroRollRate / 208.0;
  gRoll = gRoll - gyroRollDelta;
  gaRoll = gaRoll - gyroRollDelta;

  // Yaw
  float gyroYawRaw = ((float) lsm6.g.x) - timeDriftYaw; 
  float gyroYawRate = ((float) gyroYawRaw) * GYRO_SENS;  // Rate in degreesChange/sec
  float gyroYawDelta = -gyroYawRate / 208.0; // degrees changed during period
  gYaw += gyroYawDelta;   //
  float tc = (gYaw > 0.0) ? 180.0 : -180.0;
  int rotations = (int) ((gYaw + tc) / 360.0);
  gyroHeading = gYaw - (((float) rotations) * 360.0);

//  sumX += lsm6.g.x;
//  sumY += lsm6.g.y;
//  sumZ += lsm6.g.z;
//  sumCount++;
}


const float GYRO_WEIGHT = 0.995;   // Weight for gyro compared to accelerometer
const float APITCH_TC = 0.99;
/***********************************************************************.
 * updateAccel()
 ***********************************************************************/
void updateAccel() {
  static float oldLpfAPitch = 0.0;
  
  // Pitch
  float accelX = lsm6.a.z + (xAccelComp * 10000.0 * lpfCoFpsAccel);  // 
//  float accelX = lsm6.a.z;  // 
  aPitch = ((atan2(-accelX, lsm6.a.x)) * RAD_TO_DEG) + pitchError;
  lpfAPitch = (oldLpfAPitch * APITCH_TC) + (aPitch * (1.0 - APITCH_TC));
  oldLpfAPitch = lpfAPitch;
  gaPitch = (gaPitch * GYRO_WEIGHT) + (aPitch * (1 - GYRO_WEIGHT));
  lpfAPitch = (oldLpfAPitch * APITCH_TC) + (aPitch * (1.0 - APITCH_TC));
  oldLpfAPitch = lpfAPitch;

  // Roll
  aRoll =  (atan2(lsm6.a.y, lsm6.a.x) * RAD_TO_DEG);
  gaRoll = (gaRoll * GYRO_WEIGHT) + (aRoll * (1 - GYRO_WEIGHT)); // Weigh factors
}



/**************************************************************************.
 * resetIMU()  From: https://forum.arduino.cc/index.php?topic=386269.0
 *             I2C clocks to make sure no slaves are hung in a read
 *             at startup
 *             
 *             This is left over from TwoPotatoe Due code and is probably
 *             not needed here.
 **************************************************************************/
void resetIMU() {
  // Issue 20 I2C clocks to make sure no slaves are hung in a read
  pinMode(20, OUTPUT);
  pinMode(21, OUTPUT);
  pinMode(70, OUTPUT);
  pinMode(71, OUTPUT);
  digitalWrite(20, LOW);
  digitalWrite(70, LOW);
  for (int i = 0; i < 1000; i++)
  {
    digitalWrite(21, LOW);
    digitalWrite(71, LOW);
    delayMicroseconds(10);
    digitalWrite(21, HIGH);
    digitalWrite(71, HIGH);
    delayMicroseconds(10);
  }
}


