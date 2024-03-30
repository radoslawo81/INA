#include <Wire.h>               // Only needed for Arduino 1.6.5 and earlier
#include "SH1106Wire.h"   // legacy: #include "SH1106.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#define ONE_WIRE_BUS 12
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensory(&oneWire);
//ROM = 28 96 94 80 30 23 7 A5 ROM = 28 ED 97 ED 95 23 B E6 No more addresses.

DeviceAddress sensorA = { 0x28, 0x96, 0x94, 0x80, 0x30, 0x23, 0x7, 0xA5 };
DeviceAddress sensorB = { 0x28, 0xED, 0xB97, 0xED, 0x95, 0x23, 0xB, 0xE6 };
SH1106Wire display(0x3c, SDA, SCL);     // ADDRESS, SDA, SCL

#include <SDL_Arduino_INA3221.h>
SDL_Arduino_INA3221 ina3221;

int counter = 1;
int progress = (counter / 5) % 100;

float shuntvoltage1 = 0;
float busvoltage1 = 0;
float current_mA1 = 0;
float loadvoltage1 = 0;
float power1;

float shuntvoltage2 = 0;
float busvoltage2 = 0;
float current_mA2 = 0;
float loadvoltage2 = 0;
float power2;

float temperatura_A;
float temperatura_B;

#define LIPO_BATTERY_CHANNEL 1
#define SOLAR_CELL_CHANNEL 2
#define OUTPUT_CHANNEL 3



#if defined(ESP32)
#include <WiFiMulti.h>
WiFiMulti wifiMulti;
#define DEVICE "ESP32"
#elif defined(ESP8266)
#include <ESP8266WiFiMulti.h>
ESP8266WiFiMulti wifiMulti;
#define DEVICE "ESP8266"
#endif

#include <InfluxDbClient.h>

// WiFi AP SSID
#define WIFI_SSID "STAR"
// WiFi password
#define WIFI_PASSWORD "Mickiewicza1981!"
// InfluxDB  server url. Don't use localhost, always server name or ip address.
// E.g. http://192.168.1.48:8086 (In InfluxDB 2 UI -> Load Data -> Client Libraries), 
#define INFLUXDB_URL "http://192.168.50.167:8086"
// InfluxDB 2 server or cloud API authentication token (Use: InfluxDB UI -> Load Data -> Tokens -> <select token>)
#define INFLUXDB_TOKEN "wLyzoeHnR8DnGt1vhfSe5Gg2RfnwFdiA0NvWcF4VZX1bhyXho2cZjUt4PkHz1TTQoKVLX8VwEIKkeNXVzp2crw=="
// InfluxDB 2 organization id (Use: InfluxDB UI -> Settings -> Profile -> <name under tile> )
#define INFLUXDB_ORG "3ab194020d4e8700"
// InfluxDB 2 bucket name (Use: InfluxDB UI -> Load Data -> Buckets)
#define INFLUXDB_BUCKET "ESP8266"
// InfluxDB v1 database name 
//#define INFLUXDB_DB_NAME "database"

// InfluxDB client instance
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN);

// Data point
Point sensor("wifi_status");
Point sensor1("ina1");
Point sensor2("ina2");
Point sensor3("temp_ab");



void setup() {
    Serial.begin(9600);
    sensory.begin();
  Serial.println("Connecting to WiFi");
  WiFi.mode(WIFI_STA);
  wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);
  while (wifiMulti.run() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();

  // Add constant tags - only once
  sensor.addTag("device", DEVICE);
  sensor.addTag("SSID", WiFi.SSID());
  sensor1.addTag("V1", String(shuntvoltage1));
  sensor1.addTag("I1", String(current_mA1));
  sensor1.addTag("V12", String(loadvoltage1));
  sensor2.addTag("V2", String(shuntvoltage2));
  sensor2.addTag("I2", String(current_mA2));
  sensor2.addTag("V22", String(loadvoltage2));
  sensor3.addTag("temp_a", String(temperatura_A));
  sensor3.addTag("temp_b", String(temperatura_B));



  // Check server connection
  if (client.validateConnection()) {
    Serial.print("Connected to InfluxDB: ");
    Serial.println(client.getServerUrl());
  } else {
    Serial.print("InfluxDB connection failed: ");
    Serial.println(client.getLastErrorMessage());
  }

  Serial.begin(9600);
  Serial.println();
  Serial.println();
  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);

  ina3221.begin();
  
}


