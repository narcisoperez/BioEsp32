#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <FreeRTOS.h>
#include <HTTPClient.h>
#include <RadioLib.h>
#include <U8g2lib.h>
#include <SPI.h>
//#include "heltec.h"
#include <DallasTemperature.h>

// GPIO where the DS18B20 is connected to
const int oneWireBus = 6;     
// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(oneWireBus);
// Pass our oneWire reference to Dallas Temperature sensor 
DallasTemperature sensors(&oneWire);

#define PIN_MQ2 4
/*
#define LoRa_nss 8
#define LoRa_SCK 9
#define LoRa_MOSI 10
#define LoRa_MISO 11
#define LoRa_nrst 12
#define LoRa_busy 13
#define LoRa_dio1 14
*/
#define oled_sda 17
#define oled_scl 18
#define oled_rst 21
#define SW1 19
#define SW2 20

#define GREENLED 33
#define REDLED 34
#define LED 35

#define PIN_LM35       7
#define ADC_VREF_mV    3300.0 // in millivolt, el sensor era para Arduino
#define ADC_RESOLUTION 4096.0

U8G2_SSD1306_128X64_NONAME_1_SW_I2C u8g2(U8G2_R0, /* clock=*/oled_scl, /* data=*/oled_sda, /* reset=*/oled_rst);
//SX1262 radio = new Module(LoRa_nss, LoRa_dio1, LoRa_nrst, LoRa_busy);
 
//const char *SSID = "your_wifi-ssid";
//const char *PWD = "your_wifi_password";
const char *SSID = "CVTV KAURI";
const char *PWD = "35473531";
//const char *SSID = "Conectividad Cordoba";
//const char *PWD = "";

// Web server running on port 80
WebServer server(80);
 
// JSON data buffer
StaticJsonDocument<250> jsonDocument;
char buffer[250];
 
// env variable
int temperature;
int temperature1;
int temp_ant;
float tempC;
int gas;
float pressure;
const char* serverUrl = "http://192.168.40.34:5087/agregar";
const char* apiKey = "tu_api_key";

void enviarJSONaAPI(int temp) {
  // Iniciar la conexión HTTP
  HTTPClient http;
  http.begin(serverUrl);
  // Crear un objeto JSON
  StaticJsonDocument<200> jsonDoc;
  jsonDoc["Precio"] = temp; // Aquí puedes poner los datos que quieras enviar en formato JSON
  // Convertir el objeto JSON en una cadena
  String jsonString;
  serializeJson(jsonDoc, jsonString);
  // Configurar las cabeceras HTTP
  http.addHeader("Content-Type", "application/json");
  //http.addHeader("X-API-Key", apiKey);
  // Enviar la solicitud POST con el JSON
  int httpResponseCode = http.POST(jsonString);
  // Verificar la respuesta
  if (httpResponseCode > 0) {
    Serial.print("Respuesta del servidor: ");
    Serial.println(http.getString());
  } else {
    Serial.print("Error en la solicitud HTTP: ");
    Serial.println(httpResponseCode);  }
  // Cerrar la conexión HTTP
  http.end();
}

void connectToWiFi() {
  Serial.print("Connecting to ");
  Serial.println(SSID);
  WiFi.begin(SSID, PWD);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500); 
    // we can even make the ESP32 to sleep
  } 
  Serial.print("Connected. IP: ");
  Serial.println(WiFi.localIP());
}
 
void create_json(char *tag, float value, char *unit) { 
  jsonDocument.clear(); 
  jsonDocument["type"] = tag;
  jsonDocument["value"] = value;
  jsonDocument["unit"] = unit;
  serializeJson(jsonDocument, buffer);
  Serial.println("Buffer:");
  Serial.println(buffer);  
}
 
void add_json_object(char *tag, float value, char *unit) {
  JsonObject obj = jsonDocument.createNestedObject();
  obj["type"] = tag;
  obj["value"] = value;
  obj["unit"] = unit; 
}

void read_sensor_data(void * parameter) {
   for (;;) {
     //temperature = analogRead(2);
     //humidity = 20; // analogRead(37); //bme.readHumidity();
     pressure = 30; //bme.readPressure() / 100;
     Serial.println("Read sensor data");
 
     // delay the task
     vTaskDelay(60000 / portTICK_PERIOD_MS);
   }
}
 
void getTemperature() {
  Serial.println("Get temperature");
  //temperature = analogRead(2);
  int adcVal = analogRead(PIN_LM35); //lee entrada analogica
  //calcula el valor del voltaje en mV
  //OJO en la simulación del LM35 tengo que usar como valor de 
  //voltaje de referencia superior 5V, en lugar de 3.3V (ADC_VREF_mV 50000)
  //cuando lo monte en protoboard usaremos ADC_VREF_mV 33000
  float milliVolt = adcVal * (ADC_VREF_mV / ADC_RESOLUTION);
  float temperature = milliVolt / 10; 
  Serial.print(temperature);
  Serial.println(" C");
  
  create_json("temperature", temperature, "°C");
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", buffer);

}

