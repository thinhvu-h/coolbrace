

/*** PWM table ***/
/* 0/255    0% */
/* 64/255   25% */
/* 127/255  50% */
/* 191/255  75% ~ 23C */
/* 255/255  100% */

/*
 * cooler 1  -------       
 *          |       |       
 *          |       |       |        |        |        |
 *          -       ------------------------------------
 * cooler 2          -------      
 *                  |       |
 *                  |       |        |        |        |
 *          ---------       ----------------------------
 * cooler 3                  --------      
 *                          |        |
 *                          |        |        |        |
 *          -----------------        -------------------          
 *          
 * 
 */

#define PWM_00_PERCENT    0   // OFF
#define PWM_10_PERCENT    26
#define PWM_25_PERCENT    64  // mode 1
#define PWM_35_PERCENT    90
#define PWM_50_PERCENT    127 // mode 2
#define PWM_55_PERCENT    140
#define PWM_60_PERCENT    152
#define PWM_75_PERCENT    191 // mode 3
#define PWM_90_PERCENT    229
#define PWM_100_PERCENT   255 // mode 4

#define PWM_ON       0xFF
#define PWM_OFF      0x00 

#define COOLER_MODE       0
#define HEATER_MODE       1
#define COOLER_MODE_SUPPORTED    4
#define HEATER_MODE_SUPPORTED    2

#define COOLER_PIN        9   // sò
#define HEATER_PIN        11
#define FAN_PIN           10  // Fan
#define LED_RED_PIN       A1  // Active LOW
#define LED_BLUE_PIN      A2  // Active LOW
#define BUTTON_PW_LVL_PIN        3   // pull-up Active LOW
#define BUTTON_MODE_PIN          7   // pull-up Active LOW

// LED attribute
int ledState = HIGH;                  // ledState used to set the LED
unsigned long currentMillis = 0;
unsigned long previousMillis = 0;     // will store last time LED was updated
const long interval = 5000;            // interval at which to blink (milliseconds)
const long button_interval = 4000;            // interval at which to blink (milliseconds)

// Button attribute
int button_power_level_state = HIGH;
int prev_button_power_level_state = HIGH;
int button_mode_state = HIGH;
int prev_button_mode_state = HIGH;

static bool buttonModeInProgress = false;
static bool buttonPowerInProgress = false;
static bool buttonPowerLongPress = false;
static bool buttonModeLongPress = false;

// MODE attribute
unsigned int power_mode_supported = COOLER_MODE_SUPPORTED;
unsigned int power_level = 0; // off
unsigned int prev_cooler_power_level = 0; // off
unsigned int prev_heater_power_level = 0; // off
unsigned int controlState = PWM_OFF;
bool device_mode = COOLER_MODE;

void setup() {
  
  Serial.begin(115200);
  Serial.println("\nsystem init...");
  
  pinMode(COOLER_PIN, OUTPUT);
  pinMode(HEATER_PIN, OUTPUT);
  pinMode(FAN_PIN, OUTPUT);
  pinMode(BUTTON_PW_LVL_PIN, INPUT);
  pinMode(BUTTON_MODE_PIN, INPUT);

  pinMode(LED_RED_PIN, OUTPUT);
  digitalWrite(LED_RED_PIN, HIGH); 
  pinMode(LED_BLUE_PIN, OUTPUT);
  digitalWrite(LED_BLUE_PIN, HIGH); 
  
  Serial.print("\n Device Mode: ");Serial.print(device_mode); 
  Serial.print("\n Power Mode: ");Serial.print(power_level); Serial.print(" click button to start...\n");
}

