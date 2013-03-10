#include <SPI.h>
#include <WiFi.h>
#include <dht.h>

#define dht_dpin A0 //no ; here. Set equal to channel sensor is on
#define ldr_dpin A1 //no ; here. Set equal to channel sensor is on

dht DHT;

char ssid[] = "PfefferNET";      //  your network SSID (name) 
char pass[] = "whatawonderfulworld";   // your network password
int keyIndex = 0;            // your network key Index number (needed only for WEP)

int status = WL_IDLE_STATUS;

// Initialize the Wifi client library
WiFiClient client;

// server address:
char server[] = "http://nameless-anchorage-9071.herokuapp.com/sensor_readings";
//IPAddress server(64,131,82,241);

unsigned long lastConnectionTime = 0;           // last time you connected to the server, in milliseconds
boolean lastConnected = false;                  // state of the connection last time through the main loop
const unsigned long postingInterval = 60*1000;  // delay between updates, in milliseconds

void setup() {
  // start serial port:
  Serial.begin(9600);
  delay(1000);//Let system settle
  // attempt to connect to Wifi network:
  while ( status != WL_CONNECTED) { 
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass);
    // wait 10 seconds for connection:
    delay(10000);
  } 
  // you're connected now, so print out the status:
  printWifiStatus();
  httpRequest("{\"id\":746,\"metadata\":\"blah blah\",\"reading\":\"72\",\"sensor_id\":null,\"timestamp\":null,\"unit\":\"F\"}");
}

void loop() {
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

  // if you're not connected, and ten seconds have passed since
  // your last connection, then connect again and send data:
  if(!client.connected() && (millis() - lastConnectionTime > postingInterval)) {
    httpRequest("{\"id\":746,\"metadata\":\"blah blah\",\"reading\":\"72\",\"sensor_id\":null,\"timestamp\":null,\"unit\":\"F\"}");
    DHT.read11(dht_dpin);
    Serial.print("Current humidity = ");
    Serial.print(DHT.humidity);
    Serial.print("%  ");
    Serial.print("temperature = ");
    Serial.print(DHT.temperature); 
    Serial.println("C  ");
  }

  // store the state of the connection for next time through
  // the loop:
  lastConnected = client.connected();
}

String buildString(int reading, String unit, String timestamp, String sensor_id){
    String data = "{";
    data = data + "\"reading\":\""   + reading   + "\",";
    data = data + "\"unit\":\""      + unit      + "\",";
    data = data + "\"timestamp\":\"" + timestamp + "\",";
    data = data + "\"sensor_id\":\"" + sensor_id + "\"";
    data = data + "}";
}

// this method makes a HTTP connection to the server:
void httpRequest(String data) {
  // if there's a successful connection:
  if (client.connect(server, 80)) {
    Serial.println("connecting...");
    // send the HTTP POST request:
    client.println("POST /sensor_readings HTTP/1.1");
    client.println("Host: nameless-anchorage-9071.herokuapp.com");
    client.println("Content-Type: application/json");
    //client.println("Connection: close");
    //client.print( "Content-Length: " );
    //client.println( data.length() );
    client.println();
    client.print(data);
    client.println();
    //Serial.println(data);
    // note the time that the connection was made:
    lastConnectionTime = millis();
    Serial.println(lastConnectionTime);

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