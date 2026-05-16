/*
 *Final firmware code
 * - DS18B20 Temperature Sensor
 * - SD Card logging
 * - INA226 Voltage/Current Monitor
 *
 * Wakes up every 12 hours to take sample (10s ADC burst). Alert triggerd if bat below 8.4V and shutdown.
 */

#include <OneWire.h>
#include <DallasTemperature.h>
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <INA226_WE.h>
#include "STM32LowPower.h"
#include "STM32RTC.h"

// --- Pin Definitions ---
#define SENSOR_PIN         PA10   // DS18B20 data pin
#define SD_CS_PIN          PA4    // SD card chip select
#define LED_PIN            PC13   // onboard LED (active LOW on Black Pill)
#define LED_SENSE_PIN      PA8    // signal to enable sensing LED
#define SHUTDOWN_PIN       PA11   // shutdown signal
#define POWER_RAIL_PIN     PA15   // controls external power rails (low power mode pin)
#define ADC_PIN            PA0    // photodiode input
#define INA226_ADDR        0x40
#define SLEEP_HOURS        12
#define BURST_SIZE         100
#define SAMPLE_DELAY_MS    100    // 100ms × 100 = 10 second burst
#define MAX_TEMP           40.0f  // shutdown above this temperature
#define INA_ALERT_PIN      PB0  // connect INA226 ALERT pin here + 10k pullup to 3V3
#define SLEEP_SECONDS 10

OneWire oneWire(SENSOR_PIN);
DallasTemperature tempSensor(&oneWire);
INA226_WE ina226(INA226_ADDR);
STM32RTC& rtc = STM32RTC::getInstance();

bool sdReady  = false;
bool wiperFlag=false;
bool shutterFlag=false;
volatile bool alarmFired = false;
volatile bool inaAlert = false;

float readings[100]; //array to store intensity values from ADC
int count = 0; //final value of intensity voltages stored
float threshold = 0.2; //used to check intensity outliers
float totalIntensity=0.0f;
float avgIntensity = 0.0f;
float baselineIntensity =0.0f;

float voltage =0.0f;
float current=0.0f;

void goToSleep();  // forward declaration

void alarmCallback(void* data) {
    alarmFired = true; //alarm occured
}

void inaAlertCallback() {
    inaAlert = true;
}
void logMsg(const char* msg) {
    if (!sdReady) return;
    File f = SD.open("LOG.TXT", FILE_WRITE);
    if (f) {
        char timestamp[20];
        snprintf(timestamp, sizeof(timestamp), "20%02d-%02d-%02dT%02d:%02d:%02d",
            rtc.getYear(), rtc.getMonth(), rtc.getDay(),
            rtc.getHours(), rtc.getMinutes(), rtc.getSeconds());
        f.print(timestamp); f.print(" — "); f.println(msg);
        f.close();
    }
}

void setNextAlarm() { //function to set alarm every 12 hours
    uint8_t h = rtc.getHours();
    uint8_t m = rtc.getMinutes();
    uint8_t s = rtc.getSeconds();

    //h += SLEEP_HOURS; //increase current hour + 12
    //if (h >= 24) h -= 24; //wrap around

    //set alarm
    //rtc.setAlarmHours(h);
    //rtc.setAlarmMinutes(m);
    //rtc.setAlarmSeconds(s);
   // rtc.enableAlarm(rtc.MATCH_HHMMSS);  // match exact time

    //demo version
    s += SLEEP_SECONDS;
    if (s >= 60) {
        s -= 60;
        m += 1;
        if (m >= 60) m = 0;
    }

    rtc.setAlarmMinutes(m);
    rtc.setAlarmSeconds(s);
    rtc.enableAlarm(rtc.MATCH_MMSS);
}

void initLowPower(){ //function to intialise rtc and low power mode 
    rtc.begin();
    LowPower.begin();
    rtc.attachInterrupt(alarmCallback); //create interrupt to alarm
    LowPower.attachInterruptWakeup(INA_ALERT_PIN, inaAlertCallback, FALLING,DEEP_SLEEP_MODE);  // register alert as wakeup
  //  Serial.println("Going to sleep...");
    goToSleep(); // set STM into low power mode
}

