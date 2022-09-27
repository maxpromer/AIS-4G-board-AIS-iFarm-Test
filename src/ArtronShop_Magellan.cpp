#include <Arduino.h>
#include <ArduinoJson-v6.18.3.h>
#include "ArtronShop_Magellan.h"

#ifdef MAGELLAN_DEBUG
#define LOG(FORMAT, ...) Serial.printf("[Magellan]: " FORMAT "\n", ## __VA_ARGS__)
#else
#define LOG(a, ...) 
#endif

// --- Configs
#define JSON_DOC_BUFFER_SIZE (512)
#define JSON_DOC_BUFFER2_SIZE (512)

// Magellan
// #define MAGELLAN_SERVER      "119.31.104.1"
// #define MAGELLAN_SERVER      "enterprise-magellan.ais.co.th"
#define MAGELLAN_SERVER      "119.31.104.48"
#define MAGELLAN_PORT        1883

struct {
    String identify;
    String secret;
    String token;
    String name;
} thingAuthInfo;

typedef struct {
    String name;
    ControlSensorHandle handle;
    void* next;
} ControlSensorHandleList_t;

ControlSensorHandleList_t *controleHandleFirst = NULL;
ControlSensorHandleList_t *controleHandleLast = NULL;

void mqtt_callback(char *topic, byte *payload, unsigned int length) {
    String payloadString = "";
    for (int i = 0; i < length; i++) {
        payloadString += (char)payload[i];
    }
    LOG("Message arrived [%s]: %s", topic, payloadString.c_str());

    if (String(topic).endsWith("/delta/resp")) {
        DynamicJsonDocument jsonDoc(JSON_DOC_BUFFER2_SIZE);
        if (deserializeJson(jsonDoc, payloadString) == DeserializationError::Ok) {
            if (jsonDoc.containsKey("Sensor")) {
                JsonObject sensor_list = jsonDoc["Sensor"].as<JsonObject>();
                ControlSensorHandleList_t *next = controleHandleFirst;
                while (next != NULL) {
                    ControlSensorHandleList_t *it = next;
                    if (sensor_list.containsKey(it->name)) {
                        if (it->handle) {
                            // LOG("Call function handle address : %d", it->handle);
                            it->handle(sensor_list.getMember(it->name).as<JsonVariant>());
                        }
                    }
                    next = (ControlSensorHandleList_t *) it->next;
                }
            }
        }
    }
}

ArtronShop_Magellan::ArtronShop_Magellan(Client *c) {
    this->mqttClient = new PubSubClient(*c);
    this->jsonDoc = new DynamicJsonDocument(JSON_DOC_BUFFER_SIZE);
    this->jsonDoc->clear();

    this->mqttClient->setServer(MAGELLAN_SERVER, MAGELLAN_PORT);
    this->mqttClient->setCallback(mqtt_callback);
}

void ArtronShop_Magellan::setAuth(String thing_identify, String thing_secret, String thing_token, String thing_name) {
    thingAuthInfo.identify = thing_identify;
    thingAuthInfo.secret = thing_secret;
    thingAuthInfo.token = thing_token;

    // Gen Client Id
    if (thing_name.length() == 0) {
        randomSeed(micros());
        String clientId = "ESP8266Client-";
        clientId += String(random(0xffff), HEX);
        thingAuthInfo.name = clientId;
    }
}

bool ArtronShop_Magellan::connect() {
    LOG("Attempting MQTT connection...");
    LOG(" --- Client ID: %s", thingAuthInfo.name.c_str());
    LOG(" --- Username: %s", thingAuthInfo.identify.c_str());
    LOG(" --- Password: %s", thingAuthInfo.secret.c_str());
    if (this->mqttClient->connect(thingAuthInfo.name.c_str(), thingAuthInfo.identify.c_str(), thingAuthInfo.secret.c_str())) {
        LOG("connected");
        this->mqttClient->subscribe(String("api/v2/thing/" + String(thingAuthInfo.token) + "/report/resp").c_str());
        this->mqttClient->subscribe(String("api/v2/thing/" + String(thingAuthInfo.token) + "/delta/resp").c_str());

        this->mqttClient->publish(String("api/v2/thing/" + String(thingAuthInfo.token) + "/delta/req").c_str(), "");

        return true;
    }

    return false;
}

bool ArtronShop_Magellan::isConnected() {
    return this->mqttClient->connected();
}

#define DEFINE_SET_SETNSOR_FN_CODE(TYPE) \
    void ArtronShop_Magellan::setSensorValue(String name, TYPE value) { \
        this->jsonDoc->getOrAddMember(name).set(value); \
    }

DEFINE_SET_SETNSOR_FN_CODE(String);
DEFINE_SET_SETNSOR_FN_CODE(char *);
DEFINE_SET_SETNSOR_FN_CODE(int);
DEFINE_SET_SETNSOR_FN_CODE(unsigned int);
DEFINE_SET_SETNSOR_FN_CODE(long);
DEFINE_SET_SETNSOR_FN_CODE(unsigned long);
DEFINE_SET_SETNSOR_FN_CODE(float);
DEFINE_SET_SETNSOR_FN_CODE(double);

bool ArtronShop_Magellan::pushData() {
    if (!this->isConnected()) {
        return false;
    }

    String dataJsonString = "";
    serializeJson(*this->jsonDoc, dataJsonString);
    this->jsonDoc->clear();
    String topic = "api/v2/thing/" + String(thingAuthInfo.token) + "/report/persist";
    bool res = this->mqttClient->publish(topic.c_str(), dataJsonString.c_str());
    LOG("Push: %s", dataJsonString.c_str());

    if (!res) {
        return false;
    }

    return true;
}

void ArtronShop_Magellan::addControlSensorHandle(String name, ControlSensorHandle handle) {
    ControlSensorHandleList_t* it = new ControlSensorHandleList_t;
    it->name = name;
    it->handle = handle;
    it->next = NULL;
    if (!controleHandleFirst) {
        controleHandleFirst = it;
    } else {
        ((ControlSensorHandleList_t*) controleHandleLast)->next = (void *) it;
    }
    controleHandleLast = it;
}

void ArtronShop_Magellan::loop() {
    if (this->mqttClient->connected()) {
        this->mqttClient->loop();
    }
}

void ArtronShop_Magellan::stop() {
    if (this->jsonDoc) {
        delete this->jsonDoc;
    }
    if (this->mqttClient) {
        delete this->mqttClient;
    }
}

ArtronShop_Magellan::~ArtronShop_Magellan() {
    this->stop();
}
