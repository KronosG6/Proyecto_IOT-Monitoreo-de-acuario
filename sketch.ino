#include <string>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <LiquidCrystal_I2C.h>

#define SENSOR_TEMP 2
#define BOMBA_AGUA 14
#define ALIMENTADOR 18
#define LED_R 32
#define LED_G 33
#define LED_B 25

#define DELAY_SENSOR 5000      // tiempo entre lecturas del sensor
#define DELAY_ALIMENTADOR 100 // tiempo de activacion del alimentador
#define MIN_TEMP 18
#define MAX_TEMP 30
#define DELTA_TEMP 2

static const char *root_ca PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
)EOF";

// comunicaciones
WiFiClientSecure wClient;
PubSubClient mqtt_client(wClient);

// alimentador
Servo servoMotor;

// sensor de temperatura
OneWire ourWire(SENSOR_TEMP);
DallasTemperature sensors(&ourWire);

// Display 16x2
LiquidCrystal_I2C lcd(0x27, 16, 2);

// estado de la bomba de agua
bool estado_bomba = false;

// WiFi
const String ssid = "redmi";
const String password = "12345678";

// MQTT
const String mqtt_server = "79beceafa2c24c47a7f6ab64b12d7e37.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;
const String mqtt_user = "admin";
const String mqtt_pass = "Admin123";

// ID y topics
const String ID_PLACA = "A1";
const String TOPIC_PUB = "/home/sensors";
const String TOPIC_SUB = "/home/actions";

//-----------------------------------------------------
//     Comunicaciones
//-----------------------------------------------------
void conecta_wifi()
{
    Serial.print("Conectando a " + ssid + " ");
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(100);
        Serial.print(".");
    }
    Serial.println(" Conectado!");
    Serial.println();
    Serial.println("WiFi conectado, IP: " + WiFi.localIP().toString());
}

//-----------------------------------------------------
void conecta_mqtt()
{
    while (!mqtt_client.connected())
    {
        Serial.print("Intentando conexion MQTT ...");

        if (mqtt_client.connect(ID_PLACA.c_str(), mqtt_user.c_str(), mqtt_pass.c_str()))
        {
            Serial.println(" conexion exitosa!");
            mqtt_client.subscribe(TOPIC_SUB.c_str(), 1); // QoS 1
        }
        else
        {
            Serial.println("ERROR:" + String(mqtt_client.state()) + " reintentando en 5s");
            delay(5000);
        }
    }
}

//-----------------------------------------------------
void procesa_mensaje(char *topic, byte *payload, unsigned int length)
{
    String mensaje = String(std::string((char *)payload, length).c_str());
    Serial.println("\nMensaje recibido [" + String(topic) + "]: " + mensaje);

    // Parsear el JSON
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, mensaje);

    if (error)
    {
        Serial.print("Error al parsear JSON: ");
        Serial.println(error.c_str());
        return;
    }

    // Acceder a los valores en el JSON
    const char *bomba_key = doc["wp"];
    const char *alimentador_key = doc["f"];
    const char *id_acuario_key = doc["aq"];

    // comprueba el topic
    if (String(topic) == TOPIC_SUB && strcmp(id_acuario_key, ID_PLACA.c_str()) == 0)
    {
        if (alimentador_key != nullptr)
        {
            if (strcmp(alimentador_key, "ON") == 0)
            {
                Serial.println("ALIMENTADOR ON");
                activar_alimentador();
            }
            else
            {
                Serial.println("ALIMENTADOR OFF");
            }
        }

        if (bomba_key != nullptr)
        {
            if (strcmp(bomba_key, "ON") == 0)
            {
                Serial.println("BOMBA ON");
                actualiza_estado_bomba(true);
            }
            else
            {
                Serial.println("BOMBA OFF");
                actualiza_estado_bomba(false);
            }
        }
    }
}

void publica_mensaje(String topic, String mensaje)
{
    Serial.println();
    Serial.println("Topic   : " + topic);
    Serial.println("Payload : " + mensaje);

    mqtt_client.publish(topic.c_str(), mensaje.c_str());
}

//-----------------------------------------------------
//     Controladores
//-----------------------------------------------------
void activar_alimentador()
{
    // Funcionamiento ServoMotor
    servoMotor.write(0);
    delay(DELAY_ALIMENTADOR);
    servoMotor.write(90);
    publica_mensaje(TOPIC_SUB, "{\"f\":\"OFF\",\"aq\":\"" + ID_PLACA + "\"}");
}

