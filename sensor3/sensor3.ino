#include <SPI.h>
#include <WiFi.h>
#include <DHT.h>

/* ---- DHT TEMP AND HUMIDITY INITIALIZATION CODE ----- */
#define DHTPIN 2     // what pin we're connected to

// Uncomment whatever type you're using!
#define DHTTYPE DHT11   // DHT 11 
//#define DHTTYPE DHT22   // DHT 22  (AM2302)
//#define DHTTYPE DHT21   // DHT 21 (AM2301)

// Connect pin 1 (on the left) of the sensor to +5V
// Connect pin 2 of the sensor to whatever your DHTPIN is
// Connect pin 4 (on the right) of the sensor to GROUND
// Connect a 10K resistor from pin 2 (data) to pin 1 (power) of the sensor

DHT dht(DHTPIN, DHTTYPE);

unsigned long dhtLastConnectionTime = 0;
const unsigned long dhtReadInterval = 5000;
String humidity;
String temperature;

/* --- DHT TEMP AND HUMIDITY INITIALIZATION CODE END --- */

/* ------------ WiFi INITIALIZATION CODE --------------- */

char ssid[] = "PfefferNET";      //  your network SSID (name) 
char pass[] = "whatawonderfulworld";   // your network password
int keyIndex = 0;            // your network key Index number (needed only for WEP)

int status = WL_IDLE_STATUS;

// Initialize the Wifi client library
WiFiClient temperatureClient;
WiFiClient humidityClient;

// server address:
//char server[] = "www.arduino.cc";
char server[] = "192.168.1.90"; int port = 3000;
//char server[] = "nameless-anchorage-9071.herokuapp.com/"; int port = 80;
//IPAddress server(64,131,82,241);


/* ------------- WiFi INITIALIZATION END ---------------- */

/* -------------------- LDR CODE ------------------------ */

int ldr_pin = 2;

String readStringLightLevel(){
  int light_level = analogRead(ldr_pin);
  String result;
  if (light_level < 50)
    result = "very dark";
  else if (light_level < 125)
    result = "dim";
  else
    result = "bright";
}

/* ------------------ LDR CODE END ---------------------- */

unsigned long temperatureLastConnectionTime = 0;           // last time you connected to the server, in milliseconds
boolean temperatureLastConnected = false;                  // state of the connection last time through the main loop
unsigned long humidityLastConnectionTime = 0;           // last time you connected to the server, in milliseconds
boolean humidityLastConnected = false;                  // state of the connection last time through the main loop

const unsigned long postingInterval = 60000;  // delay between updates, in milliseconds
int com_error_count = 0;
int com_success_count = 0;
char tmp[10];  //char buffer for number to string conversions
int resetPin = 12;

void setup() {
  // start serial port:
  Serial.begin(9600);
  dht.begin();

  // attempt to connect to Wifi network:
  com_error_count--;
  while ( status != WL_CONNECTED) { 
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass);
    // wait 10 seconds for connection:
    delay(10000);
    if(com_error_count > 10)
      softwareReset();
  } 
  // you're connected now, so print out the status:
  printWifiStatus();
}

void loop() {
  if(millis() - dhtLastConnectionTime > dhtReadInterval) {
    // read DHT sensor
    double h = dht.readHumidity();
    double t = dht.readTemperature();  
    // make sure readings are numbers
    if (isnan(t) || isnan(h)) {
      Serial.println("Failed to read from DHT");
    } else {
      humidity = floatToString(h,1);
      Serial.println(humidity);
      temperature = floatToString(t,1);
      Serial.println(temperature);
    }
    dhtLastConnectionTime = millis();
  }
  
  String data = "{\"sensor_reading\":{\"reading\":\"";
  data += temperature;
  data += "\", \"unit\":\"F\", \"sensor_id\":\"1\"}}";
  postDataToServer(temperatureClient, data, temperatureLastConnected, temperatureLastConnectionTime);
  data = "{\"sensor_reading\":{\"reading\":\"";
  data += humidity;
  data += "\", \"unit\":\"pct\", \"sensor_id\":\"2\"}}";
  postDataToServer(humidityClient, data, humidityLastConnected, humidityLastConnectionTime);
}

// this method makes a HTTP connection to the server:
void httpRequest(WiFiClient client, String data, unsigned long &lastConnectionTime) {
  // if there's a successful connection:
  if (client.connect(server, port)) {
    Serial.println(data);
    Serial.println("connecting...");
    // send the HTTP POST request:
    client.println("POST /sensor_readings HTTP/1.1");
    client.println("Host: 192.168.1.90");
    client.println("User-Agent: arduino-ethernet");
    client.println("Content-Type: application/json");
    client.println("Connection: close");
    client.print( "Content-Length: " );
    client.println( data.length() );
    client.println();
    client.print(data);
    client.println();

    // note the time that the connection was made:
    lastConnectionTime = millis();
  } 
  else {
    // if you couldn't make a connection:
    Serial.println("connection failed");
    Serial.println("disconnecting.");
    client.stop();
  }
}

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

void softwareReset() // Restarts program from beginning but does not reset the peripherals and registers
{
    asm volatile ("  jmp 0");
}

void postDataToServer(WiFiClient client, String data, boolean &lastConnected, unsigned long &lastConnectionTime){
  // if there's incoming data from the net connection.
  // send it out the serial port.  This is for debugging
  // purposes only:
  while (client.available()) {
    char c = client.read();
    Serial.write(c);
  }
  
  // if there's no net connection, but there was one last time
  // through the loop, then stop the client:
  if (!client.connected() && lastConnected) {
    Serial.println();
    Serial.println("disconnecting.");
    client.stop();
  }
  
  // if you're not connected, and sixty seconds have passed since
  // your last connection, then connect again and send data:
  if(!client.connected() && (millis() - lastConnectionTime > postingInterval)) {
    httpRequest(client, data, lastConnectionTime);
    // if two consecutive missed connections, increment 
    if (!client.connected() && !lastConnected) {
      com_error_count++;
      if(com_error_count > 10)
        softwareReset();
    }
  }

  // store the state of the connection for next time through the loop:
  if(client.connected()){
    lastConnected = true;
    com_success_count++;
    // if the connection succeeds 30 consecutive times, reset com_error_count to zero.
    if(com_success_count > 30){
      com_success_count = 0;
      com_error_count = 0;
    }
  }
}

//Rounds down (via intermediary integer conversion truncation)
String floatToString(double input,int decimalPlaces){
  if(decimalPlaces!=0){
    String string = String((int)(input*pow(10,decimalPlaces)));
    if(abs(input)<1){
      if(input>0)
        string = "0"+string;
      else if(input<0)
        string = string.substring(0,1)+"0"+string.substring(1);
    }
    return string.substring(0,string.length()-decimalPlaces)+"."+string.substring(string.length()-decimalPlaces);
  }
  else {
    return String((int)input);
  }
}
