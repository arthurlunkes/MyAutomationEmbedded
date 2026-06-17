#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <ESPmDNS.h>
#include <math.h>

Preferences prefs;
WebServer server(80);

const int ledPin = 2;
const int buzzerPin = 18;
const int buzzerChannel = 0;
const int thermistorPin = 36; // Pino ADC para o termistor
const int buttonPin = 0; // Botão BOOT (Flash) do ESP32

const int imperialMarchNotes[] = {
    440, 440, 440, 349, 523, 440, 349, 523, 440,
    659, 659, 659, 698, 523, 415, 349, 523, 440,
    880, 440, 440, 880, 830, 784, 740, 698, 740,
    455, 622, 587, 554, 523, 466, 523
};

const int imperialMarchDurations[] = {
    500, 500, 500, 350, 150, 500, 350, 150, 650,
    500, 500, 500, 350, 150, 500, 350, 150, 650,
    500, 350, 150, 500, 325, 175, 125, 125, 250,
    250, 500, 325, 175, 125, 125, 250
};

const int imperialMarchLength = sizeof(imperialMarchNotes) / sizeof(imperialMarchNotes[0]);
const int notePauseMs = 50;

bool alarmEnabled = false;
bool noteIsPlaying = false;
int currentNoteIndex = 0;
unsigned long nextNoteChangeAt = 0;

bool isAPMode = false;
unsigned long buttonPressTime = 0;
bool buttonResetTriggered = false;

// --- FUNÇÕES DO ALARME ---
void stopAlarm() {
    ledcWriteTone(buzzerChannel, 0);
    noteIsPlaying = false;
    currentNoteIndex = 0;
    nextNoteChangeAt = 0;
}

void turnAlarmOn() {
    alarmEnabled = true;
    noteIsPlaying = false;
    currentNoteIndex = 0;
    nextNoteChangeAt = 0;
}

void turnAlarmOff() {
    alarmEnabled = false;
    stopAlarm();
}

void updateAlarm() {
    if (!alarmEnabled) {
        return;
    }

    unsigned long now = millis();

    if (nextNoteChangeAt != 0 && now < nextNoteChangeAt) {
        return;
    }

    if (noteIsPlaying) {
        ledcWriteTone(buzzerChannel, 0);
        noteIsPlaying = false;
        nextNoteChangeAt = now + notePauseMs;
        currentNoteIndex++;

        if (currentNoteIndex >= imperialMarchLength) {
            currentNoteIndex = 0;
        }
        return;
    }

    ledcWriteTone(buzzerChannel, imperialMarchNotes[currentNoteIndex]);
    noteIsPlaying = true;
    nextNoteChangeAt = now + imperialMarchDurations[currentNoteIndex];
}

// --- ROTAS DO WEBSERVER ---

void handleRoot() {
    String html = "<html><body>";
    html += "<h1>Controle do ESP32</h1>";
    html += "<p><a href='/H'><button style='height:50px;width:100px;background:green;color:white;border:none;border-radius:5px;'>LIGAR</button></a></p>";
    html += "<p><a href='/L'><button style='height:50px;width:100px;background:red;color:white;border:none;border-radius:5px;'>DESLIGAR</button></a></p>";
    html += "<p><a href='/reset'><button style='height:50px;width:150px;background:gray;color:white;border:none;border-radius:5px;margin-top:20px'>RESETAR WIFI</button></a></p>";
    html += "</body></html>";
    server.send(200, "text/html", html);
}

