#include <SPI.h>
#include <SD.h>

#define SD_CS_PIN PA4   // chip select pin — change if wired differently
#define LED_PIN PC13   // onboard LED (Blackpill)

void setup() {
    Serial.begin(115200);
   // while (!Serial);    // wait for serial monitor to open

    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    Serial.println("Initialising SD card...");
    SPI.setMOSI(PA7);
    SPI.setMISO(PA6);
    SPI.setSCLK(PA5);
    if (!SD.begin(SD_CS_PIN)) {
        Serial.println("SD card init failed. Check wiring and CS pin.");
        digitalWrite(LED_PIN, HIGH);
        delay(1000);
        digitalWrite(LED_PIN, LOW);

        while (1);      // halt — no point continuing
    }

    Serial.println("SD card ready.");

    digitalWrite(LED_PIN, HIGH);   // LED ON
    Serial.println("Writing to SD...");

    // --- Write to file ---
    File dataFile = SD.open("DATA.TXT", FILE_WRITE);

    if (dataFile) {
        dataFile.println("timestamp,signal,reference,temperature,battery");
        dataFile.println("2026-09-08T08:00:00,0.82,1.00,12.4,3.7");
        dataFile.println("2026-09-08T08:00:01,0.81,1.00,12.4,3.7");
        dataFile.close();   // always close — flushes to card
        Serial.println("Data written successfully.");
    } else {
        Serial.println("Failed to open file for writing.");
    }

    delay(200);
    digitalWrite(LED_PIN, LOW);

    
    digitalWrite(LED_PIN, HIGH);
    // --- Read back to verify ---
    dataFile = SD.open("DATA.TXT");

    if (dataFile) {
        Serial.println("Reading back from DATA.TXT:");
        while (dataFile.available()) {
            Serial.write(dataFile.read());
        }
        dataFile.close();
    } else {
        Serial.println("Failed to open file for reading.");
    }
    delay(200);
    digitalWrite(LED_PIN, LOW);
}

void loop() {
    // nothing here — just demonstrating setup
}