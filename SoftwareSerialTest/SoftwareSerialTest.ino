#include <ESP8266WiFi.h>
#include <SoftwareSerial.h>   // Special version for ESP8266, apparently
#include "C:\Leif\GitHub\ESP8266\Common\ssids.h"

//how many clients should be able to telnet to this ESP8266
#define MAX_SRV_CLIENTS 1

#define BLUE_LED 2
#define RED_LED  0

WiFiServer server(23);
WiFiClient serverClients[MAX_SRV_CLIENTS];

SoftwareSerial softSerial(4, 5); // RX, TX

void setup() 
{  
  Serial.begin(115200);
  softSerial.begin(9600);

  // LEDs
  pinMode(BLUE_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  digitalWrite(BLUE_LED, HIGH);  // HIGH=Off
  digitalWrite(RED_LED, HIGH);

  WiFi.begin(ssid, password);
  softSerial.print("\nConnecting to ");
  softSerial.println(ssid);

  uint8_t i = 0;
  while (WiFi.status() != WL_CONNECTED && i++ < 20) delay(500);
  
  if(i == 21)
  {
    softSerial.print("Could not connect to"); 
    softSerial.println(ssid);
    while(1) delay(500);
  }
 
  // Start the server 
  server.begin();
  server.setNoDelay(true);   

  // Init message
  softSerial.print("Ready! Use 'telnet ");
  softSerial.print(WiFi.localIP());
  softSerial.println(" 23' to connect");
}


void loop() 
{  
  digitalWrite(BLUE_LED, HIGH);  // HIGH=Off
  digitalWrite(RED_LED, HIGH);

  uint8_t i;
  //check if there are any new clients
  if (server.hasClient())
  {
    for (i = 0; i < MAX_SRV_CLIENTS; i++)
    {
      //find free/disconnected spot
      if (!serverClients[i] || !serverClients[i].connected())
      {
        if (serverClients[i]) serverClients[i].stop();
        serverClients[i] = server.available();
        softSerial.print("New client: "); Serial.println(i);
        continue;
      }
    }

    //no free/disconnected spot so reject
    WiFiClient serverClient = server.available();
    serverClient.write("\n\rSorry, server is busy\n\r\n\r");
    serverClient.stop();
  }

  //check clients for data
  for (i = 0; i < MAX_SRV_CLIENTS; i++)
  {
    if (serverClients[i] && serverClients[i].connected())
    {
      if (serverClients[i].available())
      {
        digitalWrite(BLUE_LED, LOW);  // Low=On

        //get data from the telnet client and push it to the UART
        while (serverClients[i].available())
        {
          softSerial.write(serverClients[i].read());
        }
      }
    }
  }

  //check UART for data
  if (softSerial.available())
  {
    digitalWrite(RED_LED, LOW);  // Low=On

    size_t len = softSerial.available();
    uint8_t sbuf[len];
    softSerial.readBytes(sbuf, len);
    //push UART data to all connected telnet clients

    for (i = 0; i < MAX_SRV_CLIENTS; i++)
    {
      if (serverClients[i] && serverClients[i].connected())
      {
        serverClients[i].write(sbuf, len);
        delay(1);
      }
    }
  }
}
