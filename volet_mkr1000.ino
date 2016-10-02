/*
   Control roller shutters through MKR1000 with http over wifi
 */
#include <SPI.h>
#include <WiFi101.h>

typedef __gnuc_va_list va_list;

// display message on serial port
#define DEBUG true
// display debug messages instead of controlling remote
#define MOCK false

// Circuit parameters
// UP and Down Button attached the the arduino inputs
const int DOWN = 6;
const int UP = 9;

const int NORMALLY_CLOSED = HIGH; // my relay is reversed
const int NORMALLY_OPENED = LOW;

// Allowed commands
const String COMMAND_UP = "up";
const String COMMAND_DOWN = "down";
const String COMMAND_SWITCH = "switch";
const String COMMAND_STATUS = "status";

// Available status
typedef enum { OPENED, CLOSED, UNKNOWN } status_type;
// Current shutters status
status_type status = UNKNOWN;

// Press delay on remote buttons
const int PRESS_DELAY = 1000;

// Network parameters
char ssid[] = "XXX fill me TODO";
char pass[] = "XXX fill me TODO";
IPAddress ip(192,168,0,20); // used only to hardcode ip address, could be done in dhcp
IPAddress gateway(192,168,0,1); // ip address used for the gateway
IPAddress dns(192,168,0,1); // not used at the moment

WiFiServer server(80);

#define BUFSIZE 255
int wifi_status = WL_IDLE_STATUS;     // the Wifi radio's status
// Buffer
char buffer[BUFSIZE];
int buff_index = 0;


void setup() {
    //Initialize serial and wait for port to open:
    Serial.begin(9600);
    while (!Serial) {
        ; // wait for serial port to connect. Needed for native USB port only
    }

    // initialize the digital pin as an output.
    pinMode(DOWN, OUTPUT);
    pinMode(UP, OUTPUT);
    digitalWrite(UP, NORMALLY_CLOSED);
    digitalWrite(DOWN,NORMALLY_CLOSED);


    // check for the presence of the shield:
    if (WiFi.status() == WL_NO_SHIELD) {
        Serial.println("WiFi shield not present");
        // don't continue:
        while (true);
    }

    // attempt to connect to Wifi network:
    int delay_ms = 0;
    while ( wifi_status != WL_CONNECTED) {
        Serial.print("Attempting to connect to WPA SSID: ");
        Serial.println(ssid);

        WiFi.config(ip, dns, gateway);

        // Connect to WPA/WPA2 network:
        wifi_status = WiFi.begin(ssid, pass);

        delay_ms += 1000;
        // wait x milliseconds for connection:
        delay(delay_ms);
    }
    char buf[255];
    sprintf(buf, "Connected after %s ms", delay_ms);
    debug(String(buf));

    // you're connected now, so print out the data:
    Serial.print("You're connected to the network");
    printCurrentNet();
    printWifiData();

    server.begin();

}

void loop() {
    WiFiClient client = server.available();   // listen for incoming clients

    // current implementation reads line by line, answers as soon as we get a
    // GET /..., then close the connection. Other lines from request are simply
    // considered as a new request which is dropped
    // TODO: read the entire request and then parse command correctly
    if (client) {
        String currentLine = "";
        while (client.connected()) {
            if (client.available()) {             // if there's bytes to read from the client,
                char c = client.read();
                if (c == '\n') {

                    // if the current line is blank, you got two newline characters in a row.
                    // that's the end of the client HTTP request, so send a response:
                    if (currentLine.length() == 0) {
                        break;
                    }
                    else {
                        currentLine = "";
                    }
                }
                else if (c != '\r') {
                    currentLine += c;
                }

                if (currentLine.endsWith(" ") && currentLine.startsWith("GET /")) {
                    debug("Command: ");
                    String command = readCommand(currentLine);
                    debug(command);
                    if (command == COMMAND_STATUS) {
                        printStatus(client);
                        break;
                    } else if (command == COMMAND_UP) {
                        openShutters();
                        printOK(client);
                        break;
                    } else if (command == COMMAND_DOWN) {
                        closeShutters();
                        printOK(client);
                        break;
                    } else if (command == COMMAND_SWITCH) {
                        switchShutters();
                        printOK(client);
                        break;
                    } else {
                        printError(client);
                        break;
                    }
                }
            }
        }
        client.stop();
        debug("client disconnected");
    }
}