void initSD() { //initialise SPI for SD card
    SPI.setMOSI(PA7);
    SPI.setMISO(PA6);
    SPI.setSCLK(PA5);

    if (!SD.begin(SD_CS_PIN)) { //check sd card connected properly
       // Serial.println("[SD] Init failed. Check wiring.");
        return;
    }

  //  Serial.println("[SD] Ready."); //all good-continue
    sdReady = true;

    if (SD.exists("DATA.TXT")) {
           logMsg("[SD] DATA.TXT found — appending.");
        } else {
            Serial.println("[SD] DATA.TXT not found — creating.");
            File f = SD.open("DATA.TXT", FILE_WRITE);
            if (f) {
                f.println("timestamp,baseline,sample,temperature_C,voltage_V,current_mA");
                f.close();
                logMsg("[SD] Header written.");
            }
    }
        // create log file header if new
    if (!SD.exists("LOG.TXT")) {
        File f = SD.open("LOG.TXT", FILE_WRITE);
        if (f) { f.println("timestamp — message"); f.close(); }
    }
}
void writeSD(float Intensity, float baseline, float temp, float vol,float cur){ //function to write data to sd card
    if (sdReady) {
        char timestamp[20];
        snprintf(timestamp, sizeof(timestamp),"20%02d-%02d-%02dT%02d:%02d:%02d",
            rtc.getYear(), rtc.getMonth(), rtc.getDay(), rtc.getHours(), rtc.getMinutes(), rtc.getSeconds());

        File f = SD.open("DATA.TXT", FILE_WRITE);
        if (f) {
            f.print(timestamp);  f.print(",");
            f.print(baseline, 4); f.print(",");
            f.print(Intensity, 4);   f.print(",");
            f.print(temp, 2);    f.print(",");
            f.print(vol, 3);  f.print(",");
            f.println(cur, 3);
            f.close();
             // print the same line to serial
            logMsg("[SD] Record written.");
        } else {
            logMsg("[SD] Failed to open file.");
            // digitalWrite(LED_PIN, HIGH); delay(50); digitalWrite(LED_PIN, LOW);
        }
    }
}
void initTemp(){ //intialise temperature sensor
    tempSensor.begin();
    logMsg("[Temp] DS18B20 ready.");
}

void initINA(){ //initialse I2C for INA226
    Wire.begin();
    if (!ina226.init()) { //check connection
        logMsg("[INA226] Not found. Check wiring.");
        while (1);
    }
    ina226.setResistorRange(0.1, 1.0);

    ina226.setAlertType(INA226_BUS_UNDER, 8.4);  // type + limit in one call
    ina226.enableAlertLatch();                    // hold alert pin LOW until manually cleared

    pinMode(INA_ALERT_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(INA_ALERT_PIN), inaAlertCallback, FALLING);


    logMsg("[INA226] Ready.");
}
float ADCRead(){
    digitalWrite(POWER_RAIL_PIN,HIGH); //whole system needs power
    digitalWrite(LED_SENSE_PIN,HIGH); //led pin on
    delay(10000); //wait 10s for led to stabilise

    count =0;
    totalIntensity=0;
        for (int i = 0; i < 100; i++) {
        float intensity = analogRead(PA0) * (3.3 / 4095.0);

        if (count == 0 || abs(intensity - readings[count - 1]) < threshold) {
            readings[count] = intensity;
            count++;
        } else {
           logMsg("Outlier discarded");
        }

        delay(100); //100ms*100 samples = 10 seconds
    }

    for (int i =0;i<count;i++){
        totalIntensity += readings[i];
    }

    float average = totalIntensity/count;
    digitalWrite(LED_SENSE_PIN,LOW); //turn led off
    return average;
    
}

void batteryRead(){
    ina226.readAndClearFlags();
    voltage = ina226.getBusVoltage_V();
    current = ina226.getCurrent_mA();

   // Serial.print("[INA226] Sample ");
    //Serial.print(": "); Serial.print(voltage, 3);
    //Serial.print(" V  |  "); Serial.print(current, 3); Serial.println(" mA");

    //    if (voltage < 3.5) Serial.println("[INA226] WARNING: Low battery!");
}

void shutdown() {
    shutterFlag = true;
    logMsg("Shutting down permanently.");
    ina226.readAndClearFlags();
    digitalWrite(SHUTDOWN_PIN, HIGH);
    digitalWrite(LED_PIN, HIGH);
    digitalWrite(POWER_RAIL_PIN, LOW);
    detachInterrupt(digitalPinToInterrupt(INA_ALERT_PIN));
    rtc.disableAlarm();
    inaAlert = false;
    alarmFired = false;
    while (true) {
        LowPower.deepSleep();
    }
}


