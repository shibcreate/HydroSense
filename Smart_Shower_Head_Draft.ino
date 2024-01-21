//Inc
#include <stdint.h>
#include <stdlib.h>
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>

//Website Stuff
String URL = "http://IP_ADDRESS/HydroSense/test_data.php";

//User/Pass for Hotspot
const char* ssid = "USER";
const char* password = "PASS";

//Default values if nothing is fed in
String Time = (String)1;
String Liters = (String)1;
String PersonType = (String)1;
String Temperature = (String)1;

String randomNumber = (String)random(1, 36);

String postData = "Time=" + Time + "&Liters=" + Liters + "&PersonType=" + PersonType + "&Temperature=" + Temperature;

//Return code
int httpCode = 0;
String payload = "";

HTTPClient http;

//Button Definitions
#define INTERRUPT_PIN_BUTTON 32 
#define BUTTON_DEBOUNCE 250

//Time after upload before ESP Sleeps, in ms
#define TIME_TO_SLEEP 10000

//Piezo Buzzer Pin
#define BUZZER_PIN 4 //TODO: CHANGE LATER

//Sensor and Shower Definitions
#define INTERRUPT_PIN_SENSOR 35
#define PULSES_PER_LITER 596 
#define LITER_LIMIT 75
#define SHOWER_IDLE_TIME 10000 //Allowed time after shower turned off to be considered "Complete"

//LED Definitions
#define LED_BUILTIN 2
#define LED_SIZE 4 //# of LEDs used
#define LED_PWM 13

//Pin numbers of LEDs, (CHANGE LATER)
#define LED_0 27 //Blue 0, User0 
#define LED_1 26 //Blue 1, User1
#define LED_2 25 //Blue 2, User2
#define LED_3 33 //Blue 3, User3
#define LED_4 12 //Red
#define LED_5 14 //Green
#define RED_LED LED_4
#define GREEN_LED LED_5

//Thresholds in Liters
#define GREEN_THRESHOLD 0.4f //Green LEDs on if under 100 pulses
#define TEALE_THRESHOLD 0.8f //Blue + Green LEDs on if under 100 pulses
#define RED_THRESHOLD 1.2f //Red LEDs on if under 100 pulses

//Array of all LED Pins used
const int led_addr[LED_SIZE + 2] = {LED_0, LED_1, LED_2, LED_3, LED_4, LED_5};

//a funciton that takes in a parameter to currentUserSelectedByButton and sets corresponding LED
void setBlueLED();

//do pinMode out for all the LEDS and pull them to high
void initLeds();

//Function to enable LEDs based on Cumulative Water Usage in a Shower
void setLEDColors();


//User variable, dictates which selection
volatile int currentUserSelectedByButton = -1; //-1 Because User is 0 indexed and interrupt increments automatically

//Global Vars for button status
uint32_t button_start_time;
bool built_in_status = 0;
int last_press = millis();


//Set if shower has started else clear
volatile bool Shower_Started = false;

//holds the milisecond count of the last waterflow sensor trigger
uint32_t lastTimeStamp;

// Enum for state machine
typedef enum {
    SLEEP_STATE = 0,       // Low Power Consumption
    WAKEUP_STATE,     // Recently turned on, Initialization
    USER_INPUT_STATE, //User inputting data using button presses
    ACTIVE_STATE,     // Collecting Data
    UPDATE_DB_STATE,  // Update Database
} possibleStates;

//Holds the current state value, initially set to SLEEP_STATE
volatile possibleStates currentState = SLEEP_STATE;

// Struct to hold water flow sensor information
typedef struct {
    volatile uint32_t   pulseCount;
    uint8_t             interruptPinNum;
    void                (*ISR_waterFlowSensor_t)();
    uint8_t             triggerType;
    uint8_t             maxLiters;
    uint16_t            numSeconds; 
} WaterFlowSensor;

//Pointer to the sensor as global to access in function
WaterFlowSensor* sensor0;

//Interrupt Service Routine for water flow sensor
void ISR_waterFlowSensor0();

// Function to get water usage in Liters
//Todo maybe Pass in reference of a Sensor Struct instead to make it more consistent and scalable?
float getWaterUsage();

// Function to initialize sensor0
//Todo, Same as getWaterUsage()?
void initSensor0(void);

// Function that will return true if Liters of water used surpasses maxliter
//Same as getWaterUsage()
bool waterExceeded();

