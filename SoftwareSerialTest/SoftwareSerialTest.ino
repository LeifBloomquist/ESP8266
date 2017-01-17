#include <ESP8266WiFi.h>
#include <SoftwareSerial.h>   // Special version for ESP8266, apparently
#include "C:\Leif\GitHub\ESP8266\Common\ssids.h"

//how many clients should be able to telnet to this ESP8266
#define MAX_SRV_CLIENTS 1

#define BLUE_LED 2
#define RED_LED  0

//WiFiServer server(23);
//WiFiClient serverClients[MAX_SRV_CLIENTS];

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

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
 
  // Start the server 
  //server.begin();
  //server.setNoDelay(true);   

  // Init message
  softSerial.println("READY.");
  //softSerial.print("Ready! Use 'telnet ");
  //softSerial.print(WiFi.localIP());
  //softSerial.println(" 23' to connect");
}


void loop()
{
  digitalWrite(BLUE_LED, HIGH);  // HIGH=Off
  digitalWrite(RED_LED, HIGH);

  DoTelnet();
}

  /*
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
  */



// ----------------------------------------------------------
// Telnet Handling

// Telnet Stuff
#define NVT_SE 240
#define NVT_NOP 241
#define NVT_DATAMARK 242
#define NVT_BRK 243
#define NVT_IP 244
#define NVT_AO 245
#define NVT_AYT 246
#define NVT_EC 247
#define NVT_GA 249
#define NVT_SB 250
#define NVT_WILL 251
#define NVT_WONT 252
#define NVT_DO 253
#define NVT_DONT 254
#define NVT_IAC 255

#define NVT_OPT_TRANSMIT_BINARY 0
#define NVT_OPT_ECHO 1
#define NVT_OPT_SUPPRESS_GO_AHEAD 3
#define NVT_OPT_STATUS 5
#define NVT_OPT_RCTE 7
#define NVT_OPT_TIMING_MARK 6
#define NVT_OPT_NAOCRD 10
#define NVT_OPT_TERMINAL_TYPE 24
#define NVT_OPT_NAWS 31
#define NVT_OPT_TERMINAL_SPEED 32
#define NVT_OPT_LINEMODE 34
#define NVT_OPT_X_DISPLAY_LOCATION 35
#define NVT_OPT_ENVIRON 36
#define NVT_OPT_NEW_ENVIRON 39

String lastHost = "";
int lastPort = 23;

void DoTelnet()
{
  int port = 23;

  softSerial.println();
  softSerial.print(F("\r\nTelnet host ("));
  softSerial.print(lastHost);
  softSerial.print(F("): "));

  String hostName = GetInput();

  if (hostName.length() > 0)
  {
    port = getPort();

    lastHost = hostName;
    lastPort = port;

    Connect(hostName, port, false);
  }
  else
  {
    if (lastHost.length() > 0)
    {
      port = getPort();

      lastPort = port;
      Connect(lastHost, port, false);
    }
    else
    {
      return;
    }
  }
}

int getPort(void)
{
  softSerial.println();
  softSerial.print(F("Port ("));
  softSerial.print(lastPort);
  softSerial.print(F("): "));

  String strport = GetInput();

  if (strport.length() > 0)
  {
    return(strport.toInt());
  }
  else
  {
    return(lastPort);
  }
}

void Connect(String host, int port, boolean raw)
{
  char temp[80];
  softSerial.println();
  softSerial.print(F("Connecting to "));
  softSerial.println(host);

  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  if (!client.connect(host.c_str(), port)) 
  {
    softSerial.println("Connection failed");
    return;
  }

  TerminalMode(client);
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
    while (client.available() > 0)
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
  while (in.available() == 0) {}
  return in.read();
}

boolean CheckTelnet(bool isFirstChar, bool telnetBinaryMode, Stream& client)
{
  int inpint, verbint, optint;                        //    telnet parameters as integers

  // First time through
  if (isFirstChar)
  {
    SendTelnetParameters(client);                         // Start off with negotiating
  }

  verbint = ReadByte(client);                          // receive negotiation verb character

  if (verbint == NVT_IAC && telnetBinaryMode)
  {
    return true;                                    // Received two NVT_IAC's so treat as single 255 data
  }

  switch (verbint) {                                  // evaluate negotiation verb character
  case NVT_WILL:                                      // if negotiation verb character is 251 (will)or
  case NVT_DO:                                        // if negotiation verb character is 253 (do) or
    optint = ReadByte(client);                       // receive negotiation option character

    switch (optint) {

    case NVT_OPT_SUPPRESS_GO_AHEAD:                 // if negotiation option character is 3 (suppress - go - ahead)
      SendTelnetDoWill(verbint, optint, client);
      break;

    case NVT_OPT_TRANSMIT_BINARY:                   // if negotiation option character is 0 (binary data)
      SendTelnetDoWill(verbint, optint, client);
      telnetBinaryMode = true;
      break;

    default:                                        // if negotiation option character is none of the above(all others)
      SendTelnetDontWont(verbint, optint, client);
      break;                                      //  break the routine
    }
    break;
  case NVT_WONT:                                      // if negotiation verb character is 252 (wont)or
  case NVT_DONT:                                      // if negotiation verb character is 254 (dont)
    optint = ReadByte(client);                       // receive negotiation option character

    switch (optint) {

    case NVT_OPT_TRANSMIT_BINARY:                   // if negotiation option character is 0 (binary data)
      SendTelnetDontWont(verbint, optint, client);
      telnetBinaryMode = false;
      break;

    default:                                        // if negotiation option character is none of the above(all others)
      SendTelnetDontWont(verbint, optint, client);
      break;                                      //  break the routine
    }
    break;
  case NVT_IAC:                                       // Ignore second IAC/255 if we are in BINARY mode
  default:
    ;
  }
  return false;
}

void SendTelnetDoWill(int verbint, int optint, Stream& client)
{
  client.write(NVT_IAC);                               // send character 255 (start negotiation)
  client.write(verbint == NVT_DO ? NVT_DO : NVT_WILL); // send character 253  (do) if negotiation verb character was 253 (do) else send character 251 (will)
  client.write((int16_t)optint);
}

void SendTelnetDontWont(int verbint, int optint, Stream& client)
{
  client.write(NVT_IAC);                               // send character 255   (start negotiation)
  client.write(verbint == NVT_DO ? NVT_WONT : NVT_DONT);    // send character 252   (wont) if negotiation verb character was 253 (do) else send character254 (dont)
  client.write((int16_t)optint);
}

void SendTelnetParameters(Stream& client)
{
  client.write(NVT_IAC);                               // send character 255 (start negotiation) 
  client.write(NVT_DONT);                              // send character 254 (dont)
  client.write(34);                                    // linemode

  client.write(NVT_IAC);                               // send character 255 (start negotiation)
  client.write(NVT_DONT);                              // send character 253 (do)
  client.write(1);                                     // echo
}


// EOF