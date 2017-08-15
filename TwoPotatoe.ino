/***********************************************************************.
 *  TwoPotatoe.ino
 *  
 *     This file contains most of the algorithms for balancing and
 *     steering FivePotatoe.  It is a simplified and easier to 
 *     understand version of the algorithm developed for TwoPotatoe.
 *     A more complete-but still inadequate-explanation is given here: 
 *         http://twopotatoe.net/?page_id=69
 ***********************************************************************/
const float Y_INC = 0.05;
void twoPotatoe() {
  static float lpfCoFpsOld = 0.0;
  static float controllerFps = 0.0;
  
  readFps();
 
  // Compute Center of Oscillation speed (cos)
  float rotation = -gyroPitchDel * cosFactor;  // 4.5
  float coFps = wFps + rotation;
  lpfCoFps = (lpfCoFpsOld * cosTc) + (coFps * (1.0 - cosTc)); // large values give slow response
  lpfCoFpsAccel = lpfCoFps - lpfCoFpsOld; // Used to compensate for effect of acceleration on aPitch
  lpfCoFpsOld = lpfCoFps;

  // Get the controller speed and mooth it out.
  float newFps = controllerY * 5.0;
  if (!isFast) newFps *= 0.4;
  if (newFps > controllerFps) controllerFps += Y_INC;
  else if (newFps < controllerFps) controllerFps -= Y_INC;
  else controllerFps = newFps;

  // Find the speed error.  Constrain rate of change.
  float fpsError = controllerFps - lpfCoFps;

  // compute a weighted angle to eventually correct the speed error
  float targetAngle = -(fpsError * angleFactor); 
  targetAngle = constrain(targetAngle, -20.0, 20.0);

  // Compute angle error and weight factor.
  float angleError = targetAngle - gaPitch;
  float fpsCorrection = angleError * fpsFactor; 

  // Add the angle error to the base speed to get the target wheel speed.
  float targetWFps = fpsCorrection + lpfCoFps;

  // Set the steering values.
  float steeringGain = 0.4;   // Controls how fast it turns at low speed
  float steeringAtMax = 0.2;  // Controls how fast it turns at top speed
  float speedAdjustment = (((1.0 - abs(controllerY)) * steeringGain) + steeringAtMax) * controllerX; 
  targetWFpsRight = targetWFps - speedAdjustment;
  targetWFpsLeft = targetWFps + speedAdjustment;

  checkMotorRight();
  checkMotorLeft();
}


/***********************************************************************.
 *  runDown() Run TwoPotatoe while lying on the ground.  With a little 
 *            skill and new batteries, FivePotatoe can be brought
 *            to an upright position.
 ***********************************************************************/
void runDown() {
  float targetWFps = 6.0 * controllerY;
  float speedAdjustment = (((1.0 - abs(controllerY)) * 0.4) + 0.2) * controllerX; 
  targetWFpsRight = targetWFps - speedAdjustment;
  targetWFpsLeft = targetWFps + speedAdjustment;

  readFps();
  checkMotorRight();
  checkMotorLeft();
}

