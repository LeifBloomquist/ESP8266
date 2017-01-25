/*
ANSI Wifi Terminal - WiFiModem ESP8266
Copyright 2015-2016 Leif Bloomquist and Alex Burger

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2
as published by the Free Software Foundation.
*/

/* ESP8266 port by Alex Burger */
/* Written with assistance and code from Greg Alekel and Payton Byrd */

/* ChangeLog
January 24, 2017: Leif Bloomquist
- Major refactoring plus support for ANSI WYSE Terminal instead of C64

Sept 18th, 2016: Alex Burger
- Change Hayes/Menu selection to variable
...
*/

#include <ESP8266WiFi.h>
#include <SoftwareSerial.h>   // Special version for ESP8266, apparently
#include <EEPROM.h>
#include "Telnet.h"
#include "ADDR_EEPROM.h"
#include "C:\Leif\GitHub\ESP8266\Common\ssids.h"

#define VERSION "ESP 0.13"

//how many clients should be able to telnet to this ESP8266
#define MAX_SRV_CLIENTS 1

// Defines for the Adafruit Feather HUZZAH ESP8266
#define BLUE_LED 2
#define RED_LED  0

//WiFiServer server(23);
//WiFiClient serverClients[MAX_SRV_CLIENTS];

SoftwareSerial softSerial(4, 5); // RX, TX

unsigned int BAUD_RATE = 9600;

String lastHost = "";
int lastPort = 23;

