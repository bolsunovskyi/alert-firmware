#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <Syslog.h>
#include <WiFiUdp.h>
//#include <EEPROM.h>

//#define POWER_PIN 5
//#define BRIGHTNESS_PIN 4
//#define POWER_TOPIC "shouse/cmnd/nodemcu-1/POWER"
//#define BRIGHTNESS_TOPIC "shouse/cmnd/nodemcu-1/BRIGHTNESS"
#define ALERT_TOPIC "shouse/tele/nodemcu-1/ALERT"

#define WIFI_SSID "MARVEL"
#define WIFI_PASSWORD "sosishurup"
#define WIFI_HOSTNAME "nodemcu-1"

#define SYSLOG_SERVER "192.168.1.5"
#define SYSLOG_PORT 514
#define SYSLOG_DEVICE_HOSTNAME WIFI_HOSTNAME
#define SYSLOG_APP_NAME WIFI_HOSTNAME
#define OTA_PASSWORD "53263952360"

#define MQTT_SERVER "192.168.1.5"
#define MQTT_PORT 1883


#define ALERT_PIN 14
#define WHITE_LED 2
#define ESP_LED 0
#define RED_BTN 4
#define BLACK_BTN 5
#define ALERT_INTERVAL 700

WiFiClient espClient;
PubSubClient client(espClient);

WiFiUDP udpClient;
Syslog syslog(udpClient, SYSLOG_SERVER, SYSLOG_PORT, SYSLOG_DEVICE_HOSTNAME, SYSLOG_APP_NAME, LOG_KERN);

bool alertOn = false;
bool alertStatus = false;
unsigned long last_time;

void setup() {
  pinMode(ALERT_PIN, OUTPUT);
  pinMode(WHITE_LED, OUTPUT);
  pinMode(ESP_LED, OUTPUT);
  pinMode(RED_BTN, INPUT);
  pinMode(BLACK_BTN, INPUT);
  
  digitalWrite(ALERT_PIN, HIGH);
  digitalWrite(WHITE_LED, HIGH);
  digitalWrite(ESP_LED, LOW);

  
  //EEPROM.begin(512);
  Serial.begin(115200);
  Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  WiFi.hostname(WIFI_HOSTNAME);
  ArduinoOTA.setHostname(WIFI_HOSTNAME);

  ArduinoOTA.setPassword(OTA_PASSWORD);

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";    
    } else { // U_SPIFFS
      type = "filesystem";
    }

    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  syslog.logf(LOG_INFO, "Device ready, ip address: %s", WiFi.localIP().toString().c_str());

  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(callback);

  Serial.setDebugOutput(0);
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  char *msg = new char[length + 1];
  memcpy(msg, payload, length);
  msg[length] = '\0';

  syslog.logf(LOG_INFO, "mqtt message arrived, topic: %s, message: %s", topic, msg);
  
  if (strcmp(topic, ALERT_TOPIC) == 0) {
    if(strcmp(msg, "ON") == 0) {
      alertOn = true;
    } else if(strcmp(msg, "OFF") == 0) {
      alertOn = false;
    }
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(WIFI_HOSTNAME)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      //client.publish(STATUS_TOPIC,"poweron");
      // ... and resubscribe
      client.subscribe(ALERT_TOPIC);
      //client.subscribe(BRIGHTNESS_TOPIC);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}




void loop() {
  ArduinoOTA.handle();

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  if(millis() > last_time + ALERT_INTERVAL) {
    if (alertOn && !alertStatus) {
      digitalWrite(ALERT_PIN, LOW);
      alertStatus = true;
    } else if(alertStatus) {
      digitalWrite(ALERT_PIN, HIGH);
      alertStatus = false;
    }

    last_time = millis();
  }
  

  if (digitalRead(RED_BTN) == 1) {
    syslog.logf(LOG_INFO, "RED BTN PRESSED");
    alertOn = false;
    delay(1000);
  }

  if (digitalRead(BLACK_BTN) == 1) {
    syslog.logf(LOG_INFO, "BLACK5 BTN PRESSED");
    delay(1000);
  }
  
  
  
}

