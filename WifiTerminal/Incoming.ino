// ----------------------------------------------------------
// Simple Incoming connection handling

void Incoming()
{
    WiFiClient FirstClient;

    softSerial.print(F("\r\nIncoming port ("));
    softSerial.print(WiFiLocalPort);
    softSerial.print(F("): "));

    String strport = GetInput();

    if (strport.length() > 0)
    {
        WiFiLocalPort = strport.toInt();
        //setLocalPort(localport);  !!!! Write to EEPROM?
    }

    // Start the server 
    wifi_server.begin();
    wifi_server.setNoDelay(true);

    softSerial.print(F("\r\nWaiting for connection on "));
    softSerial.print(WiFi.localIP());
    softSerial.print(" port ");
    softSerial.println(WiFiLocalPort);

    while (true)
    {
        // 0. Let the ESP8266 do its stuff in the background
        yield();
     
        // 1. Check for new connections
        if (wifi_server.hasClient())
        {
            FirstClient = wifi_server.available();

            softSerial.print(F("Incoming connection from "));
            softSerial.println(FirstClient.remoteIP());
            FirstClient.println(F("CONNECTING..."));
            //CheckTelnet(client);

            TerminalMode(FirstClient);
            FirstClient.stop();
            yield();

            softSerial.println(F("Incoming connection closed."));          
        }

        // 2. Check for cancel
        if (softSerial.available() > 0)  // Key hit
        {
            softSerial.read();  // Eat Character
            softSerial.println(F("Cancelled"));
            wifi_server.close();
            return;
        }
    }
}

// Handle first incoming connection
bool ConnectIncoming(WiFiClient client)
{
    
}

// Reject additional incoming connections
bool RejectIncoming(WiFiClient client)
{
    //no free/disconnected spot so reject
    client.write("\n\rSorry, server is busy\n\r\n\r");
    client.stop();
}