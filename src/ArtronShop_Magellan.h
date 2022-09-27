#pragma once

#include <Arduino.h>
#include <ArduinoJson-v6.18.3.h>
#include <PubSubClient.h>

typedef std::function<void(JsonVariant value)> ControlSensorHandle;

class ArtronShop_Magellan {
    private:
        DynamicJsonDocument *jsonDoc = NULL;
        Client *tcpClient = NULL;

    public:
        PubSubClient *mqttClient = NULL;

        ArtronShop_Magellan(Client *c);

        void setAuth(String thing_identify, String thing_secret, String thing_token = "", String thing_name = "") ;
        bool connect() ;
        bool isConnected() ;

        // Push Data
        #define DEFIND_SET_SETNSOR_FN(TYPE) void setSensorValue(String name, TYPE value)
        DEFIND_SET_SETNSOR_FN(String) ;
        DEFIND_SET_SETNSOR_FN(char*) ;
        DEFIND_SET_SETNSOR_FN(int) ;
        DEFIND_SET_SETNSOR_FN(unsigned int) ;
        DEFIND_SET_SETNSOR_FN(long) ;
        DEFIND_SET_SETNSOR_FN(unsigned long) ;
        DEFIND_SET_SETNSOR_FN(float) ;
        DEFIND_SET_SETNSOR_FN(double) ;
        bool pushData() ;

        // Get sensor data (Control)
        void addControlSensorHandle(String name, ControlSensorHandle handle) ;

        void loop() ;

        void stop() ;
        ~ArtronShop_Magellan();

};