//Structure to keep track of users
typedef struct{
    uint8_t     idNumber; //Numbered based on LEDs, 1-4
    uint8_t     latestWaterUsage; //Total water used in last shower in Liters
    uint16_t    totalTemp; //Summation all the temperature readings
    uint16_t    totalTempReadings; //total times temperature sensor read data
    uint8_t     avgTemp; //Average temperature for the last shower
    uint16_t    totalTime; //total time of shower in seconds
}User;

//Global variable to hold the current user in shower
User* currentUser = NULL;

//Initialize 4 users because of 4 LEDs
User users[LED_SIZE];

//Initiate user passed in
void initUser(User* this_user);

//Call All update functions, maybe use a function vector table? 
void updateAll(User* this_user);

//declarations of all update functions
void updateWaterUsage(User* this_user);
void updateTotalTemp(User* this_user);
void updateTotalTempReadings(User* this_user);
void updateAvgTemp(User *this_user);
void updateTotalTime(User *this_user);


//Event Handler on button press
//Todo: Find out why this interrupt triggers sensor interrupt as well
void ISR_Button_Press(void); 

void setup(void) {
    initLeds();
    initSensor0();

    //Built in LED, for debugging
    pinMode(2, OUTPUT);
    pinMode(INTERRUPT_PIN_BUTTON, INPUT_PULLDOWN);
    pinMode(INTERRUPT_PIN_SENSOR, INPUT);
    pinMode(BUZZER_PIN, OUTPUT);

    //Mute buzzer (Added)
    digitalWrite(BUZZER_PIN, HIGH);
    
    // Initialize Serial to view data
    Serial.begin(115200);

    

    //Wifi Setup
    connectWiFi();
    
    //Initialize all users
    for(int i = 0; i < LED_SIZE; i++){
        initUser(&users[i]); //Sets all Users Attributes to 0
        users[i].idNumber = i; //associate user number with LED
    }
    //Button Interrupt
    attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN_BUTTON), ISR_Button_Press, RISING); //On button press, run button ISR
}

void loop(void) {

    if (WiFi.status() != WL_CONNECTED) {
       connectWiFi();
     }

    /*Start of FSM*/
    switch (currentState) {
        //Todo: Implement low power function, interrupted by button press
        case SLEEP_STATE:
            /**
             * @Occures When no activity for 5 minutes
             * @Action Go to low power mode
            */
            digitalWrite(BUZZER_PIN, HIGH);
            Serial.println("Sleeping:"); 
            break;

        case WAKEUP_STATE:
            /** 
             * @Occurs When button clicked
             *  @Action LED animation.  go USER_INPUT_STATE. 
             * 
            */
            Serial.println("Wakeup Sequence:");
            OPENING_SEQUENCE();
            /*
                More Wakeup Code here
            */
            currentState = USER_INPUT_STATE; //go to USER_INPUT_STATE
            break;           
        case USER_INPUT_STATE:
            /**
             * @Occurs When button press detected after SLEEP
             * @Action Allows user to select user. Sets LED
             * 
            */
            
            //Set the current user to the current button selection
            currentUser = &users[currentUserSelectedByButton];
            setBlueLED();
            Serial.print("User Selected: ");
            Serial.println(currentUser->idNumber);
  
             //If 5 Seconds elapsed, select current user
             if ((millis() - last_press) >= 5000) {
                   currentState = ACTIVE_STATE;
                   //Begin shower's initalization
                   lastTimeStamp = millis();
                   detachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN_BUTTON));
             }

           break;
        case ACTIVE_STATE:
            /**
             * @Occurs When water flow detected or user is selected in USER_INPUT_STATE
             * @Action Measures amount of water used and time  
            */
            //Button presses should do nothing at this point so detach the interrupt 
            attachInterrupt(digitalPinToInterrupt(sensor0->interruptPinNum), sensor0->ISR_waterFlowSensor_t, sensor0->triggerType); //On Sensor Pulse, Run Sensor ISR
            detachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN_BUTTON));

            setLEDColors();
            
            Serial.print("Shower Pulse Count: ");
            Serial.println(sensor0->pulseCount);
            Serial.print("Shower Liter Usage: ");
            Serial.println(getWaterUsage());
            
            //If 5 mins have passed since lastTimeStamp
            if ((millis() - lastTimeStamp) >= SHOWER_IDLE_TIME) {
                currentState = UPDATE_DB_STATE; // go to next state
            }
            break;
        case UPDATE_DB_STATE:
            Serial.println("Updating Database!");
            /**
             * @Occurs 5 minutes after no water flow activity in ACTIVE_STATE
             * @Action Post data to database. Go to sleep mode.
            */
            updateAll(currentUser);

            Time = (String)(currentUser->avgTemp/1000);
            Liters = (String)getWaterUsage();
            PersonType = (String)(currentUser->idNumber+1);
            Temperature = randomNumber;

            postData = "Time=" + Time + "&Liters=" + Liters + "&PersonType=" + PersonType + "&Temperature=" + Temperature;
            http.begin(URL);

            // Set the Content-Type header before making the request
            http.addHeader("Content-Type", "application/x-www-form-urlencoded");

            httpCode = http.POST(postData);

            payload = http.getString();

            Serial.print("URL: "); Serial.println(URL);
            Serial.print("Data: "); Serial.println(postData);
            Serial.print("httpCode: "); Serial.println(httpCode);
            Serial.print("payload: "); Serial.println(payload);
            Serial.println("------------------------------------------");
            
            Serial.print("User: ");
            Serial.println(currentUser->idNumber);
            Serial.print("Water Usage: ");
            Serial.println(getWaterUsage());
            Serial.print("Temperature: ");
            Serial.println(currentUser->avgTemp);
            Serial.print("Time in Shower: ");
            Serial.println(currentUser->totalTime);
            Serial.println("Going to sleep.");
            delay(TIME_TO_SLEEP); //Display Information for 10 Seconds
            
            for(int i = 0; i < LED_SIZE + 2; i++){
              digitalWrite(led_addr[i], HIGH);
            }
            currentState = SLEEP_STATE;

            //Reactivate Button ISR
            detachInterrupt(digitalPinToInterrupt(sensor0->interruptPinNum));
            attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN_BUTTON), ISR_Button_Press, RISING); //On button press, run button ISR
            break;
            
        default:
          Serial.print("Current State: ");
          Serial.println(currentState);
          return;
    }
 
}