void getTempDS18B20() {
  Serial.println("Get temp DS18B20");
  Serial.print("Requesting temperatures...");
  sensors.requestTemperatures(); // Send the command to get temperatures
  Serial.println("DONE");
  tempC = sensors.getTempCByIndex(0);
  if (tempC != DEVICE_DISCONNECTED_C)  {
    Serial.print("Temperature for the device 1 (index 0) is: ");
    Serial.println(tempC);  }
  else  {
    Serial.println("Error: Could not read temperature data");  }
  Serial.println(" C");
  create_json("temperature", tempC, "°C");
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", buffer);

}
 
void getGas() {
  Serial.println("Get Gas");
  //temperature = analogRead(2);
  int adcVal = analogRead(PIN_MQ2); //lee entrada analogica
  //calcula el valor del voltaje en mV
  //OJO en la simulación del LM35 tengo que usar como valor de 
  //voltaje de referencia superior 5V, en lugar de 3.3V (ADC_VREF_mV 50000)
  //cuando lo monte en protoboard usaremos ADC_VREF_mV 33000
  float milliVolt = adcVal * (ADC_VREF_mV / ADC_RESOLUTION);
  float temperature = milliVolt / 10; 
  Serial.print(temperature);
  Serial.println(" %");
  create_json("gas", gas, "%");
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", buffer);
}
 
void getPressure() {
  Serial.println("Get pressure");
  pressure = analogRead(2);
  create_json("pressure", pressure, "mBar");
  server.send(200, "application/json", buffer);
}
 
void getEnv() {
  Serial.println("Get env");
  jsonDocument.clear();
  add_json_object("temperature", temperature, "°C");
  add_json_object("humidity", tempC, "%");
  add_json_object("pressure", pressure, "mBar");
  serializeJson(jsonDocument, buffer);
  server.send(200, "application/json", buffer);
}

void handleOptions() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "POST");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
  server.send(200);
}

void handlePost() {
  if (server.hasArg("plain") == false) {
    //handle error here
  }

  String body = server.arg("plain");
  Serial.println(body);
  deserializeJson(jsonDocument, body);
  
  // Get RGB components
  int red = jsonDocument["red"];
  int green = jsonDocument["green"];
  int blue = jsonDocument["blue"];

  Serial.print("Red: ");
  Serial.print(red);
  if (red==1){
    digitalWrite(REDLED,true);
  }
  else
    digitalWrite(REDLED,false);
  if (green==1){
    digitalWrite(GREENLED,true);
  }
  else
    digitalWrite(GREENLED,false);
  //pixels.fill(pixels.Color(red, green, blue));
  delay(30);
  //pixels.show();

  // Respond to the client
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", "{}");
}
 
 
// setup API resources
void setup_routing() {
  
  server.on("/temperature", getTemperature);
  server.on("/tempDS18B20", getTempDS18B20);
  server.on("/pressure", getPressure);
  server.on("/humidity", getGas);
  server.on("/env", getEnv);
  server.on("/led", HTTP_POST, handlePost);
  server.on("/led", HTTP_OPTIONS, handleOptions);
  server.sendHeader("Access-Control-Allow-Origin", "*");
  // start server
  server.begin();
}

void setup_task() {
  xTaskCreate(
    read_sensor_data,    
    "Read sensor data",   // Name of the task (for debugging)
    1000,            // Stack size (bytes)
    NULL,            // Parameter to pass
    1,               // Task priority
    NULL             // Task handle
  );
}
 
void setup() {
   Serial.begin(9600);
  connectToWiFi();
  setup_task();
  setup_routing();  
  pinMode(REDLED,OUTPUT);
  digitalWrite(REDLED,HIGH);
  pinMode(GREENLED,OUTPUT);
  digitalWrite(GREENLED,HIGH);
  delay(100);
  digitalWrite(REDLED,LOW);
  digitalWrite(GREENLED,LOW);
  pinMode(SW1,PULLUP);
  pinMode(SW2,PULLUP);  
  u8g2.begin();
	pinMode(LED,OUTPUT);
  digitalWrite(LED,LOW); 
  Serial.println("Dallas Temperature IC Control Library Demo");
  sensors.begin();


}
 
void loop() {
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_ncenB14_tr);
    u8g2.drawStr(5, 20, "Biodigestor 1"); 
    u8g2.setFont(u8g2_font_ncenB10_tr);       
    u8g2.drawStr(4, 40, "Controlador OK");
    u8g2.drawStr(20, 60, "!! ISBH !!");
  } while (u8g2.nextPage());

  server.handleClient();
  temperature1=analogRead(2);
  if (!(temperature1 == temp_ant)){
    enviarJSONaAPI(temperature1);
    temp_ant=temperature1;
  }


  /*
  sensors.requestTemperatures(); 
  float temperatureC = sensors.getTempCByIndex(0);
  float temperatureF = sensors.getTempFByIndex(0);
  Serial.print(temperatureC);
  Serial.println("ºC");
  Serial.print(temperatureF);
  Serial.println("ºF");
  */
  delay(250);
 
}