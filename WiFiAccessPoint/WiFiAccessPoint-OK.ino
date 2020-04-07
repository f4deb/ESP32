/*
  WiFiAccessPoint.ino creates a WiFi access point and provides a web server on it.

  Steps:
  1. Connect to the access point "yourAp"
  2. Point your web browser to http://192.168.4.1/H to turn the LED on or http://192.168.4.1/L to turn it off
     OR
     Run raw TCP "GET /H" and "GET /L" on PuTTY terminal with 192.168.4.1 as IP address and 80 as port

  Created for arduino-esp32 on 04 July, 2018
  by Elochukwu Ifediora (fedy0)
*/

#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>

#define LED_BUILTIN 2   // Set the GPIO pin where you connected your test LED or comment this line out if your dev board has a built-in LED

// Set these to your desired credentials.
const char *ssid = "Titan";
const char *password = "FocusRobotique";

WiFiServer server(23);


void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  Serial2.begin(115200);
  
  Serial.println();
  Serial.println("Configuring access point...");

  // You can remove the password parameter if you want the AP to be open.
  WiFi.softAP(ssid, password);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  server.begin();

  Serial.println("Server started");
}

void loop() {
  WiFiClient client = server.available();   // listen for incoming clients

  if (client) {                             // if you get a client,
    Serial.println("Client connected!");
    digitalWrite(LED_BUILTIN, HIGH);               // GET /H turns the LED on
    Serial2.print("z");
    Serial2.print("Lw4E657720");           // print a message out the serial port
    Serial2.print("Lw436C6965");           // print a message out the serial port
    Serial2.print("Lw6E742E2E");           // print a message out the serial port
    //String currentLine = "";                // make a String to hold incoming data from the client
   
    //check clients for data


    while (client.connected()) {            // loop while the client's connected
      
      if (client.available()) {             // if there's bytes to read from the client,             
        Serial2.write(client.read());        // read a byte, then print it out the serial monitor
      }    
      

      //check UART for data
      if(Serial2.available()){
        size_t len = Serial2.available();
        uint8_t sbuf[len];
        Serial2.readBytes(sbuf, len);
        //push UART data to all connected telnet clients
        client.write(sbuf, len);
            //JJdelay(1);
      }       
    }
    
    // close the connection:
    client.stop();
    digitalWrite(LED_BUILTIN, LOW);                // GET /L turns the LED off
    Serial.println("Client Disconnected!");
  }
}
