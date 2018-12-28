#include <Arduino.h>

#include <firebase/FirebaseArduino.h>
#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <DHT_U.h>
#include <ctime>

#include "../.piolibdeps/ArduinoJson/ArduinoJson.h"
#include "firebase/FirebaseObject.h"
#include "firebase/FirebaseArduino.h"

#define FIREBASE_HOST "pumper-eb154.firebaseio.com"
#define FIREBASE_AUTH "1N1ByyD9oBiZYiwXHLYATC4J9HrpxGRkUfECQSKq"

StaticJsonBuffer<512> buffer;

WiFiManager wifiManager;

const int DHTPIN = D3;
const int RELAY = D4;
const int MOIST_READ = A0;
const int DHTTYPE = DHT22;

DHT dht(DHTPIN, DHTTYPE);

int read_moist() {
    int result = analogRead(MOIST_READ);
    Serial.println(result);
    int tmp = map(result, 1024, 400, 0, 100);
    return min(max(0, tmp), 100);
}


void setup() {
    Serial.begin(9600);
    dht.begin();
    delay(1000);
    pinMode(RELAY, OUTPUT);
    digitalWrite(RELAY, LOW);
    pinMode(MOIST_READ, INPUT);
    Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
    Serial.println("Start");
}

JsonObject &data = buffer.createObject();

time_t get_time() {
    time_t now = time(nullptr);

    if (!now) {
        Serial.println("start gettting time");
        configTime(0, 0, "pool.ntp.org", "time.nist.gov");

        while (!time(nullptr)) {
            Serial.print("-");
            delay(100);
        }
        now = time(nullptr);
        Serial.println(ctime(&now));
        Serial.print(now);
    }
    return now;
}

void loop() {
    if (!WiFi.isConnected()) {
        wifiManager.autoConnect("CaiDatMayBom");
    }

    int moist = read_moist();

    FirebaseObject config = Firebase.get("/config");
    bool mode = config.getBool("auto");
    int moist_threshold = config.getInt("moist");
    Serial.print("Mode: ");
    Serial.println(mode);
    Serial.print("moist_threshold: ");
    Serial.println(moist_threshold);
    if (mode) {
        if (moist < moist_threshold) {
            digitalWrite(RELAY, LOW);//open
        } else {
            digitalWrite(RELAY, HIGH);//close
        }
    } else {
        int val = config.getInt("pump");
        Serial.print("Pump: ");
        Serial.println(val);
        digitalWrite(RELAY, val);
    }
    data["time"] = get_time();
    data["moist"] = moist;
    float temperature = dht.readTemperature(false, true);
    data["temp"] = temperature;
    float humidity = dht.readHumidity();
    data["humi"] = humidity;
    data["pump"] = digitalRead(RELAY);
    data.printTo(Serial);

    Serial.print("Temperature = ");
    Serial.print(temperature);
    Serial.println(" c");
    Serial.print("Humidity = ");
    Serial.print(humidity);
    Serial.println(" %");
    if (!(isnan(temperature) || isnan(humidity))) {
        Firebase.push("/data", data);
        Firebase.set("/current", data);
    }

    if (Firebase.failed()) {
        Serial.print("setting /number failed:");
        Serial.println(Firebase.error());
    }

    delay(2000);
}
