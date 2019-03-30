#include "DHTesp.h"
#include <Ticker.h>
#include "Thing.h"
#include "WebThingAdapter.h"
#include "user_config.h"

#define DHTPIN D4 // Pin which is connected to the DHT sensor.

#ifdef ESP32
#pragma message(THIS EXAMPLE IS FOR ESP8266 ONLY!)
#error Select ESP8266 board.
#endif

DHTesp dht;
Ticker sampler;
boolean isTimeToSample = false;



WebThingAdapter* adapter;

void sample(){
  isTimeToSample = true;
}

void setupDHT(){
  dht.setup(DHTPIN, DHTesp::DHT22);
  sampler.attach_ms(dht.getMinimumSamplingPeriod(), sample);
}

void setupWiFi(){
#if defined(LED_BUILTIN)
  const int ledPin = LED_BUILTIN;
#else
  const int ledPin = 13;  // manully configure LED pin
#endif
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);
  Serial.begin(115200);
  Serial.println("");
  Serial.print("Connecting to \"");
  Serial.print(STA_PASS1);
  Serial.println("\"");
#if defined(ESP8266) || defined(ESP32)
  WiFi.mode(WIFI_STA);
#endif
  WiFi.begin(STA_PASS1, STA_PASS1);
  Serial.println("");

  // Wait for connection
  bool blink = true;
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    Serial.print(".");
    digitalWrite(ledPin, blink ? LOW : HIGH); // active low led
    blink = !blink;
  }
  digitalWrite(ledPin, HIGH); // active low led

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(STA_PASS1);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void webThingSetup(){
    adapter = new WebThingAdapter("NodeMCU1", WiFi.localIP());
    

  Serial.println("HTTP server started");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.print("/things/");
}

void setup(){
  Serial.begin(115200);
  Serial.println();
  setupDHT();
  setupWiFi();
  webThingSetup();
}

void loop(){
  if (isTimeToSample){
    float humidity = dht.getHumidity();
    float temperature = dht.getTemperature();
    Serial.println("Status\tHumidity (%)\tTemperature (C)\tHeatIndex (C)");
    Serial.print(dht.getStatusString());
    Serial.print("\t");
    Serial.print(humidity, 1);
    Serial.print("\t\t");
    Serial.print(temperature, 1);
    Serial.print("\t\t");
    Serial.print("\t\t");
    Serial.print(dht.computeHeatIndex(temperature, humidity, false), 1);
    Serial.print("\t\t");
    isTimeToSample = false;
  }
}