// Open the shutters
void openShutters(){
    if (MOCK) {
        debug("Would open");
    } else {
        digitalWrite(UP, NORMALLY_OPENED);
        delay(PRESS_DELAY);
        digitalWrite(UP, NORMALLY_CLOSED);
        status = OPENED;
    }
}

// Close the shutters
void closeShutters(){
    if (MOCK) {
        debug("Would close");
    } else {
        digitalWrite(DOWN, NORMALLY_OPENED);
        delay(PRESS_DELAY);
        digitalWrite(DOWN, NORMALLY_CLOSED);
        status = CLOSED;
    }
}

void switchShutters() {
    if(status == OPENED) {
        closeShutters();
    } else {
        openShutters();
    }
}

void printResponse(WiFiClient client, int code, String message) {
    char req[ 255 ];
    sprintf(req, "HTTP/1.1 %d OK", code);
    client.println(req);
    client.println("Content-Type: application/json");
    client.println("Connnection: close");
    client.println("Access-Control-Allow-Origin: *");
    client.println("Cache-Control: no-cache, no-store, must-revalidate");
    client.println("Pragma: no-cache");
    client.println("Expires: 0");
    sprintf(req, "Content-Length: %d", message.length() + 4);
    client.println(req);
    client.println();
    client.println(message);
    client.println();
    debug("Response sent");
}

void debug(String message) {
#if DEBUG
    Serial.println(message);
#endif
}

String readCommand(String request) {
    //  we're only interested in the first part...
    int qr = request.indexOf('/') + 1;
    return request.substring(qr, request.indexOf(' ', qr));
}

void printError(WiFiClient client) {
    printResponse(client, 404,  "{\"error\": \"command_not_found\"}");
}

void printOK(WiFiClient client) {
    printResponse(client, 200, "{\"success\": true}");
}

void printStatus(WiFiClient client) {
    char req[ 10 ];
    int shutter_status = 0;
    switch (status) {
        case CLOSED:
            shutter_status = 100;
            break;
        case UNKNOWN:
            shutter_status = 50;
            break;
        case OPENED:
            shutter_status = 0;
            break;
    }
    sprintf(req, "%d", shutter_status);
    printResponse(client, 200, req);
}




void printWifiData() {
    // print your WiFi shield's IP address:
    IPAddress ip = WiFi.localIP();
    Serial.print("IP Address: ");
    Serial.println(ip);
    Serial.println(ip);

    // print your MAC address:
    byte mac[6];
    WiFi.macAddress(mac);
    Serial.print("MAC address: ");
    Serial.print(mac[5], HEX);
    Serial.print(":");
    Serial.print(mac[4], HEX);
    Serial.print(":");
    Serial.print(mac[3], HEX);
    Serial.print(":");
    Serial.print(mac[2], HEX);
    Serial.print(":");
    Serial.print(mac[1], HEX);
    Serial.print(":");
    Serial.println(mac[0], HEX);

}

void printCurrentNet() {
    // print the SSID of the network you're attached to:
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());

    // print the MAC address of the router you're attached to:
    byte bssid[6];
    WiFi.BSSID(bssid);
    Serial.print("BSSID: ");
    Serial.print(bssid[5], HEX);
    Serial.print(":");
    Serial.print(bssid[4], HEX);
    Serial.print(":");
    Serial.print(bssid[3], HEX);
    Serial.print(":");
    Serial.print(bssid[2], HEX);
    Serial.print(":");
    Serial.print(bssid[1], HEX);
    Serial.print(":");
    Serial.println(bssid[0], HEX);

    // print the received signal strength:
    long rssi = WiFi.RSSI();
    Serial.print("signal strength (RSSI):");
    Serial.println(rssi);

    // print the encryption type:
    byte encryption = WiFi.encryptionType();
    Serial.print("Encryption Type:");
    Serial.println(encryption, HEX);
    Serial.println();
}
