#include <Wiegand.h>
#include <SPI.h>
#include <HttpClient.h>
#include <Ethernet.h>
#include <EthernetClient.h>

WIEGAND rfid;

// Enter a MAC address for your controller below.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
// Set the static IP address to use if the DHCP fails to assign
IPAddress ip(192,168,0,20);

// if you don't want to use DNS (and reduce your sketch size)
// use the numeric IP instead of the name for the server:
//IPAddress server(192,168,0,100);
const char server[] = "192.168.0.100";
//const char server[] = "www.google.com";    // name address for Google (using DNS)

const char script[] = "/test.php?device=12";

// Initialize the Ethernet client library
// with the IP address and port of the server 
// that you want to connect to (port 80 is default for HTTP):
EthernetClient client;

// 0 = off
// 1 = on
int interlockState = 0;
int err = 0;
char uri[32];

const int ledPin =  9;
const int stopButtonPin = 8;

void setup() {
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  
  // Start RFID reader
  rfid.begin();

  // start the Ethernet connection:
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // no point in carrying on, so do nothing forevermore:
    // try to congifure using IP address instead of DHCP:
    Ethernet.begin(mac, ip);
  } else {
    Serial.println("Connected via DHCP");
  }
  
  // give the Ethernet shield a second to initialize:
  delay(1000);

  // initialize the LED pin as an output:
  pinMode(ledPin, OUTPUT);      
  // initialize the pushbutton pin as an input:
  pinMode(stopButtonPin, INPUT);
  // turn on pullup resistors
  digitalWrite(stopButtonPin, HIGH); 
}

void loop() {
  int buttonState = digitalRead(stopButtonPin);
  if (buttonState == LOW) {
    deactivate();
  } else {
    if(rfid.available()) {
      unsigned long tag = rfid.getCode();
    
      Serial.print("Wiegand HEX = ");
      Serial.print(tag,HEX);
      Serial.print(", DECIMAL = ");
      Serial.print(tag);
      Serial.print(", Type W");
      Serial.println(rfid.getWiegandType());

      sprintf(uri, "%s&tag=%lu", script, tag);
    
      EthernetClient c;
      HttpClient http(c);
      err = http.get(server, uri);
      if (err == 0) {
        err = http.responseStatusCode();
        if (err == 204) {
          err = http.skipResponseHeaders();
          if (err >= 0) {
            Serial.println("Authorized!");
            activate();
          } else {
            Serial.print("Failed to skip response headers: ");
            Serial.println(err);
            deactivate();
          }
        } else {    
          Serial.print("Getting response failed: ");
          Serial.println(err);
          deactivate();
        }
      } else {
        Serial.print("Connection failed: ");
        Serial.println(err);
        deactivate();
      }
      http.stop();
    }
  }
}

void activate()
{
  interlockState = 1;
  digitalWrite(ledPin, HIGH);
  Serial.println("Device activated!");
}

void deactivate()
{
  Serial.println("Notifying server of device shutdown.");

  sprintf(uri, "%s&status=shutdown", script);

  EthernetClient c;
  HttpClient http(c); 
  err = http.get(server, uri);

  if (err == 0) {
    err = http.responseStatusCode();
    if (err == 200) {
      Serial.println("Notified server of device shutdown.");
    } else {    
      Serial.print("Getting response failed: ");
      Serial.println(err);
    }
  } else {
    Serial.print("Connection failed: ");
    Serial.println(err);
  }
  http.stop();

  interlockState = 0;
  digitalWrite(ledPin, LOW);
  Serial.println("Device de-activated!");
}
