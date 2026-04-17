#include <Arduino.h>
#include <WiFi.h>

// --- CONFIGURAÇÃO DO WIFI ---
const char* ssid     = "Arthur_PalmasNet";
const char* password = "03957455944";
const int buzzerPin = 18; // Porta do buzzer para o alarme, na porta d2
const int buzzerChannel = 0;

// Porta do servidor (80 é o padrão para HTTP)
WiFiServer server(80);

const int ledPin = 2;

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

void stopAlarm() {
    ledcWriteTone(buzzerChannel, 0);
    noteIsPlaying = false;
    currentNoteIndex = 0;
    nextNoteChangeAt = 0;
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

void handleRequest(const String& requestLine) {
    if (requestLine.startsWith("GET /H ")) {
        digitalWrite(ledPin, HIGH);
        turnAlarmOn();
        return;
    }

    if (requestLine.startsWith("GET /L ")) {
        digitalWrite(ledPin, LOW);
        turnAlarmOff();
    }
}

void setup() {
    Serial.begin(115200);
    pinMode(ledPin, OUTPUT);
    ledcSetup(buzzerChannel, 2000, 8);
    ledcAttachPin(buzzerPin, buzzerChannel);
    digitalWrite(ledPin, LOW);
    stopAlarm();

    // Conectando ao WiFi
    Serial.print("Conectando a ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi conectado!");
    Serial.print("Endereço IP: ");
    Serial.println(WiFi.localIP()); // ESSA É A URL QUE VOCÊ VAI USAR NO NAVEGADOR

    server.begin();
}

void loop() {
    updateAlarm();

    WiFiClient client = server.available();

    if (client) {
        Serial.println("Novo cliente conectado.");
        String currentLine = "";
        String requestLine = "";
        bool isFirstLine = true;
        while (client.connected()) {
            if (client.available()) {
                char c = client.read();
                if (c == '\n') {
                    if (isFirstLine) {
                        requestLine = currentLine;
                        isFirstLine = false;
                        handleRequest(requestLine);
                    }

                    if (currentLine.length() == 0) {
                        if (requestLine.startsWith("GET /info ")) {
                            client.println("HTTP/1.1 200 OK");
                            client.println("Content-type:application/json");
                            client.println();
                            client.print("{\"mac\":\"");
                            client.print(WiFi.macAddress());
                            client.print("\"}");
                        } else {
                            // Cabeçalho HTTP padrão
                            client.println("HTTP/1.1 200 OK");
                            client.println("Content-type:text/html");
                            client.println();

                            // PÁGINA HTML
                            client.print("<h1>Controle do ESP32</h1>");
                            client.print("<p><a href=\"/H\"><button style='height:50px;width:100px;background:green;color:white'>LIGAR</button></a></p>");
                            client.print("<p><a href=\"/L\"><button style='height:50px;width:100px;background:red;color:white'>DESLIGAR</button></a></p>");
                        }
                        
                        client.println();
                        break;
                    } else {
                        currentLine = "";
                    }
                } else if (c != '\r') {
                    currentLine += c;
                }
            }

            updateAlarm();
        }
        client.stop();
        Serial.println("Cliente desconectado.");
    }
}