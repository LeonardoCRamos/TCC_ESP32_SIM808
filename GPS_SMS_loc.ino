/*

  ### GPS SMS loc
  Funcionalidades:
  1. Estabelecer comunicacao serial entre ESP32 e SIM808
  2. Realizar leitura da localizacao utilizando o GPS do SIM808
  3. Enviar uma mensagem de perigo para o paciente
  4. Ler o SMS de resposta do numero
  5. Acionar um contato de emergencia a um numero pre-cadastrado

*/

#include "SIM808_methods.h"
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <math.h>

#define SERVICE_UUID        "a49e4056-1260-4b64-a5a9-8b0ae5ba5126"
#define CHARACTERISTIC_UUID "42fcbe5f-ecb8-4577-a739-6ea6aa54615e"

//Definicao de pi
#define PI 3.14159265359

//Variaveis auxiliares
char MESSAGE[300]; //mensagem de SMS que sera enviada
char lat[12]; //latitude
char lon[12]; //longitude

float del_lat; //diferenca medida na latitude em radianos
float del_lon; //diferenca medida na latitude em radianos
float avg_lat; //latitude media em radianos
double dist; //distancia do usuario para sua casa

int state; //estado do usuario
int sms; //indentificador do SMS recebido

bool send_help = false;
bool help_sent = false;

unsigned long time_begin;
unsigned long time_now;
unsigned long time_help;

//Comunicacao serial com o SIM808
//PIN RX 16
//PIN TX 17
HardwareSerial Serial2(2);

//Inicia classe utilizando a biblioteca criada SIM808_methods
SIM808_methods class_sim808(&Serial2);

void setup() {
  Serial.begin(9600);
  Serial2.begin(9600);

  //Iniciando comunicacao com o SIM808
  while(!(class_sim808.init())) {
    delay(1000);
    Serial.print("\r\nProblema na comunicacao\r\n");
  }
  Serial.println("Comunicacao estabelecida!");

  //Verificando se existe problema com o cartao SIM
  while(!(class_sim808.SIM_status())) {
    delay(1000);
    Serial.println("Problema com cartao SIM\r\n");
  }
  Serial.println("SIM conectado!");
  
  //Ligando GPS
  while(!(class_sim808.turnon_GPS())) {
    delay(1000);
    Serial.print("Problema ao ligar GPS...\r\n");
  }
  Serial.println("GPS ligado com sucesso!");

  //Deleta todos os SMS (seguranca)
  while(!(class_sim808.deleteall_SMS())) {
    delay(1000);
    Serial.println("Problema ao deletar as mensagens\r\n");
  }
  Serial.println("Pronto para receber mensagens!");

  //Inicia comunicacao Bluetooth (BT)
  BLEDevice::init("MyESP32"); //nome do BT para pareamento
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_READ
                                       );
  pCharacteristic->setValue("11987345829"); //envia numero do SIM do ESP32
  pService->start();
  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->start();
  Serial.println("Bluetooth ligado!");

  //Verifica se recebeu uma resposta
  sms = class_sim808.new_SMS();
  if(!sms)
    Serial.print("\r\nEsperando SMS");
  //Enquanto nao recebe um SMS de resposta e esperou menos que 10s
  while(!sms) {
    Serial.print(".");
    delay(2000); //espera 2s
    sms = class_sim808.new_SMS(); //verifica se recebeu algo
  }
  
  while(!class_sim808.read_SMS_data(sms,CHARACTERISTIC_UUID,36)) {
    Serial.println("SMS com formatacao errada!");
    Serial.println("Espera-se um SMS no formato:");
    Serial.println("UUID,numero do contato de emergencia, latitude, longitude");
    //Verifica se recebeu uma resposta
    sms = class_sim808.new_SMS();
    if(!sms)
      Serial.print("\r\nAinda esperando SMS");
    //Enquanto nao recebe um SMS de resposta e esperou menos que 10s
    while(!sms) {
      Serial.print(".");
      delay(2000); //espera 2s
      sms = class_sim808.new_SMS(); //verifica se recebeu algo
    }
  }

  class_sim808.USERdata.safe_radius = 50.0; //raio de seguranca

  Serial.println();
  Serial.print("Numero do usuario = ");
  Serial.println(class_sim808.USERdata.user_phone);    
  Serial.print("Numero de ajuda = ");
  Serial.println(class_sim808.USERdata.help_phone);  
  Serial.print("Latitude = ");
  Serial.println(class_sim808.USERdata.lat_home);
  Serial.print("Longitude = ");
  Serial.println(class_sim808.USERdata.lon_home);
}

