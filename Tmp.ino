
void testMotors() {
  for (float f = 0.0; f < 1.0; f += 0.1) {
    setMotorRight(f);
    setMotorLeft(f);
    delay(300);
  }
  for (float f = 1.0; f > 0.0; f -= 0.1) {
    setMotorRight(f);
    setMotorLeft(f);
    delay(300);
  }
  for (float f = 0.0; f > -1.0; f -= 0.1) {
    setMotorRight(f);
    setMotorLeft(f);
    delay(300);
  }
  for (float f = -1.0; f < 0.0; f += 0.1) {
    setMotorRight(f);
    setMotorLeft(f);
    delay(300);
  }
  setMotorRight(0.0);
  setMotorLeft(0.0);
}

  static float aX, aY, aZ;

void avgGyro() {
  if (sumCount >= 1000) {
    float aX = ((float) sumX) / ((float) sumCount);
    float aY = ((float) sumY) / ((float) sumCount);
    float aZ = ((float) sumZ) / ((float) sumCount);
    Serial.print(aX); Serial.print("\t"); Serial.print(aY); Serial.print("\t"); Serial.println(aZ);
    sumX = sumY = sumZ = sumCount = 0;
  }
}
