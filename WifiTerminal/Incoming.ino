// ----------------------------------------------------------
// Simple Incoming connection handling

//how many clients should be able to telnet to this ESP8266
#define MAX_SRV_CLIENTS 1

int WiFiLocalPort = TELNET_DEFAULT_PORT;

void Incoming()
{
  softSerial.print(F("\r\nIncoming port ("));
  softSerial.print(WiFiLocalPort);
  softSerial.print(F("): "));

  String strport = GetInput();

  if (strport.length() > 0)
  {
    WiFiLocalPort = strport.toInt();
    //setLocalPort(localport);  !!!! Write to EEPROM?
  }

  WiFiServer server(WiFiLocalPort);
  WiFiClient serverClients[MAX_SRV_CLIENTS];

  // Start the server 
  server.begin();
  server.setNoDelay(true);

  softSerial.print(F("\r\nWaiting for connection on "));
  softSerial.print(WiFi.localIP());
  softSerial.print(" port ");
  softSerial.println(WiFiLocalPort);

  while (1)
  {
    yield();

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

          softSerial.print(F("Incoming connection from "));
          softSerial.println(serverClients[i].remoteIP());

          // Handle incoming connection
          serverClients[i].println(F("CONNECTING..."));
          //CheckTelnet();
          TerminalMode(serverClients[i]);
          softSerial.println(F("Incoming connection closed."));
          continue;
        }
      }

      softSerial.println(F("DEBUG: Incoming connection but already connected ****"));

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