void setup() 
{  
  // Serial connections
  Serial.begin(115200);
  softSerial.begin(BAUD_RATE);

  // LEDs
  pinMode(BLUE_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  digitalWrite(BLUE_LED, HIGH);  // HIGH=Off
  digitalWrite(RED_LED, HIGH);

  // EEPROM
  EEPROM.begin(1024);

  // Opening Message and connect to Wifi
  softSerial.println();
  softSerial.println();
  AnsiClearScreen(softSerial);
  softSerial.print("Connecting to ");
  softSerial.print(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    softSerial.print(".");
  }

  softSerial.println("WiFi connected!");

  ShowInfo(true);
 
  // Start the server 
  //server.begin();
  //server.setNoDelay(true);   
}


void loop()
{
  digitalWrite(BLUE_LED, HIGH);  // HIGH=Off
  digitalWrite(RED_LED, HIGH);

  ShowMenu();
}

void ShowMenu()
{
  softSerial.print
  (F("1. Telnet to Host\r\n"
     "2. Phone Book\r\n"
     "3. Wait for Incoming Connection\r\n"
     "4. Configuration\r\n"
     "5. Hayes Emulation Mode \r\n"
     "\r\n"
     "Select: "));

  int option = ReadByte(softSerial);
  softSerial.println((char)option);   // Echo back

  switch (option)
  {
    case '1':
      DoTelnet();
      break;

    case '2':
      PhoneBook();
      break;

    case '3':
      Incoming();
      break;

    case '4':
      Configuration();
      break;

    case '5':
   //   mode_Hayes = true;
    //  updateEEPROMByte(ADDR_HAYES_MENU, mode_Hayes);
      softSerial.println(F("Restarting in Hayes Emulation mode."));
      softSerial.println(F("Use AT&M to return to menu mode."));
      softSerial.println("NOT IMPLEMENTED - rebooting!");  // !!!!
      ESP.restart();
      while (1);
      break;

    case '\n':
    case '\r':
    case ' ':
      break;

    default:
      softSerial.println(F("Unknown option, try again"));
      break;
    }
}

void Configuration()
{
  while (true)
  {
    char temp[30];
    softSerial.print
      (F("\r\n" 
      "Configuration Menu\r\n" 
      "\r\n"
      "1. Display Current Configuration\r\n"
      "2. Change SSID\r\n"
      "3. Return to Main Menu\r\n"
      "\r\nSelect: "));

    int option = ReadByte(softSerial);
    softSerial.println((char)option);  // Echo back

    switch (option)
    {
      case '1':
        ShowInfo(false);
        delay(100);        // Sometimes menu doesn't appear properly after
        break;

      case '2':
        //ChangeSSID();
        break;

      case '3': return;

      case '\n':
      case '\r':
      case ' ':
        continue;

      default: softSerial.println(F("Unknown option, try again"));
        continue;
    }
  }
}

// ----------------------------------------------------------
// Show Configuration

void ShowInfo(boolean powerup)
{
  softSerial.println();

  softSerial.print(F("IP Address:  "));    
  softSerial.println(WiFi.localIP());

  yield();  // For 300 baud

  softSerial.print(F("IP Subnet:   "));    
  softSerial.println(WiFi.subnetMask());
   
  yield();  // For 300 baud

  softSerial.print(F("IP Gateway:  "));   
  softSerial.println(WiFi.gatewayIP());

  yield();  // For 300 baud

  softSerial.print(F("Wi-Fi SSID:  "));    
  softSerial.println(WiFi.SSID());

  yield();  // For 300 baud

  softSerial.print(F("MAC Address: "));    
  softSerial.println(WiFi.macAddress());

  yield();  // For 300 baud

  softSerial.print(F("DNS IP:      "));
  softSerial.println(WiFi.dnsIP());

  yield();  // For 300 baud

  softSerial.print(F("Hostname:    "));   
  softSerial.println(WiFi.hostname());

  yield();  // For 300 baud

  softSerial.print(F("Firmware:    "));
  softSerial.println(VERSION);

  yield();  // For 300 baud

  //C64Print(F("Listen port: "));    C64Serial.print(WiFlyLocalPort); C64Serial.println();
  yield();  // For 300 baud
}


void TerminalMode(WiFiClient client)
{
  int i = 0;
  char buffer[10];
  int buffer_index = 0;
  int buffer_bytes = 0;
  bool isFirstChar = true;
  bool isTelnet = false;
  bool telnetBinaryMode = false;

  while (client.connected())
  {
    digitalWrite(BLUE_LED, HIGH);  // HIGH=Off
    digitalWrite(RED_LED, HIGH);

    // Get data from the telnet client and push it to the UART client
    if (client.available() > 0)
    {
      digitalWrite(BLUE_LED, LOW);  // Low=On

      int data = client.read();

      // If first character back from remote side is NVT_IAC, we have a telnet connection.
      if (isFirstChar)
      {
        if (data == NVT_IAC)
        {
          isTelnet = true;
          CheckTelnet(isFirstChar, telnetBinaryMode, client);
        }
        else
        {
          isTelnet = false;
        }
        isFirstChar = false;
      }
      else  // Connection already established, but may be more telnet control characters
      {
        if ((data == NVT_IAC) && isTelnet)
        {
          if (CheckTelnet(isFirstChar, telnetBinaryMode, client))
          {
            softSerial.write(NVT_IAC);
          }
        }
        else   //  Finally regular data - just pass the data along.
        {
          softSerial.write(data);
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
      client.write(sbuf, len);
      delay(1);  // needed?
    }
  } // while (client.connected())
}

// ----------------------------------------------------------
// Simple Incoming connection handling

int WiFiLocalPort = 0;

void Incoming()
{
  int localport = WiFiLocalPort;

  softSerial.print(F("\r\nIncoming port ("));
  softSerial.print(localport);
  softSerial.print(F("): "));

  String strport = GetInput();

  if (strport.length() > 0)
  {
    localport = strport.toInt();
    //setLocalPort(localport);  !!!! Write to EEPROM?
  }

  WiFiLocalPort = localport;

  WiFiServer server(localport);
  WiFiClient serverClients[MAX_SRV_CLIENTS];

  while (1)
  {
    // Force close any connections that were made before we started listening, as
    // the WiFly is always listening and accepting connections if a local port
    // is defined.
    server.close();

    softSerial.print(F("\r\nWaiting for connection on port "));
    softSerial.println(WiFiLocalPort);

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

          softSerial.println(F("Incoming Connection"));  // From....?         


          // Handle incoming connection
          serverClients[i].println(F("CONNECTING..."));
          //CheckTelnet();
          TerminalMode(serverClients[i]);
          continue;
        }
      }

      //no free/disconnected spot so reject
      WiFiClient serverClient = server.available();
      serverClient.write("\n\rSorry, server is busy\n\r\n\r");
      serverClient.stop();
    }
    else
    {
      if (softSerial.available() > 0)  // Key hit
      {
        softSerial.read();  // Eat Character
        softSerial.println(F("Cancelled"));
        server.close();
        return;
      }
    }
  }
}

// ----------------------------------------------------------
// User Input Handling

boolean IsBackSpace(char c)
{
  if ((c == 8) || (c == 20) || (c == 127))
  {
    return true;
  }
  else
  {
    return false;
  }
}

String GetInput()
{
  String temp = GetInput_Raw();
  temp.trim();
  return temp;
}

String GetInput_Raw()
{
  char temp[80];

  int max_length = sizeof(temp);

  int i = 0; // Input buffer pointer
  char key;

  while (true)
  {
    key = ReadByte(softSerial);  // Read in one character

    if (!IsBackSpace(key))  // Handle character, if not backspace
    {
      temp[i] = key;
      softSerial.write(key);    // Echo key press back to the user

      if (((int)key == 13) || (i >= (max_length - 1)))   // The 13 represents enter key.
      {
        temp[i] = 0; // Terminate the string with 0.
        return String(temp);
      }
      i++;
    }
    else     // Backspace
    {
      if (i > 0)
      {
        softSerial.write(key);
        i--;
      }
    }

    // Make sure didn't go negative
    if (i < 0) i = 0;
  }
}


// ----------------------------------------------------------
// Helper functions for read/peek

int ReadByte(Stream& in)
{
  while (in.available() == 0) 
  {
    yield();
  }
  return in.read();
}


// EOF