void loop() {
  
  
  display.clear();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(128, 54, String(millis()));
 

busvoltage1 = ina3221.getBusVoltage_V(LIPO_BATTERY_CHANNEL);
busvoltage1=busvoltage1;
shuntvoltage1 = ina3221.getShuntVoltage_mV(LIPO_BATTERY_CHANNEL);
current_mA1 = -ina3221.getCurrent_mA(LIPO_BATTERY_CHANNEL);  // minus is to get the "sense" right.   - means the battery is charging, + that it is discharging
loadvoltage1 = busvoltage1 + (shuntvoltage1 / 1000);
power1=(busvoltage1*current_mA1)/1000;

busvoltage2 = ina3221.getBusVoltage_V(SOLAR_CELL_CHANNEL);
busvoltage2=busvoltage2;
shuntvoltage2 = ina3221.getShuntVoltage_mV(SOLAR_CELL_CHANNEL);
current_mA2 = -ina3221.getCurrent_mA(SOLAR_CELL_CHANNEL);
loadvoltage2 = busvoltage2 + (shuntvoltage2 / 1000);
power2=(busvoltage2*current_mA2)/1000;


  display.drawString(0, 0, ("I: "+ String(current_mA1)+"mA"));
  display.drawString(0, 10, ("V: " + String(busvoltage1)+"V"));
  display.drawString(0, 20, ("P: " + String(power1)+"W"));
  

  
  display.drawString(60, 0, ("I2: "+ String(current_mA2)+"mA"));
  display.drawString(60, 10, ("V2: " + String(busvoltage2)+"V"));
  display.drawString(60, 20, ("P2: " + String(power2)+"W"));
 



Serial.print("LIPO_Battery Bus Voltage:   "); Serial.print(busvoltage1); Serial.println(" V");
Serial.print("LIPO_Battery Shunt Voltage: "); Serial.print(shuntvoltage1); Serial.println(" mV");
Serial.print("LIPO_Battery Load Voltage:  "); Serial.print(loadvoltage1); Serial.println(" V");
Serial.print("LIPO_Battery Current 1:       "); Serial.print(current_mA1); Serial.println(" mA");
Serial.println("");

Serial.print("Solar Cell Bus Voltage 2:   "); Serial.print(busvoltage2); Serial.println(" V");
Serial.print("Solar Cell Shunt Voltage 2: "); Serial.print(shuntvoltage2); Serial.println(" mV");
Serial.print("Solar Cell Load Voltage 2:  "); Serial.print(loadvoltage2); Serial.println(" V");
Serial.print("Solar Cell Current 2:       "); Serial.print(current_mA2); Serial.println(" mA");
Serial.println("");
temp_read();

display.drawString(0,  30, ("TempA: " + String(sensory.getTempC(sensorA))));
display.drawString(0, 40, ("TempB: " + String(sensory.getTempC(sensorB))));
 display.display();





// Store measured value into point
  sensor.clearFields();
  sensor1.clearFields();
  sensor2.clearFields();
  sensor3.clearFields();

  sensor.addField("rssi", WiFi.RSSI());
  
  sensor1.addField("bus_1", busvoltage1);
  sensor1.addField("current_mA1", current_mA1);

  sensor2.addField("bus_2", busvoltage2);
  sensor2.addField("current_mA2", current_mA2);

  sensor3.addField("tempeA", temperatura_A );
  sensor3.addField("tempeB", temperatura_B );



  Serial.print("Writing: ");
  Serial.println(client.pointToLineProtocol(sensor));
  // If no Wifi signal, try to reconnect it
  if (wifiMulti.run() != WL_CONNECTED) {
    Serial.println("Wifi connection lost");
  }
  // Write point
  if (!client.writePoint(sensor)) {
    Serial.print("InfluxDB write failed: ");
    Serial.println(client.getLastErrorMessage());
  }

   Serial.print("Writing: ");
  Serial.println(client.pointToLineProtocol(sensor1));
  // If no Wifi signal, try to reconnect it
  if (wifiMulti.run() != WL_CONNECTED) {
    Serial.println("Wifi connection lost");
  }
  // Write point
  if (!client.writePoint(sensor1)) {
    Serial.print("InfluxDB write failed1: ");
    Serial.println(client.getLastErrorMessage());
  }

   Serial.print("Writing: ");
  Serial.println(client.pointToLineProtocol(sensor2));
  // If no Wifi signal, try to reconnect it
  if (wifiMulti.run() != WL_CONNECTED) {
    Serial.println("Wifi connection lost");
  }
  // Write point
  if (!client.writePoint(sensor2)) {
    Serial.print("InfluxDB write failed2: ");
    Serial.println(client.getLastErrorMessage());
  }

  Serial.print("Writing: ");
  Serial.println(client.pointToLineProtocol(sensor3));
  // If no Wifi signal, try to reconnect it
 if (wifiMulti.run() != WL_CONNECTED) {
   Serial.println("Wifi connection lost");
  }
  // Write point
  if (!client.writePoint(sensor3)) {
    Serial.print("InfluxDB write failed3: ");
  Serial.println(client.getLastErrorMessage());
  }

 
 
  
  delay(1000);
}

void temp_read()
{

  Serial.print("Requesting temperatures...");
  sensory.requestTemperatures(); // Send the command to get temperatures
  Serial.println("DONE");
  temperatura_A=sensory.getTempC(sensorA);
  temperatura_B=sensory.getTempC(sensorB);

  
  Serial.print("Sensor 1(*C): ");
  Serial.println(temperatura_A); 
  
 
  Serial.print("Sensor 2(*C): ");
  Serial.println(temperatura_B); 
  
 
}