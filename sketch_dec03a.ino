#include <WiFi.h>
#include "dht.h"
#include <Wire.h>
#include <HTTPClient.h>
#include <PubSubClient.h>
#include <WiFiClient.h>
#include "SSD1306.h"
#include "AutoConnect.h"

#define DHT22_PIN 5

dht DHT;

HTTPClient http;
bool passedIp = false;
WebServer server;
AutoConnect Portal(server);
WiFiClient espClient;
const char *mqtt_server = "35.233.127.48";
PubSubClient client(espClient);

static char celsiusTemp[7];
static char humidityTemp[7];
SSD1306 display(0x3c, 21, 22);

char linebuf[80];
int charcount = 0;
int chk;
bool initialLoad = true;
float h, t, tempH, tempT;
int sensorCounter = 0;
static int taskCore = 0;

void loadSensorData()
{
  chk = DHT.read22(DHT22_PIN);
  tempH = DHT.humidity;
  tempT = DHT.temperature;
  if (tempH == -999.0 || tempT == -999.0)
  {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
  if (initialLoad)
  {
    h = tempH;
    t = tempT;
    initialLoad = false;
  }
  else if (abs(tempH - h) < 100 && abs(tempT - t) < 100)
  {
    h = tempH;
    t = tempT;
  }
  dtostrf(t, 6, 2, celsiusTemp);
  dtostrf(h, 6, 2, humidityTemp);
}

void getExternalIpData()
{
  String payload = String("");
  if ((WiFi.status() == WL_CONNECTED))
  {
    HTTPClient http;
    http.begin("http://api.ipify.org/?format=json");
    int httpCode = http.GET();

    if (httpCode > 0)
    {
      payload = http.getString();
    }
    else
    {
      Serial.println("Error on HTTP request");
    }

    http.end(); //Free the resources
  }
  client.publish("esp32/humidity", payload.c_str());
}

void showSensorData()
{
  display.resetDisplay();
  display.setFont(ArialMT_Plain_24);
  int span = display.getStringWidth("********");
  display.drawString(0, 0, celsiusTemp);
  display.drawString(0, 25, humidityTemp);
  display.drawString(span, 0, "*C");
  display.drawString(span, 25, "%");
  display.display();
}

void callback(char *topic, byte *message, unsigned int length)
{
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;

  for (int i = 0; i < length; i++)
  {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();
}

void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("esp32"))
    {
      Serial.println("connected");
      // Subscribe
      client.subscribe("esp32/output");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void isConnectingTo(String ssid)
{
  Serial.print("Connecting to ");
  Serial.println(ssid);
  while (WiFi.status() != WL_CONNECTED)
  {
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void checkAndConnect(String ssid)
{
  if (ssid.equals("dd-wrt_vap"))
  {
    WiFi.begin(ssid.c_str(), "");
  }
  else if (ssid.equals("MISZCZU ver.BETA"))
  {
    WiFi.begin(ssid.c_str(), "igorciul");
  }
  else if (ssid.equals("Glaadiss"))
  {
    WiFi.begin(ssid.c_str(), "worms1234");
  }
  else
  {
    return;
  }
  isConnectingTo(ssid);
}

void scanAndConnect()
{
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0)
  {
    Serial.println("no networks found");
  }
  else
  {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i)
    {
      checkAndConnect(WiFi.SSID(i));
      Serial.println(WiFi.SSID(i));
      delay(10);
    }
  }
}

void setup()
{
  Serial.begin(115200);
  display.init();

  while (!Serial)
  {
  }

  delay(500);
  loadSensorData();
  showSensorData();
//  scanAndConnect();
  if(Portal.begin()){
    Serial.println("WiFi connected: " + WiFi.localIP().toString());
  }
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  Serial.println("works");
}

void loop()
{
  Portal.handleClient();
  if (!client.connected())
  {
    reconnect();
    passedIp = false;
    delay(1000);
  }
  client.loop();
  if (!passedIp)
  {
    passedIp = true;
    getExternalIpData();
  }

  if (sensorCounter > 300000)
  {
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
