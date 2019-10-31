/* Attack Store Release mechanism
 by Xavier Villa
 modified March 24, 2019
*/

#define MAX_NUM_ATTACK_STORES 12 // max number of attack stores being used
#define L_INTERRUPT_PIN 1        // input pin for the interrupter
#define R_INTERRUPT_PIN 2        // input pin for the interrupter
#define L_MOTOR_PIN 5            // output pin for motor to turn on
#define R_MOTOR_PIN 6            // output pin for motor to turn on
#define TRIGGER 7                // (D9) input pin for triggering bomb drop

int attackStoresRemaining = MAX_NUM_ATTACK_STORES;            // variable to maintain the servo positions
unsigned long triggerPos = 0;                                 // variable to maintain the button positions
unsigned long currTriggerPos = 0;                             // variable to maintain the button positions
uint8_t lCount = 0;                                           // counts the num of left bombs dropped
uint8_t rCount = 0;                                           // counts the num of right bombs dropped
int lRodCalibration[] = {8, 8, 7, 8, 7, 8}; //done for left   // Adjust these from first to last, to get spacing of each movement correct
int rRodCalibration[] = {9, 7, 8, 6, 5, 7};                   // Adjust these from first to last, to get spacing of each movement correct

void setup() {
  Serial.begin(9600); // set up Serial library at 9600 bps
  pinMode(L_MOTOR_PIN, OUTPUT);
  pinMode(R_MOTOR_PIN, OUTPUT);
  pinMode(TRIGGER, INPUT);
}

void loop() {
  // assumes all attack stores are loaded on boot
  delay(30000);
  
  // get current trigger position
  triggerPos = pulseIn(TRIGGER, HIGH, 10000000UL);
  currTriggerPos = triggerPos;

  // indicate readiness to serial port
//  Serial.print(MAX_NUM_ATTACK_STORES); Serial.println(" attack stores loaded!"); Serial.println("Ready!");
  
  // do while there are still attack stores to drop
  while(attackStoresRemaining > 0)
  {
    // wait for input (when flip switched to drop bomb)
    while(true){
      currTriggerPos = pulseIn(TRIGGER, HIGH, 10000000UL);
      if((currTriggerPos > triggerPos) && (currTriggerPos-triggerPos > 500)){ triggerPos = currTriggerPos; break; }
      else if((triggerPos > currTriggerPos) && (triggerPos-currTriggerPos > 500)){ triggerPos = currTriggerPos; break; }
    }

    // start turning the left motor until the left optointerupter counts X teeth
//    Serial.println("Dropping left bomb!");
    digitalWrite(L_MOTOR_PIN, HIGH);
    killMotorAfterXPeaks('l', lRodCalibration[lCount]);
    attackStoresRemaining--;
    lCount++;
//    Serial.println("Left bomb dropped!"); Serial.print(attackStoresRemaining); Serial.println(" bombs remaining.");
    
    // check to make sure there are still more to drop, or exit if not (this handles odd number of MAX_NUM_ATTACK_STORES)
    if(attackStoresRemaining==0){ break; }
    
     // wait for input (when flip switched to drop bomb)
    while(true){
      currTriggerPos = pulseIn(TRIGGER, HIGH, 10000000UL);
      if((currTriggerPos > triggerPos) && (currTriggerPos-triggerPos > 500)){ triggerPos = currTriggerPos; break; }
      else if((triggerPos > currTriggerPos) && (triggerPos-currTriggerPos > 500)){ triggerPos = currTriggerPos; break; }
    }
    
    // start shifting the left attack store until the left optointerupter counts X teeth
//    Serial.println("Dropping right bomb!");
    digitalWrite(R_MOTOR_PIN, HIGH);
    killMotorAfterXPeaks('r', rRodCalibration[rCount]);
    rCount++;
    attackStoresRemaining--;
//    Serial.println("Right bomb dropped!"); Serial.print(attackStoresRemaining); Serial.println(" bombs remaining.");
  }
  
  // program is done, reset the bombs
}

unsigned short int avgSample(char side, uint8_t numSamples){
    // returns an average value across a sample size of analog readings for the photo interrupter on either wing side

    // pick the pin to probe
    uint8_t interruptPin = 0;
    if (side == 'l') { interruptPin = L_INTERRUPT_PIN; }
    else { interruptPin = R_INTERRUPT_PIN; }
    // do the thing
    unsigned int sum = 0;
    for (uint8_t i = 0; i < numSamples; i++){
      //Serial.println(analogRead(interruptPin));
      sum += analogRead(interruptPin);;
    }
//Serial.print("Avg: ");
//Serial.println(sum/numSamples);
    return sum/numSamples;
}

void killMotorAfterXPeaks(char side, int maxPeaks){
    // This code will turn off the attack store motor after a certain number of rising and falling teeth on the attack store gear

    // Configurable numbers
    unsigned short int mid = 750;    // This is the middle range of our signal
    unsigned short int thr = 200;    // the number of values between our high/low thresholds
    uint8_t numSamples = 2;         // The quantity of samples to average (powers of 2)

    // Initial conditions
    bool isHigh;
    unsigned short int trigger;
    if(avgSample(side, numSamples) > mid){
         isHigh = true;
         trigger = mid - thr/2;
    }
    else{
        isHigh = false;
        trigger = mid + thr/2;
    }

    // I think this is called a schmitt trigger? It just tries to keep the arduino from counting the same peak twice
    uint8_t peaks = 0;
    unsigned long startTime;
    unsigned long currentTime;
    startTime = millis();
    while(peaks < maxPeaks)
    {
        if (isHigh == true){
            if(avgSample(side, numSamples) < trigger){
                isHigh = false;
                peaks++;
                trigger = mid+thr/2;
            }
        }
        else{
            if(avgSample(side, numSamples) > trigger){
                isHigh = true;
                peaks++;
                trigger = mid-thr/2;
            }
        }
        currentTime = millis();
        if(abs(currentTime-startTime) > 2000){
//          Serial.println("Motor timed out. Check for Jams!");
          break;
        }
    }
//    Serial.println("Schmitty done");
    //Now turn off the motor on correct wing
    if (side == 'l') { digitalWrite(L_MOTOR_PIN, LOW); }
    else { digitalWrite(R_MOTOR_PIN, LOW); }
}
