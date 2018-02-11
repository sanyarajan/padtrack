
#include <SPI.h>
#include <Ethernet2.h>
#include <PubSubClient.h>

#define DEBUG false

//ethernet
EthernetClient ethClient;
//MAC from ethernet shield
byte mac[] = { 0x90, 0xA2, 0xDA, 0x11, 0x12, 0x8A };
//byte ip[] = { 192, 168, 1, 148 };
IPAddress ip(192, 168, 1, 149);

IPAddress serverAddress(192, 168, 1, 157);

PubSubClient mqttClient(ethClient);

int sensorPin = A0; // select the input pin for the potentiometer
int sensorValue = 0; // variable to store the value coming from the sensor
String output = String();

int soundDetectionPin = 9; // Use Pin 9 as our Input
int soundDetectionVal = LOW; // This is where we record our Sound Measurement
long soundDetectionEventID = 0;
long deviceID = 0;
boolean isSoundDetected = false;
boolean wasSoundDetected = false;


unsigned long lastSoundDetectTime; // Record the time that we measured a sound
unsigned long lastSoundDetectID = 0;


int soundAlarmTime = 500; // Number of milli seconds to keep the sound alarm high

long lastReconnectAttempt = 0;

boolean reconnect() {
  if (mqttClient.connect("arduinoClient")) {
    // Once connected, publish an announcement...
    String registration = "{";
    registration += "\"deviceID\":\"";
    registration += deviceID;
    registration += "\"";
    registration += "}";

    mqttClient.publish("SoundDetectorRegistration", (char*)registration.c_str());

  }
  return mqttClient.connected();
}


void callback(char* topic, byte* payload, unsigned int length) {
  // handle message arrived
}


// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(soundDetectionPin, INPUT);
  Serial.begin(9600);

  if (DEBUG) // wait for serial port connection if we are debugging
  {
    while (!Serial) {
      ; 
    }
  }
  Serial.println("Connecting...");
  // start the Ethernet connection:
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // try to congifure using IP address instead of DHCP:
    Ethernet.begin(mac, ip);
  }

  // give the Ethernet shield a second to initialize:
  delay(1000);

  // print the shield's IP address:
  Serial.print("IP Address: ");
  Serial.println(Ethernet.localIP());

  mqttClient.setServer(serverAddress, 1883);
  mqttClient.setCallback(callback);

  randomSeed(analogRead(A0));

  deviceID = random();

}
String topic = "SoundDetected";

String postData = "{}";

// the loop function runs over and over again forever
void loop() {

  // try to reconnect if connection lost
  if (!mqttClient.connected()) {
    long now = millis();
    if (now - lastReconnectAttempt > 2000) {
      lastReconnectAttempt = now;
      // Attempt to reconnect
      if (reconnect()) {
        lastReconnectAttempt = 0;
      }
    }
  } else {
    // Client connected
    sensorValue = analogRead (sensorPin);

    soundDetectionVal = digitalRead (soundDetectionPin) ; // read the sound alarm time
    //Serial.println(soundDetectionVal);

    if (soundDetectionVal == HIGH) // If we hear a sound
    {


      lastSoundDetectTime = millis(); // record the time of the sound event
      if (!isSoundDetected) {
        lastSoundDetectID = random();
        //Serial.println("Sound detected: ");
        postData = "{";


        postData += "\"detectionID\":\"";
        postData += lastSoundDetectID;
        postData += "\"";

        postData += ",";

        postData += "\"soundLevel\":\"";
        postData += sensorValue;
        postData += "\"";

        postData += ",";

        postData += "\"detectionTick\":\"";
        postData += lastSoundDetectTime;
        postData += "\"";

        postData += ",";

        postData += "\"eventType\":\"";
        postData += "START";
        postData += "\"";


        postData += "}";

        Serial.println(postData);
        mqttClient.publish("SoundDetected", (char*)postData.c_str(), false);
        digitalWrite (LED_BUILTIN, HIGH);
        isSoundDetected = true;
      }
    }
    else // sound is no longer detected
    {
      // check if there is still sound detected within the alarm time
      if ( (millis() - lastSoundDetectTime) > soundAlarmTime  &&  isSoundDetected) {
        //Serial.println("No sound: ");
        postData = "{";


        postData += "\"detectionID\":\"";
        postData += lastSoundDetectID;
        postData += "\"";

        postData += ",";

        postData += "\"soundLevel\":\"";
        postData += sensorValue;
        postData += "\"";

        postData += ",";

        postData += "\"detectionTick\":\"";
        postData += lastSoundDetectTime;
        postData += "\"";

        postData += ",";

        postData += "\"eventType\":\"";
        postData += "END";
        postData += "\"";


        postData += "}";

        Serial.println(postData);
        mqttClient.publish("SoundDetected", (char*)postData.c_str(), false);
        digitalWrite (LED_BUILTIN, LOW);
        isSoundDetected = false;
      }
    }
  }

  mqttClient.loop();
}
