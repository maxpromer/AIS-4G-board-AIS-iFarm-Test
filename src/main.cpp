#include <Arduino.h>
#include <ArtronShop_Magellan.h>
#include <SHT40.h>
#include <WiFi.h>
#include <SIM76xx.h>
#include <GSMClient.h>
#include <Wire.h>
#include "Configs.h"
#ifdef IFAMR_4G_BOARD
#include <PCA9557.h>
#endif

#if defined(USE_WIFI)
WiFiClient espClient;
ArtronShop_Magellan magellan(&espClient);
#elif defined(USE_4G)
GSMClient gsmClient;
ArtronShop_Magellan magellan(&gsmClient);
#endif

#ifdef IFAMR_4G_BOARD
PCA9557 io(0x19, &Wire);

// Opto input pin
// #define D1_PIN (0)
// #define D2_PIN (1)
#define D1_PIN (1) // !!! WARNING: D2 on schematic = D1 on board label
#define D2_PIN (0) // !!! WARNING: D1 on schematic = D2 on board label

// Relay output pin
#define C1_PIN (4)
#define C2_PIN (5)
#define C3_PIN (6)
#define C4_PIN (7)
#endif

void setup() {
    pinMode(BUILTIN_LED, OUTPUT);  // Initialize the BUILTIN_LED pin as an output
    Serial.begin(115200);
    Wire.begin();
    Wire.setClock(100E3);

#ifdef IFAMR_4G_BOARD
    // Configs D1, D2 to INPUT mode
    io.pinMode(D1_PIN, INPUT);
    io.pinMode(D2_PIN, INPUT);

    // Configs C1 - C4 to OUTPUT mode
    io.pinMode(C1_PIN, OUTPUT);
    io.pinMode(C2_PIN, OUTPUT);
    io.pinMode(C3_PIN, OUTPUT);
    io.pinMode(C4_PIN, OUTPUT);
#else
    SHT40.begin();
#endif

#if defined(USE_WIFI)
    delay(10);
    // We start by connecting to a WiFi network
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(CONFIG_WIFI_SSID);

    WiFi.mode(WIFI_STA);
    WiFi.begin(CONFIG_WIFI_SSID, CONFIG_WIFI_PASS);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

#elif defined(USE_4G)
    Serial.println("Turn on SIMCOM ...");
    while (!GSM.begin()) {  // Setup GSM (Power on and wait GSM ready)
        Serial.println("GSM setup fail");
        delay(2000);
    }
    Serial.println("SIMCOM ready !");

    Serial.println("Ping test ...");
    while (!Network.pingIP("www.google.com")) {
        Serial.println("Ping fail !");
        delay(1000);
    }
    Serial.println("Ping OK !");
#endif

    magellan.setAuth(CONFIG_THING_IDENTIFY, CONFIG_THING_SECRET, CONFIG_THING_TOKEN, CONFIG_THING_NAME);
    magellan.addControlSensorHandle("CH1", [](JsonVariant value) {
        int value_i = value.as<int>();
        Serial.printf("Relay1 is %d\n", value_i);
#ifdef IFAMR_4G_BOARD
        io.digitalWrite(C1_PIN, value_i);
#else
        digitalWrite(BUILTIN_LED, value_i);
#endif
    });
    magellan.addControlSensorHandle("CH2", [](JsonVariant value) {
        int value_i = value.as<int>();
        Serial.printf("Relay2 is %d\n", value_i);
#ifdef IFAMR_4G_BOARD
        io.digitalWrite(C2_PIN, value_i);
#endif
    });
    magellan.addControlSensorHandle("CH3", [](JsonVariant value) {
        int value_i = value.as<int>();
        Serial.printf("Relay3 is %d\n", value_i);
#ifdef IFAMR_4G_BOARD
        io.digitalWrite(C3_PIN, value_i);
#endif
    });
    magellan.addControlSensorHandle("CH4", [](JsonVariant value) {
        int value_i = value.as<int>();
        Serial.printf("Relay4 is %d\n", value_i);
#ifdef IFAMR_4G_BOARD
        io.digitalWrite(C4_PIN, value_i);
#endif
    });
}

void loop() {
    if (magellan.isConnected()) {
        static uint32_t startTime = 0;
        if ((millis() < startTime) || (millis() - startTime) >= (30 * 1000) || (startTime == 0)) {
            startTime = millis();

#ifdef IFAMR_4G_BOARD
            magellan.setSensorValue("Temperature", SHT40.readTemperature());
            magellan.setSensorValue("Humidity", SHT40.readHumidity());
            magellan.setSensorValue("Soil Moisture", 74.1);
            magellan.setSensorValue("Light", 65412);
            if (!magellan.pushData()) {
                Serial.println("Magellan push data fail");
            }
#else
            magellan.setSensorValue("Temperature", SHT40.readTemperature());
            magellan.setSensorValue("Humidity", SHT40.readHumidity());
            magellan.setSensorValue("Soil Moisture", 74.1);
            magellan.setSensorValue("Light", 65412);
            if (!magellan.pushData()) {
                Serial.println("Magellan push data fail");
            }
#endif
        }
        magellan.loop();
    } else {
        if (!magellan.connect()) {
            Serial.println("Connect to Magellan fail !");
#ifdef USE_4G
            uint8_t simcom_work = false;
            for (uint8_t i=0;i<5;i++) {
                if (GSM.AT()) {
                    simcom_work = true;
                    break;
                }
                delay(100);
            }
            if (!simcom_work) {
                // TODO: Hard reset SIMCOM module
                ESP.restart();
            }
#endif
        }
    }
    delay(10);
}
