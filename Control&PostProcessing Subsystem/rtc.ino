#include "STM32LowPower.h"
#include "STM32RTC.h"

#define LED_PIN PC13
#define SLEEP_SECONDS 30

STM32RTC& rtc = STM32RTC::getInstance();
volatile bool alarmFired = false;

void alarmCallback(void* data) {
    alarmFired = true;
}

void setNextAlarm() {
    uint8_t s = rtc.getSeconds();
    uint8_t m = rtc.getMinutes();

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

void goToSleep() {
    alarmFired = false;        // clear flag BEFORE sleeping
    digitalWrite(LED_PIN, HIGH);
    Serial.flush();
    Serial.println("hi");
    LowPower.sleep();
    Serial.println("by");
    digitalWrite(LED_PIN, LOW);
}

void setup() {
    Serial.begin(115200);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    rtc.begin();
    LowPower.begin();
    rtc.attachInterrupt(alarmCallback);

    setNextAlarm();
    Serial.println("Going to sleep...");
    goToSleep();
}

void loop() {
    if (!alarmFired) return;   // ignore random wakes

    Serial.println("Woke up!");
    Serial.flush();

    setNextAlarm();
    goToSleep();
}
