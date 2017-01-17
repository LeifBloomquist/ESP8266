#include <ESP8266WiFi.h>
#include <SoftwareSerial.h>   // Special version for ESP8266, apparently

//how many clients should be able to telnet to this ESP8266
#define MAX_SRV_CLIENTS 1

WiFiServer server(23);
WiFiClient serverClients[MAX_SRV_CLIENTS];

SoftwareSerial mySerial(4, 5); // RX, TX

void setup() 
{  
  Serial.begin(115200);
  mySerial.begin(9600);

  // Blue LED
  pinMode(2, OUTPUT);
  digitalWrite(2, HIGH);

  /*
  /WiFi.begin(ssid, password);
  //Serial.print("\nConnecting to "); Serial.println(ssid);
  uint8_t i = 0;
  while (WiFi.status() != WL_CONNECTED && i++ < 20) delay(500);
  
  if(i == 21)
  {
    Serial.print("Could not connect to"); Serial.println(ssid);
    while(1) delay(500);
  }
  //start UART and the server
  // Serial.begin(115200);
  server.begin();
  server.setNoDelay(true);   


  
  Serial.print("Ready! Use 'telnet ");
  Serial.print(WiFi.localIP());
  Serial.println(" 23' to connect");

  */
}


void loop() 
{  
   digitalWrite(2, LOW);
   Serial.println("Hello World Serial");
   mySerial.println("Hello World SoftwareSerial");

   digitalWrite(2, HIGH);   
   delay(1000);
}