//Blink LED's 1 by 1 to indicate tunring on
void OPENING_SEQUENCE(){
    
    //Turn on LEDs in sequence
    for (int i = 0; i < LED_SIZE; i++){
        
        digitalWrite(led_addr[i], LOW);   
        delay(250);
        digitalWrite(led_addr[i], HIGH);
    }

    //Turn on all colors and LEDs at once, then turn off after 1 second
    digitalWrite(RED_LED, LOW);
    digitalWrite(GREEN_LED, LOW);
    for(int i = 0; i < LED_SIZE; i++){
        digitalWrite(led_addr[i], LOW);
    }

    delay(750);

    for(int i = 0; i < LED_SIZE + 2; i++){
        digitalWrite(led_addr[i], HIGH);
    }
}

//Stores initial shower time in milliseconds
uint32_t Shower_Start_Time;

//Stores totalTime in shower
volatile uint32_t Shower_length;

// Interrupt Service Routine for water flow sensor, also tracks temperature
void ISR_waterFlowSensor0(void) {
    //digitalWrite(GREEN_LED, LOW);
    sensor0->pulseCount++; //increment pulse
    //If Shower Hasnt been started set Shower_Started to true and record initial time
    if(Shower_Started == false){
        
        currentState = ACTIVE_STATE; //set state to activeState immediately if any water flow detected regardless of userinput
        Shower_Started = true;
        Shower_Start_Time = millis();
    }
    
    lastTimeStamp = millis(); //make stamp of milisecond count wjem the fucntion is run
    Shower_length = lastTimeStamp - Shower_Start_Time; //set Shower_length to current time - start time
}



//Handler for when button is pressed
void ISR_Button_Press(void){
    //Check for debouncing
    if(millis() - last_press  < BUTTON_DEBOUNCE){
      return;
    }
    //If on SLEEP_STATE go to USER_STATE and thats it
    if (currentState == SLEEP_STATE) {
        currentState = WAKEUP_STATE;
    }

    //Flip built in LED
    built_in_status = !built_in_status;
    digitalWrite(2, built_in_status);
    
    //Sets the current button press, and loops back to 0
    currentUserSelectedByButton = (currentUserSelectedByButton+1) % LED_SIZE;

    //For debouncing
    last_press = millis();
}


// Function to get water usage in Liters
float getWaterUsage() {
    return static_cast<float>(sensor0->pulseCount) / PULSES_PER_LITER;
}

// Function to initialize sensor0
void initSensor0() {
    // Allocate memory for sensor0
    sensor0 = (WaterFlowSensor*)malloc(sizeof(WaterFlowSensor));

    // Assign pulseCount to 0
    sensor0->pulseCount = 0;

    // Set pin number
    sensor0->interruptPinNum = INTERRUPT_PIN_SENSOR;

    // Assign function pointer to corresponding ISR
    sensor0->ISR_waterFlowSensor_t = ISR_waterFlowSensor0;

    // Assign sensor to trigger at falling edge
    sensor0->triggerType = FALLING;

    sensor0->maxLiters = LITER_LIMIT;
}

