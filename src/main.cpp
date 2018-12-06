#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// Definição de pinos
#define BUTTON_PIN 5
#define OPEN_PIN 14
#define LED_PIN 12

// definição de parâmetros
#define T_DESTRAVADO 60000L
#define T_ABERTO 1000
#define BUTTON_PRESS 500

// classe para monitorar o botão de abertura
class InterruptMonitor
{
	// Class Member Variables

  // variáveis internas
  volatile bool flag;   // flag que avisa quando há mudança de estado
  bool do_read;         // flag que marca o agendamento de um leitura atrasada
  bool last_read;       // estado da última leitura
  unsigned long last_schedule;  // último agendamento da leitura

  public:
  // variáveis de configuração
	byte pin;             // numero do pino de entrada
  unsigned long delay;   // atraso em ms entre o agendamento e a leitura

  // Constructor - cria o monitor
  // and initializes the member variables and state
  InterruptMonitor(byte input_pin, unsigned long msdelay)
  {
	pin = input_pin;
	pinMode(pin, INPUT_PULLUP);

  delay = msdelay;
  flag = do_read = false;
  last_read = digitalRead(pin);
  last_schedule = 0;
  }

  // função que levanta a flag de mudança de estado
  // deve ser chama externamente, a flag gera um agendamendo no proximo update
  void raiseFlag() {
    flag = true;
  }

  int update() {
    // variável que guarda resposta
    // -1: não houve mudanças, 0: pino mudou para low, 1: pino mudou para high
    int resp = -1;

    // agenda leitura atrasada:
    if (flag) {
      last_schedule = millis();
      flag = false;
      do_read = true;
    }
    // executa leitura atrasada
    if (do_read && millis() - last_schedule > delay) {
      do_read = false;
      bool read = digitalRead(pin);

      if (read != last_read) resp = last_read = read;
    }
    return resp;
  }
};


//---------------------------------------------//
//            VARIÁVEIS GLOBAIS
//---------------------------------------------//
// variáveis do controle da porta
InterruptMonitor button(BUTTON_PIN, BUTTON_PRESS);
unsigned long t_destravado, t_aberto;
bool destravado, aberto;


// variáveis do wifi
const char* ssid = "????";
const char* pass = "????";
const unsigned long wifi_rcinterval = 1000;
unsigned long wifi_lastrc;
WiFiClient wclient;
char msg[50];

//variável do cliente MQTT
// const char* mqtt_server = "broker.mqttdashboard.com";
IPAddress mqtt_server(192,168,1,75);
const char* mqtt_inTopic = "testario/fechadura";   // nome do tópico de publicação
const char* mqtt_outTopic = "testario/server";  // nome do tópico de inscrição
const unsigned long mqtt_rcinterval = 3000;
unsigned long mqtt_lastrc;
PubSubClient mqtt_client(wclient);


void destravar_porta() {
  t_destravado = millis();
  destravado = true;
  digitalWrite(LED_PIN, HIGH);
  Serial.println("Porta destravada");
  mqtt_client.publish(mqtt_outTopic, "Porta destravada");
}

void travar_porta() {
  destravado = false;
  digitalWrite(LED_PIN, LOW);
  Serial.println("Porta travada");
  mqtt_client.publish(mqtt_outTopic, "Porta travada");
}

void abre_porta() {
  aberto = true;
  digitalWrite(OPEN_PIN, HIGH);
  travar_porta();
  Serial.println("Porta aberta");
  mqtt_client.publish(mqtt_outTopic, "Porta aberta");
}

void button_set(){
  button.raiseFlag();
}

void reconnectWifi() {
  Serial.print("Conectado-se a rede ");
  Serial.print(ssid);
  Serial.println("...");
  WiFi.begin(ssid, pass);

if (WiFi.waitForConnectResult() != WL_CONNECTED)
  return;
Serial.println("WiFi connected");
}

bool reconnectMQTT() {
  Serial.println("Connecting to the MQTT broker...");
  // if (mqtt_client.connect(mqtt_clientname)) {
    if (mqtt_client.connect("client_kjhadu4")) {
    // Once connected, publish an announcement...
    mqtt_client.publish(mqtt_outTopic, "hello world", true);
    // ... and resubscribe
    mqtt_client.subscribe(mqtt_inTopic);
  }
  return mqtt_client.connected();
}

// callback que lida com as mensagem recebidas
void mqtt_callback(char* topic, byte* payload, unsigned int length) {

    // Copy the payload to the new char buffer
    char* msg = (char*)malloc(length + 1);
    memcpy(msg, (char*)payload, length);
    msg[length] = '\0';

    // // prepara a msg de resposta
    // char resp[30];
    // strncpy(resp, msg, sizeof(resp));
    // resp[sizeof(resp) -1] = '\0';

    Serial.println(msg);
    char* command = strtok(msg, "=");  // horário de envio da msg
    char* temp = strtok(NULL, "="); // comando
    Serial.print("Comando: "); Serial.println(command);
    Serial.print("temp: "); Serial.println(temp);

    if(strcmp(command, "liberar") == 0) {
      destravar_porta();
    }
}

//---------------------------------------------//
//                  SETUP
//---------------------------------------------//
void setup()  {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  //configuração dos pinos
  pinMode(OPEN_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  // digitalWrite(OPEN_PIN, HIGH);
  // digitalWrite(LED_PIN, HIGH);

  // configura interruptor dos pinos de entrada
  attachInterrupt(digitalPinToInterrupt(button.pin), button_set, LOW);

  // configuracao do wifi
  WiFi.mode(WIFI_STA);

  // configuração do MQTT
  mqtt_client.setServer(mqtt_server, 1883);
  mqtt_client.setCallback(mqtt_callback);
}

//---------------------------------------------//
//                  LOOP
//---------------------------------------------//
void loop() {
  // Controle da porta
  // Se a porta estiver liberada para abertura...
  if (destravado) {
  // abre a porta se o botão for pressionado
    // if (button.update() == LOW) {
    if (digitalRead(BUTTON_PIN) == LOW) {
      abre_porta();
      t_aberto = millis();
    }

    // retira a liberação da porta depois de 'T_LIBERADO' ms
    if (millis() - t_destravado > T_DESTRAVADO) {
      travar_porta();
    }
  }
  if (aberto && millis() - t_aberto > T_ABERTO) {
    digitalWrite(OPEN_PIN, LOW);
    aberto = false;
  }

  // reconecta ao wifi
  if (WiFi.status() != WL_CONNECTED) {
    unsigned long now = millis();
    if(now - wifi_lastrc > wifi_rcinterval) {
      reconnectWifi();
      wifi_lastrc = now;
    }
  }

  // reconecta ao servido MQTT
  if (WiFi.status() == WL_CONNECTED && !mqtt_client.connected()) {
    unsigned long now = millis();
    if (now - mqtt_lastrc > mqtt_rcinterval) {
      reconnectMQTT();
      mqtt_lastrc = now;
    }
  }

  // executa loop do MQTT
  if (mqtt_client.connected()) mqtt_client.loop();
}