void checkINA() {
    if (!inaAlert) return; //continue only if alert triggered
    digitalWrite(LED_PIN, LOW);  //turn on if alert triggered

    ina226.readAndClearFlags();  // clear or pin stays LOW
    float v = ina226.getBusVoltage_V();
    float i = ina226.getCurrent_mA();

   // Serial.print("[ALERT] V="); Serial.print(v);
    //Serial.print(" I="); Serial.println(i);

    if (v < 8.4) {
       // Serial.println("[ALERT] Low battery — shutting down.");
        writeSD(-1,-1,-1,v,-1);
        digitalWrite(LED_PIN, HIGH);  //turn off before shutdown
        shutdown();
    }
    if (i > 1000.0) {
       // Serial.println("[ALERT] Overcurrent — shutting down.");
        shutdown();
    }

    inaAlert = false;
}

void goToSleep() {
    wiperFlag = false;
    setNextAlarm();
    alarmFired = false;
    inaAlert = false;                  // ← clear flag
    ina226.readAndClearFlags();        // ← clear latch so pin goes HIGH
    digitalWrite(LED_PIN, HIGH);
    digitalWrite(POWER_RAIL_PIN, LOW);
    logMsg("Going to sleep...");
    LowPower.deepSleep();
}

void setup() {
    //delay(3000); 
    //Serial.begin(115200);
 //   Serial.begin(115200);
   // unsigned long t = millis();
  //  while (!Serial && millis() - t < 5000);  // wait up to 5s for monitor to open
    
   // Serial.println("Serial ready.");
  //  while (!Serial); 
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);  // starts off (sleeping during baseline)
    pinMode(ADC_PIN, INPUT_ANALOG);
    analogReadResolution(12);        // set 12-bit resolution for ADC

    pinMode(LED_SENSE_PIN, OUTPUT); //LED sensing pin
    pinMode(SHUTDOWN_PIN, OUTPUT); //shutdown pin
    pinMode(POWER_RAIL_PIN, OUTPUT); //low power mode pin

    digitalWrite(SHUTDOWN_PIN, LOW); 

    rtc.begin();              // need to call begin before setting time
    rtc.setTime(14, 20, 0);  // HH, MM, SS
    rtc.setDate(15, 5, 26);  // DD, MM, YY


    initSD();   // SD must be first so logMsg works
    initINA();
    initTemp();


    logMsg("All inits done.");
    logMsg("--- Starting ---");

   // pinMode(LED_PIN, OUTPUT);
    //digitalWrite(LED_PIN, LOW); //led on in normal power mode
  //  Serial.println("\n--- Starting ---\n");

    //----baseline reading upon deployment--------
    digitalWrite(LED_PIN, LOW); 
    logMsg("[ADC] Taking deployment baseline...");
    baselineIntensity=ADCRead();
    logMsg("[ADC] Baseline taken.");

    digitalWrite(LED_PIN, HIGH);  // LED off before sleep
    initLowPower(); //go to sleep

}

void loop() {
    checkINA(); //check levels-> for case if alert triggered

    if (!alarmFired) return; //alarm not fired -do nothing
    //continue if alarm fired->in normal power mode
    digitalWrite(LED_PIN, LOW);  
    logMsg("Woke up!");
   // Serial.flush();

    //--------------SAMPLING WINDOW--------------------------------
   // delay(200);
    tempSensor.requestTemperatures();
    float tempC = tempSensor.getTempCByIndex(0);
    //Serial.print("[Temp] Sample "); 
   // Serial.print(": "); Serial.print(tempC); Serial.println(" C");
    if (tempC>MAX_TEMP){
        //shutdown
        shutdown();
    } else { //safe operation continue sampling

        //-----ADC operation------
        avgIntensity = ADCRead();
        batteryRead();
        //-----check levels before shutting down again----
        if (voltage < 8.4){
            logMsg("Low battery — shutting down.");
            shutdown();
       }
        if (current > 1000.0) {
            logMsg("Overcurrent — shutting down.");
            shutdown();
        }
        //---file write------
        writeSD(avgIntensity,baselineIntensity,tempC,voltage,current);
        //----clean glass to get baseline----
        wiperFlag =true;
        baselineIntensity=ADCRead(); //get baseline to be used for next sample
        //---back to low power mode-----
        logMsg("Baseline updated. Going to sleep.");
        goToSleep();
    }
    //--------------END Sampling Window-----------------------------
    
}
