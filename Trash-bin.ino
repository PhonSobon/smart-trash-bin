#include <WiFi.h>
#include <PubSubClient.h>
#include <HTTPClient.h>
#include <TinyGPS++.h>
#include <ESP32Servo.h>

// Constants for WiFi and MQTT
#define BOT_TOKEN "6413828101:AAGNvkCAkJpLkGToVaOPw7ZKdPrVLN_azLc"
#define CHAT_ID "-4274668427"

const char* ssid = "OPPO Reno10 5G";
const char* password = "heyyy143";
const char* mqtt_server = "broker.netpie.io";
const int mqtt_port = 1883;
const char* mqtt_Client = "0a2f4c80-c9fc-4ffa-bf62-46198467ec17";
const char* mqtt_username = "ehuh73WJ7ZPQAnEFw2MT95gjPkpnD9PY";
const char* mqtt_password = "xsznRfmAZRz2HoteEnNB9GsiinE5z9sA";

// Constants for hardware connections
const int echoPin = 14;
const int trigPin = 27;
const float binHeight = 30.0; // Height of the bin in cm

// Constants for ultrasonic for servo motor
const int UlechoPin = 18;
const int UltrigPin = 19;
const float lenght = 30.0; // Lenght of people in front of bin in cm

// Constants for servo motor
const int servoPin = 25;
int pos = 0;

Servo myservo;
WiFiClient espClient;
TinyGPSPlus gps;
PubSubClient client(espClient);

// Use hardware serial port 2 for GPS
HardwareSerial mySerial(2);

long duration;
int distance;
long Ulduration;
int Uldistance;
int binLevel;

void setup_wifi() {
  Serial.begin(9600);
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  // Handle incoming messages if necessary
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(mqtt_Client, mqtt_username, mqtt_password)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void sendTelegramMessage(String message) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = "https://api.telegram.org/bot" + String(BOT_TOKEN) + "/sendMessage?chat_id=" + String(CHAT_ID) + "&text=" + message;

    Serial.print("Telegram URL: ");
    Serial.println(url);

    http.begin(url);
    int httpCode = http.GET();

    Serial.print("HTTP Code: ");
    Serial.println(httpCode);

    if (httpCode > 0) {
      String payload = http.getString();
      Serial.println("Telegram Response:");
      Serial.println(payload);
    } else {
      Serial.println("Error sending message to Telegram.");
    }
    http.end();
  } else {
    Serial.println("WiFi not connected. Cannot send message to Telegram.");
  }
}


void setup() {
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  pinMode(UltrigPin, OUTPUT);
  pinMode(UlechoPin, INPUT);

  myservo.attach(servoPin);

  Serial.begin(9600);
  delay(10);
  Serial.println();
  Serial.println("Initializing...");

  setup_wifi();

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  // Initialize hardware serial for GPS
  mySerial.begin(9600, SERIAL_8N1, 16, 17); // TX=17, RX=16

  Serial.println("Setup completed.");
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  
  // Measure distance and calculate bin level
  digitalWrite(UltrigPin, HIGH);
  delayMicroseconds(1000);
  digitalWrite(UltrigPin, LOW);

  Ulduration = pulseIn(UlechoPin, HIGH);
  Uldistance = Ulduration * 0.034 / 2; // Convert to distance in cm

  Serial.print("People Distance: ");
  Serial.print(Uldistance);
  Serial.println(" cm");
  if(Uldistance < 30){
    pos = 90;              
    myservo.write(pos); 
    Serial.println("Open Bin");
    delay(1000);             
  }
  else {
    pos = 0;              
    myservo.write(pos); 
    Serial.println("Closed Bin");
    delay(1000);
    // Measure distance and calculate bin level
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    duration = pulseIn(echoPin, HIGH);
    distance = duration * 0.034 / 2; // Convert to distance in cm
    binLevel = map(distance, binHeight, 0, 0, 100); // Map distance to bin level percentage

    Serial.print("Bin Level: ");
    Serial.print(distance);
    Serial.println(" cm");

    // Construct JSON payload
    String payload = "{\"data\":{\"reported\":{\"distance\": " + String(distance) + ", \"level\": " + String(binLevel) + "}}}";

    // Convert payload to char array
    char msg[payload.length() + 1];
    payload.toCharArray(msg, payload.length() + 1);

    // Publish to Netpie shadow
    client.publish("@shadow/data/update", msg);
    if (binLevel >= 85) {
      sendTelegramMessage("⚠️ Trash bin is approaching full capacity. Current level: " + String(binLevel));
    } else if (binLevel >= 95) {
      sendTelegramMessage("🗑 Trash bin is full!");
    }
  }
  delay(1000); // Delay between measurements
}