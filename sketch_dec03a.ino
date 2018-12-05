#include <WiFi.h*>
#include "dht.h"
#include <Wire.h>
#include <HTTPClient.h>

#include "SSD1306.h"

dht DHT;

#define DHT22_PIN 5


HTTPClient http;
const char* ssid     = "MISZCZU ver.BETA";
const char* password = "igorciul";

WiFiServer server(80);

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

void coreTask( void * pvParameters ){
    String taskMessage = "Task running on core ";
    taskMessage = taskMessage + xPortGetCoreID();
    while(true){
      Serial.println(taskMessage);
      delay(10000);
      loadSensorData();
      showSensorData();
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
  delay(1000);
  loadSensorData();
  showSensorData();
  WiFi.begin(ssid, password);

//  xTaskCreate(
//                    coreTask,   /* Function to implement the task */
//                    "coreTask", /* Name of the task */
//                    10000,      /* Stack size in words */
//                    NULL,       /* Task input parameter */
//                    1,          /* Priority of the task */
//                    NULL       /* Task handle. */
//                    ); 
  // attempt to connect to Wifi network:
  while (WiFi.status() != WL_CONNECTED) {
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  server.begin();
//  gettimeofday(past_time, NULL);
}

void loop() {

  //  digitalWrite (ledPin, HIGH);  // turn on the LED
  //  delay(500); // wait for half a second or 500 milliseconds
  //  digitalWrite (ledPin, LOW); // turn off the LED
  //  delay(500); // wait for half a second or 500 milliseconds

  if(sensorCounter > 1000000){
    sensorCounter = 0;
    loadSensorData();
    showSensorData();    
  }

  sensorCounter++;
  



  WiFiClient client = server.available();
  if (client) {
    Serial.println("New client");
    memset(linebuf, 0, sizeof(linebuf));
    charcount = 0;
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        linebuf[charcount] = c;
        if (charcount < sizeof(linebuf) - 1) charcount++;
        if (c == '\n' && currentLineIsBlank) {
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");  // the connection will be closed after completion of the response
          client.println();
          client.println("<!DOCTYPE HTML><html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
          client.println("<meta http-equiv=\"refresh\" content=\"30\"></head>");
          client.println("<body><div style=\"font-size: 3.5rem;\"><p>ESP32 - DHT</p><p>");
          if (atoi(celsiusTemp) >= 25) {
            client.println("<div style=\"color: #930000;\">");
          }
          else if (atoi(celsiusTemp) < 25 && atoi(celsiusTemp) >= 5) {
            client.println("<div style=\"color: #006601;\">");
          }
          else if (atoi(celsiusTemp) < 5) {
            client.println("<div style=\"color: #009191;\">");
          }
          client.println(celsiusTemp);
          client.println("*C</p><p>");
          client.println(fahrenheitTemp);
          client.println("*F</p></div><p>");
          client.println(humidityTemp);
          client.println("%</p></div>");
          client.println("</body></html>");
          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
          memset(linebuf, 0, sizeof(linebuf));
          charcount = 0;
        } else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);

    // close the connection:
    client.stop();
    Serial.println("client disconnected");
  }
}