void loop() {
  /*if (Serial2.available()) {
    Serial.write(Serial2.read());
  }
  if (Serial.available()) {
    while(Serial.available()) {
      Serial2.write(Serial.read());
    }
  }*/

  delay(5000); //faz uma leitura do GPS a cada 5s
  
  //Se existe leitura no GPS do SIM808
  if(class_sim808.status_GPS()) {

    //Realiza a leitura do GPS e salva em 'GPSdata'
    class_sim808.read_GPS();
    Serial.print("Latitude GPS = ");
    Serial.println(class_sim808.GPSdata.lat);
    Serial.print("Longitude GPS = ");
    Serial.println(class_sim808.GPSdata.lon);

    //Verifica a distancia do usuario para sua casa usando a formula
    //distancia = sqrt((K1*del_lat)^2 + (K2*del_lon)^2); onde
    del_lat = (class_sim808.USERdata.lat_home - class_sim808.GPSdata.lat);
    del_lon = (class_sim808.USERdata.lon_home - class_sim808.GPSdata.lon);
    //De graus pra radianos
    //avg_lat = (1000/57296)*((lat_home + lat_GPS)/2) = (1000/(2*57296))*(lat_home + lat_GPS)
    avg_lat = (class_sim808.USERdata.lat_home + class_sim808.GPSdata.lat);
    K1 = 111.13209 - 0.56605*cos(2*avg_lat) + 0.0012*cos(4*avg_lat);
    K2 = 111.41513*cos(avg_lat) - 0.09455*cos(3*avg_lat) + 0.00012*cos(5*avg_lat);
    dist = sqrt(sq(K1*del_lat) + sq(K2*del_lon));

    Serial.print("dist = ");
    Serial.println(dist);
    
    //Se o usuario saiu do raio de seguranca
    if(dist > class_sim808.USERdata.safe_radius) {
      Serial.println("Voce esta longe de casa!");
      Serial.println("Enviando SMS para o idoso...");
      //Envia mensagem  SMS para verificar se ele esta bem
      while(!class_sim808.send_SMS(class_sim808.USERdata.user_phone,"ALERTA"))
        delay(1000);

      Serial.println("Esperando uma resposta");
      //Verifica se recebeu uma resposta
      sms = class_sim808.new_SMS();

      time_begin = millis();
      time_now = millis();

      //Enquanto nao recebe um SMS de resposta e esperou menos que 10s
      while((!sms) && (time_now - time_begin < 10000)) {
        Serial.print(".");
        delay(1000); //espera 1s
        sms = class_sim808.new_SMS(); //verifica se recebeu algo
        time_now = millis();
      }

      //se nao obteve resposta em 10s
      if(time_now - time_begin >= 10000) {
        if(time_now - time_help > 300000)
          help_sent = false;
        if(!help_sent) {
          time_help = millis();
          send_help = true;
        }
      }

      //se recebeu uma resposta
      else {
        //verifica se e' do usuario e qual resposta
        switch(class_sim808.read_SMS_safe(sms,class_sim808.USERdata.user_phone_size,class_sim808.USERdata.user_phone)) {
          case 0: //nao e' do paciente
            break;
          case 1: //usuario em seguranca
            break;
          case 2: //mensagem nao reconhecida
            break;
        }
      }
    } //fim do 'if fora da zona de seguranca'
    
    //Desligando GPS
    /*delay(500);
    while(!(class_sim808.turnoff_GPS())) {
      delay(1000);
      Serial.print("Problema ao desligar GPS...\r\n");
    }
    Serial.println("GPS desligado com sucesso!");*/
  }

  if(send_help) {

    time_help = millis();

    //Salva o  valor em float da latitude/longitude na string lat/lon
    //4 = casas decimais antes da virgula
    //7 = casas decimais depois da virgula
    dtostrf(class_sim808.GPSdata.lat, 4, 7, lat);
    dtostrf(class_sim808.GPSdata.lon, 4, 7, lon);

    sprintf(MESSAGE, "Socorro! Estou em: http://maps.google.com/maps?q=%s,%s\n", lat, lon);
    
    class_sim808.send_SMS(class_sim808.USERdata.help_phone,MESSAGE); //envia mensagem de socorro

    help_sent = true;
    send_help = false;
  }
  
}