bool waterExceeded() {
    return (getWaterUsage() >= sensor0->maxLiters);
}

//Function for activating Blue LEDs during User Selection
void setBlueLED() {

    //Turn off all blue LEDs
    for(int i = 0; i < LED_SIZE; i++){
      digitalWrite(led_addr[i], HIGH);
    }
    digitalWrite(RED_LED, HIGH);
    digitalWrite(GREEN_LED, HIGH);

    //Switch on current player's led
    switch(currentUserSelectedByButton) {
        case 0:
            digitalWrite(LED_0, LOW);
            break;
        case 1:
            digitalWrite(LED_1, LOW);
            break;
        case 2:
            digitalWrite(LED_2, LOW);
            break;
        case 3:
            digitalWrite(LED_3, LOW);
            break;
        default:
            return;
    }
}

//Run once, set all pins to high
void initLeds() {
    for (int i = 0; i < LED_SIZE + 2; i++) {
        pinMode(led_addr[i], OUTPUT);
        digitalWrite(led_addr[i], HIGH);
    } 
    pinMode(LED_PWM, OUTPUT);
    digitalWrite(LED_PWM, HIGH); //Make parallel Cathodes High (Multicolor LEDs share one anode)
}

//Initialize all values to 0
void initUser(User* this_user){
    this_user->idNumber = 0;
    this_user->latestWaterUsage = 0;
    this_user->avgTemp = 0;
    this_user->totalTime = 0;
}

//Maybe Use a function vector table? not sure which one is more readable
void updateAll(User* this_user){
    updateWaterUsage(this_user);
    updateTotalTemp(this_user);
    updateTotalTempReadings(this_user);
    updateAvgTemp(this_user);
    updateTotalTime(this_user);
}

//Update usage of water for this_user
void updateWaterUsage(User* this_user){
    this_user->latestWaterUsage = getWaterUsage();
}

//Update total temperature for this_user, used to calc avg temp to be initialized
void updateTotalTemp(User* this_user){
    Serial.println("Update Total Temperatures function called");
}

//Update total temperature readings for this_user, used to calc avg temp to be initialized
void updateTotalTempReadings(User* this_user){
    Serial.println("Update Total Temperature Readings function called");
}

//Update average temperature for this_user, Total Temp / Total Temp Readings
void updateAvgTemp(User *this_user){
    Serial.println("Update Average Temperature Readings function called");
}


//Update total time for this_user's shower
void updateTotalTime(User *this_user){
    this_user->totalTime = Shower_length;
}

//Function for determining what LED arrangement based on Water Usage
void setLEDColors(){
  
  if (getWaterUsage() < GREEN_THRESHOLD) {
    digitalWrite(GREEN_LED, LOW); //ALL GREEN LEDS ON
  } 
  else if (getWaterUsage() < TEALE_THRESHOLD) {
     //blue + green
     digitalWrite(GREEN_LED, HIGH); //ALL GREEN LEDS ON
     digitalWrite(LED_0, LOW); //ALL  LEDS ON
     digitalWrite(LED_1, LOW); //ALL  LEDS ON
     digitalWrite(LED_2, LOW); //ALL  LEDS ON
     digitalWrite(LED_3, LOW); //ALL  LEDS ON
  } 
  else if (getWaterUsage() < RED_THRESHOLD) {
     digitalWrite(GREEN_LED, HIGH); //ALL GREEN LEDS OFF
     digitalWrite(LED_0, HIGH); //ALL Blue LEDs off
     digitalWrite(LED_1, HIGH);
     digitalWrite(LED_2, HIGH); 
     digitalWrite(LED_3, HIGH); 

     //Turn on Red
     digitalWrite(RED_LED, LOW);
  } 
  else {
    //Blink Red LEDs
    digitalWrite(RED_LED, LOW);
    digitalWrite(BUZZER_PIN, LOW);
    delay(500);
    digitalWrite(RED_LED, HIGH);
    digitalWrite(BUZZER_PIN,HIGH);
    delay(500);
  }
}

void connectWiFi() {
  WiFi.mode(WIFI_OFF);
  delay(1000);
  WiFi.mode(WIFI_STA);

  WiFi.begin(ssid, password);
  Serial.println("Connecting to Wifi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.print("Connected to: "); Serial.println(ssid);
  Serial.print("IP Address: "); Serial.println(WiFi.localIP());
}
