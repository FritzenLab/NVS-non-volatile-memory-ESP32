
// reference: https://dronebotworkshop.com/esp32-non-volatile-storage/
#define button 20
#define LED 14
#include <Preferences.h>
#include <esp_system.h> // Required for esp_random()
#include <WiFi.h> // Necessary to generate TRNG

Preferences prefs;

volatile unsigned long debounceTimer = 0;
volatile unsigned long ledTimer = 0;
volatile int pressCount = 0;
const int ledInterval = 300;
volatile int numberOfPresses = 0;
volatile int decideNow = 0;
volatile uint16_t randomVar = 0;
volatile int randomNumber = 0;
volatile unsigned long pressTimer = 0;

// this struct "compacts" each variable below in a byte, single bit each,
// which takes less space than bool itself, which uses 1 byte per variable.
struct MyBitStruct {
  bool ledState   : 1;
  bool buttonStatus : 1;
  bool lastButton   : 1;
  bool memoryControl : 1;
};

MyBitStruct singlebit;

// Returns a random number in the range [0, bound)
// Avoids modulo bias using rejection sampling
uint32_t randomBounded(uint32_t bound) {
  if (bound == 0) return 0;  // avoid division by zero

  uint32_t x;
  uint32_t limit = UINT32_MAX - (UINT32_MAX % bound);

  do {
    x = esp_random();
  } while (x >= limit);  // Retry until unbiased

  return x % bound;
}

// Returns a random number in the range [minVal, maxVal)
uint32_t randomRange(uint32_t minVal, uint32_t maxVal) {
  if (maxVal <= minVal) return minVal;  
  return minVal + randomBounded(maxVal - minVal);
}

void setup(){
  
  pinMode(button, INPUT_PULLUP);
  pinMode(LED, OUTPUT);
  prefs.begin("nv-memory", false);
  Serial.begin(115200);  
  // Uses WiFi for random number generation only
  WiFi.mode(WIFI_MODE_STA);   // or WIFI_STA
  WiFi.begin();               // starts WiFi driver (no need to connect)

  // Optional: immediately stop scanning/connecting attempts
  WiFi.disconnect(true);
  singlebit.lastButton = digitalRead(button);

}
void loop() {
  decideNow= recordRetrieve(); // Controle the whole button pushing structure 

  if(decideNow == 1){ // record information on memory
    numberOfPresses= 0;  
    decideNow= 0;
    randomNumber = getRandom();
    prefs.putUShort("Random", randomNumber);
    Serial.print("Saved ");
    Serial.println(randomNumber);

  }else if(decideNow == 2){ // retrieve information from memory
    numberOfPresses= 0;  
    decideNow= 0;
    randomVar = prefs.getUShort("Random", 0);
    Serial.print("Retrieved ");
    Serial.println(randomVar);
  }else{

  }
}
// This function reads and debounces a push button, waiting for 1 or 2 presses to be read
int recordRetrieve(){  
  singlebit.buttonStatus = digitalRead(button);
  if(singlebit.buttonStatus != singlebit.lastButton){
    if(millis() - debounceTimer > 300){
      debounceTimer = millis();
      if(!singlebit.buttonStatus){
        pressCount++;
        if(pressCount == 1){
          pressTimer = millis();
        }
      }
      singlebit.lastButton = singlebit.buttonStatus;
    }
  } 
  
  if(pressCount > 0 && millis() - pressTimer > 2000){ // Gives two seconds before deciding what to do with button presses
    numberOfPresses = pressCount;
    pressCount = 0;
  }
  if(millis() - ledTimer > ledInterval){ // Just blink an LED
    ledTimer += ledInterval;
    singlebit.ledState= !singlebit.ledState; // blink always start with the LED HIGH, since at startup ledState= false
    // and after turn OFF (below) ledState= false also.
    digitalWrite(LED, singlebit.ledState);
  }
  
  return numberOfPresses; // return number of button presses
}
int getRandom(){
  // Example: number from 0 to 100 (including 100)
  uint32_t r = randomRange(0, 101);
  return r;
}