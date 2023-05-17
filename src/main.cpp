#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>
#include <SPIFFS.h>
#include <Servo.h>
#include <WiFi.h>
#define FORMAT_SPIFFS_IF_FAILED true

#define SERVO_PIN 12
#define BUTTON_PIN 14

#define ARM_DOWN 30
#define ARM_UP 120

#define HTTP_PORT 80

#define TIMEMAX 2147483640

AsyncWebServer server(HTTP_PORT);

Servo myservo;

long duration = 0;
long time_counter = 0;
bool auto_flag = false;
bool ring_flag = false;

const char *wifi_ssid = "sample";
const char *wifi_password = "sample";

hw_timer_t *timer = NULL;

void IRAM_ATTR onTimer() {
	if (time_counter > TIMEMAX + 2) {
		time_counter = 0;
	}
	time_counter++;
	if (auto_flag) {
		if (time_counter >= duration) {
			ring_flag = true;
			time_counter = 0;
		}
	}
}

void wifi_connect(const char *ssid, const char *password) {
	Serial.println("");
	Serial.print("WiFi Connenting");

	WiFi.begin(ssid, password);
	while (WiFi.status() != WL_CONNECTED) {
		Serial.print(".");
		delay(1000);
	}

	Serial.println("");
	Serial.print("Connected : ");
	Serial.println(WiFi.localIP());
}

void ring() {
	myservo.write(ARM_DOWN);
	delay(100);
	myservo.write(ARM_UP);
	delay(100);
	myservo.write(ARM_DOWN);
}

void setting(long d) {
	if (d < 0) {
		time_counter = 0;
		auto_flag = false;
	} else if (d <= TIMEMAX) {
		time_counter = 0;
		duration = d;
		auto_flag = true;
	}
}

void notFound(AsyncWebServerRequest *request) {
	if (request->method() == HTTP_OPTIONS) {
		request->send(200);
	} else {
		request->send(404);
	}
}

void setup() {
	// put your setup code here, to run once:
	pinMode(BUTTON_PIN, INPUT);

	timer = timerBegin(0, 80, true);
	timerAttachInterrupt(timer, &onTimer, true);
	timerAlarmWrite(timer, 1000000, true);
	timerAlarmEnable(timer);

	Serial.begin(115200);
	myservo.attach(SERVO_PIN, -1, 0, 180, 544, 2400);
	wifi_connect(wifi_ssid, wifi_password);
	SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED);

	server.on("/ring", HTTP_POST, [](AsyncWebServerRequest *request) {
		ring();
		request->send(SPIFFS, "/www/index.html");
	});

	server.on("/setting", HTTP_POST, [](AsyncWebServerRequest *request) {
		ring();
		if (request->hasArg("duration")) {
			setting((long)atol(request->arg("duration").c_str()));
		}
		request->send(SPIFFS, "/www/index.html");
	});

	server.serveStatic("/", SPIFFS, "/www/").setDefaultFile("index.html");
	server.onNotFound(notFound);

	DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
	DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "*");
	server.begin();
}

void loop() {
	// put your main code here, to run repeatedly:
	// ring();
	if (ring_flag) {
		ring();
		ring();
		ring_flag = false;
	}
	if (digitalRead(BUTTON_PIN) == HIGH) {
		ring();
	}
}