enum blinkState {
  BLINK_TURN_ON,
  BLINK_CHANGE_MODE,
  BLINK_CHANGE_POWER_LEVEL,
  BLINK_TURN_OFF
};
void power_level_indicate() {

  static unsigned int blink_counter;
  blinkState blink_state;
  unsigned int prev_power_level;

  if(blink_counter != 0) return;
  
  if(device_mode == COOLER_MODE) {
    prev_power_level = prev_cooler_power_level;
  } else {
    prev_power_level = prev_heater_power_level;
  }
  
  if(prev_power_level == 0 && power_level > 0 ) { // ON
    blink_counter = 5;
    blink_state = BLINK_TURN_ON;
  } else if(power_level > 0 && buttonModeInProgress == true) { // CHANGE MODE
    blink_counter = 5;
    blink_state = BLINK_CHANGE_MODE;
  } else if (prev_power_level != 0 && power_level > 0 && buttonPowerInProgress == true) { // CHANGE POWER LEVEL
    blink_counter = power_level;
    blink_state = BLINK_CHANGE_POWER_LEVEL;
  } else if (prev_power_level != 0 && power_level == 0) { // OFF
    blink_counter = 5;
    blink_state = BLINK_TURN_OFF;
  } else {
    // NOTHING  
  }
  
  Serial.print("blink_counter: "); Serial.println(blink_counter);
  ledState = HIGH;
  do {
    // if the LED is off turn it on and vice-versa:
    if (ledState == LOW) {
      ledState = HIGH;
      blink_counter--;
    } else {
      ledState = LOW;
    }

    if(blink_state ==  BLINK_TURN_ON || blink_state ==  BLINK_CHANGE_MODE) { // ON or CHANGE MODE
        digitalWrite(LED_BLUE_PIN, ledState);
        digitalWrite(LED_RED_PIN, ledState);
        delay(100);
        // standby little bit indicate mode cooler/heater ON
        if(blink_counter == 0) {
          // turn off
          digitalWrite(LED_BLUE_PIN, HIGH);
          digitalWrite(LED_RED_PIN, HIGH);
          delay(100);
          if(device_mode == COOLER_MODE) {
            digitalWrite(LED_BLUE_PIN, LOW);
          } else {
            digitalWrite(LED_RED_PIN, LOW);
          }
        }  
    }else if(blink_state ==  BLINK_CHANGE_POWER_LEVEL) { // CHANGE POWER LEVEL
      if(device_mode == COOLER_MODE) {
        digitalWrite(LED_BLUE_PIN, ledState);
      } else {
        digitalWrite(LED_RED_PIN, ledState);
      }
      delay(150);
    }else if (blink_state ==  BLINK_TURN_OFF) { // OFF
      digitalWrite(LED_RED_PIN, ledState);
      delay(100);
    } else { 
      // NOTHING
    }
  } while(blink_counter);
  
  delay(800);
  digitalWrite(LED_BLUE_PIN, HIGH);
  digitalWrite(LED_RED_PIN, HIGH);
  
  if(device_mode == COOLER_MODE) {
    prev_cooler_power_level = power_level;
  } else {
    prev_heater_power_level = power_level;
  }
  
}

void button_mode_long_press(long timediff) {
  if(power_level > 0) {
    if(buttonModeInProgress == true) {
      if(timediff > button_interval) {
        buttonModeLongPress = true;
        if(device_mode == HEATER_MODE) {
          device_mode = COOLER_MODE;
          power_mode_supported = COOLER_MODE_SUPPORTED; // update mode support
        } else {
          device_mode = HEATER_MODE;   
          power_mode_supported = HEATER_MODE_SUPPORTED; // update mode support
        }
        power_level = 1;
        Serial.print("power level: "); Serial.print(power_level); Serial.println(" Start!");
        // blinking LED
        power_level_indicate();
      }
    }
  }
}

void button_power_level_long_press(long timediff) {
  if(buttonPowerInProgress == true) {  // Press & Hold
    if(power_level > 0) { // COOLER ON
      if(timediff > button_interval) {
        Serial.print("timediff: "); Serial.print(timediff); Serial.print(" interval: "); Serial.println(button_interval);
        buttonPowerLongPress = true;
        power_level = 0;
        // blinking LED
        power_level_indicate();
      }
    } else { // COOLER OFF
      if(timediff >= button_interval) {
        buttonPowerLongPress = true;
        power_level = 1;
        // blinking LED
        power_level_indicate();
        Serial.print("buttonPowerLongPress: "); Serial.println(buttonPowerLongPress);
        Serial.print("timediff: "); Serial.print(timediff); Serial.print(" interval: "); Serial.println(button_interval);
      }
    }
  }
}

