 float aX, aY, aZ;

void avgGyro() {
  if (sumCount >= 1000) {
    float aX = ((float) sumX) / ((float) sumCount);
    float aY = ((float) sumY) / ((float) sumCount);
    float aZ = ((float) sumZ) / ((float) sumCount);
    Serial.print(aX); Serial.print("\t"); Serial.print(aY); Serial.print("\t"); Serial.println(aZ);
    sumX = sumY = sumZ = sumCount = 0;
  }
}
