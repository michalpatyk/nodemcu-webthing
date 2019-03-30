#include "DHTesp.h"
#include <Ticker.h>
#include "Thing.h"
#include "WebThingAdapter.h"
#include "user_config.h"
#include <Adafruit_NeoPixel.h>

#define DHTPIN D4 // Pin which is connected to the DHT sensor.
#define NUM_PIXELS 8
#define PIN_PIXELS D3

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUM_PIXELS, PIN_PIXELS, NEO_GRB + NEO_KHZ800);


#ifdef ESP32
#pragma message(THIS EXAMPLE IS FOR ESP8266 ONLY!)
#error Select ESP8266 board.
#endif

DHTesp dht;
Ticker sampler;
boolean isTimeToSample = false;

WebThingAdapter* adapter = NULL;
const char* deviceTypes[] = {"Sensor", "Sensor", nullptr};
ThingDevice dhtSensor("DHT22", "DHT22 Temperature & Humidity sensor", deviceTypes);
ThingProperty tempSensorProperty("temperature", "Temperature", NUMBER, "TemperatureProperty");
ThingProperty humiditySensorProperty("humidity", "Humidity", NUMBER, "HumidityProperty");

void sample() {
  isTimeToSample = true;
}

void setupDHT() {
  dht.setup(DHTPIN, DHTesp::DHT22);
  sampler.attach_ms(dht.getMinimumSamplingPeriod(), sample);
}

void setupWiFi() {
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
  Serial.print(STA_SSID1);
  Serial.println("\"");
#if defined(ESP8266) || defined(ESP32)
  WiFi.mode(WIFI_STA);
#endif
  WiFi.begin(STA_SSID1, STA_PASS1);
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
  Serial.println(STA_SSID1);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void webThingSetup() {
  adapter = new WebThingAdapter("NodeMCU1", WiFi.localIP());
  dhtSensor.addProperty(&tempSensorProperty);
  dhtSensor.addProperty(&humiditySensorProperty);
  adapter->addDevice(&dhtSensor);
  adapter->begin();
  Serial.println("HTTP server started");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.print("/things/");
  Serial.println(dhtSensor.id);
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  setupDHT();
  setupWiFi();
  webThingSetup();
  pixels.begin();
}

void loop() {
  if (isTimeToSample) {
    float humidity = dht.getHumidity();
    float temperature = dht.getTemperature();
    Serial.println("Status\tHumidity (%)\tTemperature (C)\tHeatIndex (C)");
    Serial.print(dht.getStatusString());
    Serial.print("\t");
    Serial.print(humidity, 1);
    Serial.print("\t\t");
    Serial.print(temperature, 1);
    Serial.print("\t\t");
    Serial.print(dht.computeHeatIndex(temperature, humidity, false), 1);
    Serial.print("\n");
    ThingPropertyValue tempValue;
    tempValue.number = temperature;
    tempSensorProperty.setValue(tempValue);
    ThingPropertyValue humidityValue;
    humidityValue.number = humidity;
    humiditySensorProperty.setValue(humidityValue);
    adapter->update();
    isTimeToSample = false;
  }

  
  int i;
  for (i = 0; i < NUM_PIXELS; i++)
  {
    pixels.setPixelColor(i, pixels.Color(127, 0, 0));
    pixels.show();
    delay(500);
  }

  for (i = NUM_PIXELS - 1; i >= 0; i--)
  {
    pixels.setPixelColor(i, pixels.Color(0, 127, 0));
    pixels.show();
    delay(500);
  }

}