void button_signal_handler() {
  // read the state of the BUTTON_PIN value:
  button_power_level_state = digitalRead(BUTTON_PW_LVL_PIN);
  button_mode_state = digitalRead(BUTTON_MODE_PIN);
  // check if the pushbutton is pressed.
  if (button_power_level_state == LOW && prev_button_power_level_state == HIGH) {
    buttonPowerInProgress = true;
    delay(300);
    // check if the pushbutton is actually pressed.
    if (button_power_level_state == LOW) {
      prev_button_power_level_state = button_power_level_state;
      Serial.println("");Serial.println("Button power level Press!");
    } else {
      buttonPowerInProgress = HIGH;
    }
  } else if (button_mode_state == LOW && prev_button_mode_state == HIGH) {
    buttonModeInProgress = true;
    delay(300);
    // check if the pushbutton is actually pressed.
    if (button_mode_state == LOW) {
      prev_button_mode_state = button_mode_state;
      Serial.println("");Serial.println("Button Mode Press!");
    } else {
      prev_button_mode_state = HIGH;
      buttonModeInProgress = false;
    }
  } else if (button_power_level_state == HIGH && prev_button_power_level_state == LOW && buttonPowerInProgress == true) {
    delay(300);
    // check if the pushbutton is actually release.
    if (button_power_level_state == HIGH && prev_button_power_level_state == LOW) {
      Serial.println("Button pwl Release!");
      // change mode
      if(power_level != 0 & buttonPowerLongPress != true) {
        if(power_level < power_mode_supported) {
          power_level++;
        } else {  // roll back mode 1
          power_level = 1;
        }
        // blinking LED
        power_level_indicate();
      }
      prev_button_power_level_state = button_power_level_state;
      buttonPowerLongPress = false;
      buttonPowerInProgress = false;
      Serial.print("power level: "); Serial.print(power_level); Serial.println(" Start!");
    } else {
      buttonPowerInProgress = false;
      prev_button_power_level_state = HIGH;
    }
  } else if (button_mode_state == HIGH && prev_button_mode_state == LOW && buttonModeInProgress == true) {
    delay(300);
    if (button_mode_state == HIGH && prev_button_mode_state == LOW){
      Serial.println("Button mode Release!");
      prev_button_mode_state = button_mode_state;
      buttonModeLongPress = false;
      buttonModeInProgress = false;
      Serial.print("Device Mode: "); Serial.print(device_mode); Serial.println(" Start!");
    } else {
      buttonModeInProgress = false;
      prev_button_mode_state = HIGH;
    }
  }
}

void cooler_handler(unsigned int controlState) {
  unsigned int coolerControl;
  if(power_level != 0) {
    coolerControl = power_level & controlState;
  } else {
    coolerControl = power_mode_supported + 1;
  }
  switch (coolerControl) {
    case 0:
      analogWrite(FAN_PIN, PWM_100_PERCENT);
      analogWrite(COOLER_PIN, PWM_00_PERCENT);
      Serial.println("COOLER OFF 0");
      break;
    
    case 1:
      analogWrite(FAN_PIN, PWM_00_PERCENT);
      analogWrite(COOLER_PIN, PWM_50_PERCENT);
      Serial.println("COOLER ONN 1");
      break;
    
    case 2:
      analogWrite(FAN_PIN, PWM_00_PERCENT);;
      analogWrite(COOLER_PIN, PWM_50_PERCENT);
      Serial.println("COOLER ONN 2");
      break;
    
    case 3:
      analogWrite(FAN_PIN, PWM_00_PERCENT);
      analogWrite(COOLER_PIN, PWM_75_PERCENT);
      Serial.println("COOLER ONN 3");
      break;
    
    case 4:
      analogWrite(FAN_PIN, PWM_00_PERCENT);
      analogWrite(COOLER_PIN, PWM_100_PERCENT);
      Serial.println("COOLER ONN 4");
      break;

    default:
      analogWrite(FAN_PIN, PWM_00_PERCENT);
      analogWrite(COOLER_PIN, PWM_00_PERCENT);
      Serial.println("COOLER OFF mode");
      break;
  }
}

void heater_handler(unsigned int controlState) {
  unsigned int heaterControl;
  if(power_level != 0) {
    heaterControl = power_level & controlState;
  } else {
    heaterControl = power_mode_supported + 1;
  }
  
  switch (heaterControl) {
    case 0:
      analogWrite(FAN_PIN, PWM_100_PERCENT);
      analogWrite(HEATER_PIN, PWM_00_PERCENT);
      Serial.println("HEATER OFF 0");
      break;
    
    case 1:
      analogWrite(FAN_PIN, PWM_00_PERCENT);
      analogWrite(HEATER_PIN, PWM_25_PERCENT);
      Serial.println("HEATER ONN 1");
      break;

    case 2:
      analogWrite(FAN_PIN, PWM_00_PERCENT);
      analogWrite(HEATER_PIN, PWM_50_PERCENT);
      Serial.println("HEATER ONN 2");
      break;
   
    default:
      analogWrite(FAN_PIN, PWM_00_PERCENT);
      analogWrite(HEATER_PIN, PWM_00_PERCENT);
      Serial.println("HEATER OFF mode");
      break;
  }
}

void loop() {
  currentMillis = millis();
  button_signal_handler();
  if (currentMillis - previousMillis >= interval) {
    button_power_level_long_press(currentMillis - previousMillis);
    button_mode_long_press(currentMillis - previousMillis);
    previousMillis = currentMillis;
    
    if (controlState == PWM_ON) {
      controlState = PWM_OFF;
    } else {
      controlState = PWM_ON;
    }

    if(device_mode == COOLER_MODE) {
      cooler_handler(controlState);
    } else {
      heater_handler(controlState);
    }
  }
}
