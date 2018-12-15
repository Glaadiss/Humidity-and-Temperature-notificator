#include <WiFi.h>
#include "dht.h"
#include <Wire.h>
#include <HTTPClient.h>
#include <PubSubClient.h>
#include <WiFiClient.h>
#include "SSD1306.h"

dht DHT;

#define DHT22_PIN 5


HTTPClient http;
const char* ssid = "Glaadiss";
const char* password = "worms1234";
const char* ssid2     = "MISZCZU ver.BETA";
const char* password2 = "igorciul";

WiFiServer server(80);
WiFiClient espClient;
const char* mqtt_server = "35.234.88.22";
PubSubClient client(espClient);

static char celsiusTemp[7];
static char fahrenheitTemp[7];
static char humidityTemp[7];
SSD1306 display(0x3c, 21, 22);

// Client variables
char linebuf[80];
int charcount = 0;
int chk;
float h, t, f;
int sensorCounter = 0;
static int taskCore = 0;

void loadSensorData() {
  chk = DHT.read22(DHT22_PIN);
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  h = DHT.humidity;
  // Read temperature as Celsius (the default)
  t = DHT.temperature;
  // Read temperature as Fahrenheit (isFahrenheit = true)
  f = DHT.temperature;
  Serial.println(h);
  Serial.println(t);

  if (h == -999.0 || t == -999.0) {
    Serial.println("Failed to read from DHT sensor!");
  }
  else {
    dtostrf(t, 6, 2, celsiusTemp);
    dtostrf(t, 6, 2, fahrenheitTemp);
    dtostrf(h, 6, 2, humidityTemp);
  }

}

void getExternalIp()
{
  WiFiClient client;
  if (!client.connect("api.ipify.org", 80)) {
    Serial.println("Failed to connect with 'api.ipify.org' !");
  }
  else {
    int timeout = millis() + 5000;
    client.print("GET /?format=json HTTP/1.1\r\nHost: api.ipify.org\r\n\r\n");
    while (client.available() == 0) {
      if (timeout - millis() < 0) {
        Serial.println(">>> Client Timeout !");
        client.stop();
        return;
      }
    }
    int size;
    while ((size = client.available()) > 0) {
      uint8_t* msg = (uint8_t*)malloc(size);
      size = client.read(msg, size);
      Serial.write(msg, size);
      free(msg);
    }
  }
}

void showSensorData(){
//  display.setColor(BLACK);
//  display.fillRect(0, 0, 120, 80);
//  display.setColor(INVERSE);
  display.resetDisplay();
  display.setFont(ArialMT_Plain_24);
  int span = display.getStringWidth("********");
  display.drawString(0, 0, celsiusTemp);
  display.drawString(0, 25, humidityTemp);
  display.drawString(span, 0, "*C");
  display.drawString(span, 25, "%");
  display.display();
}

void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  // Feel free to add more if statements to control more GPIOs with MQTT

  // If a message is received on the topic esp32/output, you check if the message is either "on" or "off". 
  // Changes the output state according to the message
  if (String(topic) == "esp32/output") {

  }
}


void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("esp32")) {
      Serial.println("connected");
      // Subscribe
      client.subscribe("esp32/output");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


void setup() {
  Serial.begin(115200);
  display.init();

  while (!Serial) {

  }

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  delay(500);
  loadSensorData();
  showSensorData();
  WiFi.begin(ssid, password);
  Serial.print("Connecting to ");

  while (WiFi.status() != WL_CONNECTED) {
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    delay(500);
    Serial.print(".");
  }
  getExternalIp();
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
//  gettimeofday(past_time, NULL);
}

void loop() {

  //  digitalWrite (ledPin, HIGH);  // turn on the LED
  //  delay(500); // wait for half a second or 500 milliseconds
  //  digitalWrite (ledPin, LOW); // turn off the LED
  //  delay(500); // wait for half a second or 500 milliseconds
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  
  if(sensorCounter > 1000000){
    sensorCounter = 0;
    loadSensorData();
    showSensorData();
    char json[80];
    strcpy(json, "{ \"temperature\":");
    strcat(json, celsiusTemp);
    strcat(json, ", \"humidity\":");
    strcat(json, humidityTemp);
    strcat(json, "}");
    client.publish("esp32/humidity", json);
  }
  sensorCounter++;
 
}