void control_rgb(bool r, bool g, bool b)
{
    digitalWrite(LED_R, r ? HIGH : LOW);
    digitalWrite(LED_G, g ? HIGH : LOW);
    digitalWrite(LED_B, b ? HIGH : LOW);
}

void actualiza_estado_bomba(bool nuevo_estado_bomba)
{
    digitalWrite(BOMBA_AGUA, nuevo_estado_bomba ? HIGH : LOW);
    if (estado_bomba != nuevo_estado_bomba)
    {
        estado_bomba = nuevo_estado_bomba;
        String bomba_on = estado_bomba ? "ON" : "OFF";
        publica_mensaje(TOPIC_SUB, "{\"wp\":\"" + bomba_on + "\",\"aq\":\"" + ID_PLACA + "\"}");
    }
}

//-----------------------------------------------------
//     SETUP
//-----------------------------------------------------
void setup()
{
    delay(1000);

    Serial.begin(115200);
    Serial.println();
    Serial.println("Empieza setup...");

    conecta_wifi();

    wClient.setCACert(root_ca);
    mqtt_client.setServer(mqtt_server.c_str(), mqtt_port);
    mqtt_client.setBufferSize(512); // para poder enviar mensajes de hasta X bytes
    mqtt_client.setCallback(procesa_mensaje);

    Serial.println("Broker MQTT: " + mqtt_server);
    conecta_mqtt();
    Serial.println("Topic publicacion  : " + TOPIC_PUB);
    Serial.println("Topic subscripcion : " + TOPIC_SUB);
    Serial.println("Termina setup en " + String(millis()) + " ms\n");

    // iniciar display
    lcd.init();
    lcd.backlight();

    // iniciar sensor temperatura
    sensors.begin();

    // iniciar led rgb
    pinMode(LED_R, OUTPUT);
    pinMode(LED_G, OUTPUT);
    pinMode(LED_B, OUTPUT);

    // iniciar bomba de agua
    pinMode(BOMBA_AGUA, OUTPUT);

    // iniciar servoMotor
    servoMotor.attach(ALIMENTADOR, 500, 2400);
}

//-----------------------------------------------------
unsigned long ultimo_mensaje = 0;
//-----------------------------------------------------
//     LOOP
//-----------------------------------------------------
void loop()
{
    if (!mqtt_client.connected())
        conecta_mqtt();

    mqtt_client.loop();

    unsigned long ahora = millis();

    if (ahora - ultimo_mensaje >= DELAY_SENSOR || ultimo_mensaje == 0)
    {
        ultimo_mensaje = ahora;

        // Funcionamiento Sensor Temperatura
        sensors.requestTemperatures();
        float temp = sensors.getTempCByIndex(0);
        String state;

        //limpiar lcd
        lcd.clear();

        // Display Primera Fila
        lcd.setCursor(0, 0);
        if (temp < MIN_TEMP)
        {
            state = "BAD";
            lcd.print("TEMP. BAJA!");
            control_rgb(true, false, false);
            actualiza_estado_bomba(false);
        }
        else if (temp < MIN_TEMP + DELTA_TEMP)
        {
            state = "WARN";
            lcd.print("TEMP. ESTABLE-");
            control_rgb(false, false, true);
            actualiza_estado_bomba(false);
        }
        else if (temp < MAX_TEMP - DELTA_TEMP)
        {
            state = "OK";
            lcd.print("TEMP. ESTABLE");
            control_rgb(false, true, false);
            actualiza_estado_bomba(false);
        }
        else if (temp < MAX_TEMP)
        {
            state = "WARN";
            lcd.print("TEMP. ESTABLE+");
            control_rgb(false, false, true);
            actualiza_estado_bomba(true);
        }
        else
        {
            state = "BAD";
            lcd.print("TEMP. ALTA!");
            control_rgb(true, false, false);
            actualiza_estado_bomba(true);
        }

        // Display Segunda Fila
        lcd.setCursor(0, 1);
        lcd.print("Temp:");
        lcd.print(temp);
        lcd.print(" C");

        // publica mensaje al broker mqtt
        String mensaje_json = "{\"aq\":\"" + ID_PLACA + "\",\"t\":" + String(temp) + ",\"st\":\"" + state + "\"}";
        publica_mensaje(TOPIC_PUB, mensaje_json);

        Serial.print("Temperatura = ");
        Serial.print(temp);
        Serial.println(" °C");
    }
}