void handleStatus() {
    prefs.begin("wifi", true);
    String ssid = prefs.getString("ssid", "");
    prefs.end();

    JsonDocument doc;
    doc["mac"] = WiFi.macAddress();
    doc["configured"] = !ssid.isEmpty();

    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

void handleWifiConfig() {
    if (!server.hasArg("plain")) {
        server.send(400, "application/json", "{\"error\":\"Corpo vazio\"}");
        return;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, server.arg("plain"));

    if (error) {
        server.send(400, "application/json", "{\"error\":\"JSON invalido\"}");
        return;
    }

    String ssid = doc["ssid"] | "";
    String password = doc["password"] | "";

    if (ssid.isEmpty()) {
        server.send(400, "application/json", "{\"error\":\"SSID vazio\"}");
        return;
    }

    prefs.begin("wifi", false);
    prefs.putString("ssid", ssid);
    prefs.putString("password", password);
    prefs.end();

    server.send(200, "application/json", "{\"success\":true}");
    
    delay(1000);
    ESP.restart();
}

void handleInfo() {
    JsonDocument doc;
    doc["mac"] = WiFi.macAddress();
    doc["ip"] = isAPMode ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
    doc["hostname"] = "alarme.local";
    doc["connected"] = !isAPMode && (WiFi.status() == WL_CONNECTED);

    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

void handleAlarmOn() {
    digitalWrite(ledPin, HIGH);
    turnAlarmOn();
    server.send(200, "application/json", "{\"success\":true}");
}

void handleAlarmOff() {
    digitalWrite(ledPin, LOW);
    turnAlarmOff();
    server.send(200, "application/json", "{\"success\":true}");
}

float readTemperature() {
    int adcValue = analogRead(thermistorPin);

    if (adcValue <= 0 || adcValue >= 4095) {
        return -999;
    }

    double voltage = ((double)adcValue / 4095.0) * 3.3;

    double Rt = 10.0 * voltage / (3.3 - voltage);

    double tempK =
        1.0 /
        (
            (1.0 / (273.15 + 25.0))
            +
            (log(Rt / 10.0) / 3950.0)
        );

    return tempK - 273.15;
}

void handleTemperature() {
    JsonDocument doc;

    doc["temperature"] = readTemperature();
    doc["unit"] = "C";

    String response;
    serializeJson(doc, response);

    server.send(
        200,
        "application/json",
        response
    );
}

void handleReset() {
    prefs.begin("wifi", false);
    prefs.clear(); // Limpa todas as chaves
    prefs.end();

    server.send(200, "application/json", "{\"success\":true}");
    delay(1000);
    ESP.restart(); // Reinicia o ESP32
}

// --- SETUP E LOOP ---
void setupServer() {
    server.on("/", HTTP_GET, handleRoot);
    server.on("/status", HTTP_GET, handleStatus);
    server.on("/wifi", HTTP_POST, handleWifiConfig);
    server.on("/temperature", HTTP_GET, handleTemperature);
    server.on("/info", HTTP_GET, handleInfo);
    server.on("/reset", HTTP_GET, handleReset);
    server.on("/H", HTTP_GET, handleAlarmOn);
    server.on("/L", HTTP_GET, handleAlarmOff);
    server.begin();
    Serial.println("Servidor Web iniciado.");
}

void startAP() {
    isAPMode = true;
    WiFi.mode(WIFI_AP);
    WiFi.softAP("ESP32-SETUP-ARTHUR", "12345678");
    Serial.println("Modo AP iniciado.");
    Serial.print("IP do AP: ");
    Serial.println(WiFi.softAPIP());
}

bool connectToWifi() {
    prefs.begin("wifi", true);
    String ssid = prefs.getString("ssid", "");
    String password = prefs.getString("password", "");
    prefs.end();

    if (ssid.isEmpty()) {
        Serial.println("WiFi nao configurado.");
        return false;
    }

    Serial.print("Conectando ao WiFi: ");
    Serial.println(ssid);
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
        delay(500);
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nConectado com sucesso!");
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());
        return true;
    }

    Serial.println("\nFalha ao conectar.");
    return false;
}

void setupMDNS() {
    if (MDNS.begin("alarme")) {
        Serial.println("mDNS iniciado. Endereço: http://alarme.local");
        MDNS.addService("http", "tcp", 80);
    } else {
        Serial.println("Erro ao iniciar mDNS.");
    }
}

void setup() {
    Serial.begin(115200);

    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, LOW);

    pinMode(buttonPin, INPUT_PULLUP);

    ledcSetup(buzzerChannel, 2000, 8);
    ledcAttachPin(buzzerPin, buzzerChannel);
    stopAlarm();

    if (!connectToWifi()) {
        startAP();
    } else {
        isAPMode = false;
    }

    setupMDNS();
    setupServer();
}

void updateButton() {
    if (digitalRead(buttonPin) == LOW) { // Botão pressionado
        if (buttonPressTime == 0) {
            buttonPressTime = millis();
        } else if (millis() - buttonPressTime > 3000 && !buttonResetTriggered) {
            buttonResetTriggered = true;
            Serial.println("Reset via botao acionado. Limpando WiFi...");
            prefs.begin("wifi", false);
            prefs.clear();
            prefs.end();
            
            digitalWrite(ledPin, HIGH); // Acende o LED para indicar reset
            delay(1000);
            ESP.restart();
        }
    } else {
        buttonPressTime = 0;
    }
}

void loop() {
    updateAlarm();
    updateButton();
    server.handleClient();
}