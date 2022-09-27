#include <Arduino.h>
#include <ArtronShop_Magellan.h>
#include <SHT40.h>
#include <WiFi.h>
#include <SIM76xx.h>
#include <GSMClient.h>
#include <Wire.h>
#include "Configs.h"

#if defined(USE_WIFI)
WiFiClient espClient;
ArtronShop_Magellan magellan(&espClient);
#elif defined(USE_4G)
GSMClient gsmClient;
ArtronShop_Magellan magellan(&gsmClient);
#endif

void setup() {
    pinMode(BUILTIN_LED, OUTPUT);  // Initialize the BUILTIN_LED pin as an output
    Serial.begin(115200);
    Wire.begin();
    SHT40.begin();

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
        digitalWrite(BUILTIN_LED, value_i);
    });
    magellan.addControlSensorHandle("CH2", [](JsonVariant value) {
        int value_i = value.as<int>();
        Serial.printf("Relay2 is %d\n", value_i);
    });
    magellan.addControlSensorHandle("CH3", [](JsonVariant value) {
        int value_i = value.as<int>();
        Serial.printf("Relay3 is %d\n", value_i);
    });
    magellan.addControlSensorHandle("CH4", [](JsonVariant value) {
        int value_i = value.as<int>();
        Serial.printf("Relay4 is %d\n", value_i);
    });
}

void loop() {
    if (magellan.isConnected()) {
        static uint32_t startTime = 0;
        if ((millis() - startTime) >= (30 * 1000) || (startTime == 0)) {
            startTime = millis();

            magellan.setSensorValue("Temperature", SHT40.readTemperature());
            magellan.setSensorValue("Humidity", SHT40.readHumidity());
            magellan.setSensorValue("Soil Moisture", 74.1);
            magellan.setSensorValue("Light", 65412);
            if (!magellan.pushData()) {
                Serial.println("Magellan push data fail");
            }
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
