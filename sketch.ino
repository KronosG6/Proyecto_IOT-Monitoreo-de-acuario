#include <WiFiEsp.h>
#include <WiFiEspClient.h>
#include <PubSubClient.h>
#include "SoftwareSerial.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>

// Conexión a la red wifi: nombre de la red y contraseña
#define WIFI_AP "samaelMTA"
#define WIFI_PASSWORD "fabrixxio2010"

// Nombre o IP del servidor mosquitto
char server[50] = "192.168.119.250";

// Inicializamos el objeto de cliente esp
WiFiEspClient espClient;

// Iniciamos el objeto subscriptor del cliente con el objeto del cliente
PubSubClient client(espClient);

// Conexión serial para el esp con una comunicación serial, pines 2: rx y 3: tx
SoftwareSerial soft(2, 3);

// Contador para el envio de datos
unsigned long lastSend;

// Definición de pines y variables para el sensor de temperatura, LEDs, bomba y servo motor
#define ledR 9
#define ledG 10
#define ledB 11
#define bombaAgua 13
#define sensorTemp 8
#define alimentador 5

// Sensor de temperatura
OneWire ourWire(sensorTemp);
DallasTemperature sensors(&ourWire);

// Display 1602
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ServoMotor
Servo servoMotor;

int status = WL_IDLE_STATUS;

void setup() {
    // Inicializamos la comunicación serial para el log
    Serial.begin(9600);
    
    // Iniciar sensor de temperatura
    sensors.begin();
    
    // Iniciar display
    lcd.init();
    lcd.backlight();

    // Iniciar LED RGB
    pinMode(ledR, OUTPUT);
    pinMode(ledG, OUTPUT);
    pinMode(ledB, OUTPUT);

    // Iniciar bomba de agua
    pinMode(bombaAgua, OUTPUT);

    // Iniciar servo motor
    servoMotor.attach(alimentador);

    // Iniciamos la conexión a la red WiFi
    InitWiFi();
    
    // Colocamos la referencia del servidor y el puerto
    client.setServer(server, 1883);
    lastSend = 0;
}

void loop() {
    // Validamos si el modulo WiFi aun está conectado a la red
    status = WiFi.status();
    if(status != WL_CONNECTED) {
        // Si falla la conexión, reconectamos el modulo
        reconnectWifi();
    }

    // Validamos si está la conexión del servidor
    if(!client.connected()) {
        // Si falla reintentamos la conexión
        reconnectClient();
    }

    // Creamos un contador para enviar la data cada 2 segundos
    if(millis() - lastSend > 2000) {
        sendDataTopic();
        lastSend = millis();
    }

    // Funcionamiento ServoMotor
    servoMotor.write(0);
    delay(1000);
    servoMotor.write(90);

    // Funcionamiento Sensor Temperatura
    sensors.requestTemperatures();
    float temp = sensors.getTempCByIndex(0);

    Serial.print("Temperatura = ");
    Serial.print(temp);
    Serial.println(" °C");

    // Funcionamiento LED y bomba de agua
    if (temp == 0) {
        lcd.setCursor(0, 0);
        lcd.print("SE MURIO TU PEZ");
        digitalWrite(ledR, HIGH);
        digitalWrite(ledG, LOW);
    } else if (temp < 20) {
        lcd.setCursor(0, 0);
        lcd.print("TEMP. BAJA!");
        digitalWrite(ledR, HIGH);
        digitalWrite(ledG, HIGH);
    } else if (temp < 30) {
        lcd.setCursor(0, 0);
        lcd.print("TEMP. ESTABLE");
        digitalWrite(bombaAgua, LOW);
        digitalWrite(ledR, LOW);
        digitalWrite(ledG, HIGH);
    } else {
        lcd.setCursor(0, 0);
        lcd.print("TEMP. ALTA!");
        digitalWrite(bombaAgua, HIGH);
        digitalWrite(ledR, HIGH);
        digitalWrite(ledG, HIGH);
    }

    // Display Segunda Fila
    lcd.setCursor(0, 1);
    lcd.print("Temp:");
    lcd.print(temp);
    lcd.print(" C");

    delay(500);

    client.loop();
}

void sendDataTopic() {
    Serial.println("sendDataTopic");
    
    // Preparamos una cadena de payload JSON
    String payload = "Hola desde arduino";
    payload += ", Temperatura: ";
    sensors.requestTemperatures();
    float temp = sensors.getTempCByIndex(0);
    payload += String(temp) + " C";

    // Enviamos el payload
    char attributes[100];
    payload.toCharArray(attributes, 100);
    client.publish("outTopic", attributes);
    Serial.println(attributes);
}

// Inicializamos la conexión a la red wifi
void InitWiFi() {
    // Inicializamos el puerto serial
    soft.begin(9600);
    // Iniciamos la conexión wifi
    WiFi.init(&soft);
    // Verificamos si se pudo realizar la conexión al wifi
    // si obtenemos un error, lo mostramos por log y detenemos el programa
    if (WiFi.status() == WL_NO_SHIELD) {
        Serial.println("El modulo WiFi no esta presente");
        while (true);
    }
    reconnectWifi();
}

void reconnectWifi() {
    Serial.println("Iniciar conexión a la red WIFI");
    while(status != WL_CONNECTED) {
        Serial.print("Intentando conectarse a WPA SSID: ");
        Serial.println(WIFI_AP);
        // Conectar a red WPA/WPA2
        status = WiFi.begin(WIFI_AP, WIFI_PASSWORD);
        delay(500);
    }
    Serial.println("Conectado a la red WIFI");
}

void reconnectClient() {
    // Creamos un loop en donde intentamos hacer la conexión
    while(!client.connected()) {
        Serial.print("Conectando a: ");
        Serial.println(server);
        // Creamos una nueva cadena de conexión para el servidor
        // e intentamos realizar la conexión nueva
        // si requiere usuario y contraseña la enviamos connect(clientId, username, password)
        String clientId = "ESP8266Client-" + String(random(0xffff), HEX);
        if(client.connect(clientId.c_str())) {
            Serial.println("[DONE]");
        } else {
            Serial.print("[FAILED] [ rc = ");
            Serial.print(client.state());
            Serial.println(" : retrying in 5 seconds]");
            delay(5000);
        }
    }
}
