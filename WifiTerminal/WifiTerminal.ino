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

// Defines for the Adafruit Feather HUZZAH ESP8266
#define BLUE_LED 2
#define RED_LED  0

SoftwareSerial softSerial(4, 5); // RX, TX

unsigned int BAUD_RATE = 9600;

String lastHost = "";
int lastPort = TELNET_DEFAULT_PORT;

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
}


void loop()
{
  digitalWrite(BLUE_LED, HIGH);  // HIGH=Off
  digitalWrite(RED_LED, HIGH);

  // Menu or Hayes AT command mode?
  mode_Hayes = EEPROM.read(ADDR_HAYES_MENU);

  if (mode_Hayes < 0 || mode_Hayes > 1)
  {
    mode_Hayes = 0;
  }

  if (mode_Hayes)
  {
    HayesEmulationMode();
  }
  else
  {
    ShowMenu();
  }
}

void ShowMenu()
{
  softSerial.print
  (F("\r\n"
     "1. Telnet to Host\r\n"
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
      mode_Hayes = true;
      updateEEPROMByte(ADDR_HAYES_MENU, mode_Hayes);
      softSerial.println(F("Hayes mode set.  Use AT&M to return to menu mode."));
      softSerial.println(""); 
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

// EOF