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
const char* dht22Types[] = {"TemperatureSensor","Sensor", nullptr};
ThingDevice dhtSensor("DHT22", "DHT22 Temperature & Humidity sensor", dht22Types);
ThingProperty tempSensorProperty("temperature", "Temperature", NUMBER, "TemperatureProperty");
ThingProperty humiditySensorProperty("humidity", "Humidity", NUMBER, nullptr);

const char* ledStripTypes[] = {"Light", "OnOffSwitch", "ColorControl", nullptr};
ThingDevice ledStrip("dimmable-color-light", "Dimmable Color Light", ledStripTypes);
ThingProperty ledStripOn("on", "Whether the led is turned on", BOOLEAN, "OnOffProperty");
ThingProperty ledStripLevel("level", "The level of light from 0-100", NUMBER, "BrightnessProperty");
ThingProperty ledStripColor("color", "The color of light in RGB", STRING, "ColorProperty");

bool lastOn = false;
String color = "#ffffff";

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

  ledStrip.addProperty(&ledStripOn);

  ThingPropertyValue colorValue;
  colorValue.string = &color; //default color is white
  ledStripColor.setValue(colorValue);
  ledStrip.addProperty(&ledStripColor);

  ThingPropertyValue levelValue;
  levelValue.number = 100;
  ledStripLevel.setValue(levelValue);
  ledStrip.addProperty(&ledStripLevel);
  adapter->addDevice(&ledStrip);

  adapter->begin();
  Serial.println("HTTP server started");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.print("/things/");
  Serial.println(dhtSensor.id);
}

/**
   hex2int
   take a 2 digit hex string and convert it to a integer
*/
int twoHex2int(String hex)
{
  int len = 2;
  int i;
  int val = 0;

  for (i = 0; i < len; i++) {
    if (hex[i] <= '9')
      val += (hex[i] - '0') * (1 << (4 * (len - 1 - i)));
    else if (hex[i] <= 'F')
      val += (hex[i] - 'A' + 10) * (1 << (4 * (len - 1 - i)));
    else
      val += (hex[i] - 'a' + 10) * (1 << (4 * (len - 1 - i)));
  }
  return val;
}

void updateLedStrip(String* color, int const level) {
  if (!color) return;
  int red, green, blue;
  if (color && (color->length() == 7) && color->charAt(0) == '#') {
    red = twoHex2int(color->substring(1, 3));
    green = twoHex2int(color->substring(3, 5));
    blue = twoHex2int(color->substring(5, 7));
  }
  for (int i = 0; i < NUM_PIXELS; i++)
  {
    pixels.setPixelColor(i, pixels.Color(red, green, blue));
    pixels.setBrightness(level);
    pixels.show();
  }
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
  bool on = ledStripOn.getValue().boolean;
  int level = ledStripLevel.getValue().number;
  updateLedStrip(&color, on ? level : 0);

  if (on != lastOn) {
    Serial.print(ledStrip.id);
    Serial.print(": on: ");
    Serial.print(on);
    Serial.print(", level: ");
    Serial.print(level);
    Serial.print(", color: ");
    Serial.println(color);
  }
  lastOn = on;

}
