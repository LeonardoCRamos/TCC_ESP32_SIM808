#include "SIM808_methods.h"

/*************
 * Variaveis *
 *************/

//Ponteiro para o serial sendo utilizado
HardwareSerial *serialGPS = NULL;

//Array para salvar resposta do SIM808
char serial_data[100]; //array
int pos = 0; //posicao do ultimo caracter lido

/***********
 * Metodos *
 ***********/

/*
 * SIM808_methods(HardwareSerial *mySerial)
 * Construtor da classe 'SIM808_methods', identifica 
 * o ponteiro *serialGPS para o HardwareSerial que esta
 * sendo utilizado pelo arquivo principal
 */
SIM808_methods::SIM808_methods(HardwareSerial *mySerial){
  serialGPS = mySerial;
}

/*
 * bool init( )
 * Realiza setup inicial, verificando se a comunicacao esta OK 
 * e definindo as funcoes do telefone para maxima
 */
bool SIM808_methods::init(void) {
  delay(500);
  
  //Envia comando de verificacao AT
  serialGPS->write("AT\r\n");
  delay(500);

  //Aguarda retorno de resposta 'OK'
  if(!strcmp(read_buffer("AT", 2), "OK") == 0)
    return false; //caso contrario, sai da funcao

  //Envia comando para definir as funcionalidades do celular
  serialGPS->write("AT+CFUN=1\r\n"); //Phone Functionality (1 = Full Functionality)
  delay(500);

  //Aguarda retorno de resposta 'OK'
  if(!strcmp(read_buffer("AT+CFUN=1", 9), "OK") == 0) {
    ERROR_code('E'); //mensagem de erro do tipo CME
    return false; //sai da funcao
  }

  //Envia comando para selecionar o formato da mensagem
  serialGPS->write("AT+CMGF=1\r\n"); //SMS Message Format (1 = Text Mode)
  delay(500);

  //Aguarda retorno de resposta 'OK'
  if(!strcmp(read_buffer("AT+CMGF=1", 9), "OK") == 0)
    return false; //caso contrario, sai da funcao
    
  return true; //fim da funcao
}

/*
 * bool SIM_status( )
 * Verifica se alguma senha e' requerida ou nao para o
 * cartao SIM inserido
 */
bool SIM808_methods::SIM_status(void) {
  
  delay(500);

  //Envia comando de verificacao de senha
  serialGPS->write("AT+CPIN?\r\n");
  delay(500);

  //Aguarda retorno de que nao existe senha pendente
  if(!strcmp(read_buffer("AT+CPIN?", 8), "READYOK") == 0) {
    if(strcmp(serial_data, "SIMPINOK") == 0) //resposta 'SIM PIN'
      Serial.println("\r\nAguardando SIM PIN ser fornecido");
    if(strcmp(serial_data, "SIMPUKOK") == 0) //resposta 'SIM PUK'
      Serial.println("\r\nAguardando SIM PUK ser fornecido");
    if(strcmp(serial_data, "PH_SIMPINOK") == 0) //resposta 'PH_SIM PIN'
      Serial.println("\r\nAguardando telefone para cartao SIM (antifurto)");
    if(strcmp(serial_data, "PH_SIMPUKOK") == 0) //resposta 'PH_SIM PUK'
      Serial.println("\r\nAguardando por SIM PUK (antifurto)");
    return false; //resposta nao esperada, sai da funcao
  }

  return true; //fim da funcao
}

/*
 * bool turnon_GPS( )
 * Liga o GPS
 */
bool SIM808_methods::turnon_GPS(void) {
  
  delay(500);

  //Envia comando para ligar o GPS
  serialGPS->write("AT+CGPSPWR=1\r\n"); //GPS Power Controle (1 = turn on GPS power supply)
  delay(500);

  //Aguarda retorno de resposta 'OK'
  if(!strcmp(read_buffer("AT+CGPSPWR=1", 12), "OK") == 0) {
    //serial_data = '89x', com x = [0,1,2]
    if(serial_data[2] == '0') //erro de codigo 890 (GPS not running)
      Serial.println("\r\nGPS nao esta rodando");
    if(serial_data[2] == '1') //erro de codigo 891 (GPS is running)
      Serial.println("\r\nGPS esta rodando, aguarde...");
    if(serial_data[2] == '2') //erro de codigo 892 (GPS is fixing)
      Serial.println("\r\nGPS esta fixando posicao, aguarde...");
    return false; //resposta inesperada 'ERROR', sai da funcao
  }

  return true; //fim da funcao
}

/*
 * bool turnoff_GPS( )
 * Desliga o GPS
 */
bool SIM808_methods::turnoff_GPS(void) {

  delay(500);

  //Envia comando para desligar o GPS
  serialGPS->write("AT+CGPSPWR=0\r\n"); //GPS Power Controle (0 = turn off GPS power supply)
  delay(500);

  //Aguarda retorno de resposta 'OK'
  if(!strcmp(read_buffer("AT+CGPSPWR=0", 12), "OK") == 0) {
    //serial_data = '89x', com x = [0,1,2]
    if(serial_data[2] == '0') //erro de codigo 890 (GPS not running)
      Serial.println("\r\nGPS nao esta rodando");
    if(serial_data[2] == '1') //erro de codigo 891 (GPS is running)
      Serial.println("\r\nGPS esta rodando, aguarde...");
    if(serial_data[2] == '2') //erro de codigo 892 (GPS is fixing)
      Serial.println("\r\nGPS esta fixando posicao, aguarde...");
    return false; //resposta inesperada 'ERROR', sai da funcao
  }
    
  return true; //fim da funcao
}

/*
 * bool GPS_status( )
 * Verifica se o GPS leu alguma possicao
 */
bool SIM808_methods::status_GPS(void) {
  
  delay(500);

  //Envia comando para verificar o status do GPS
  serialGPS->write("AT+CGPSSTATUS?\r\n");
  delay(500);

  //Aguarda retorno de resposta com localizacao fixada
  if((!strcmp(read_buffer("AT+CGPSSTATUS?", 14),"Location3DFixOK") == 0)&&(!strcmp(serial_data,"Location2DFixOK") == 0)) {
    if(strcmp(serial_data, "LocationUnknownOK") == 0) //resposta 'Location Unknown'
      Serial.println("\r\nGPS nao esta ligado");
    if(strcmp(serial_data, "LocationNotFixOK") == 0) //resposta 'Location Not Fix'
      Serial.println("\r\nGPS esta ligado, mas nao tem leitura fixa");
    return false; //resposta nao esperada, sai da funcao
  }
  
  return true; //fim da funcao
}

/*
 * bool read_GPS( )
 * Salva no struct 'GPSdata' as informacoes lidas
 * na ultima leitura de posicao GPS valida
 */
void SIM808_methods::read_GPS(void) {
  
  //Variaveis auxiliares
  int comma = 0; //contador de virgulas
  char aux[20]; //string auxiliar que salva os dados momentaneamente
  int aux_pos = 0; //indicador de posicao auxiliar da string
  char NSEW; //char posicao Norte Sul Leste Oeste
  float temp; //salva os valores float por um tempo
  
  delay(500);

  //Envia comando de leitura da localizacao GPS
  serialGPS->write("AT+CGPSINF=32\r\n"); //'32' indentifica dados $GPRMC
  delay(500);

  //Salva no 'serial_data' a resposta do comando
  read_buffer("AT+CGPSINF=32", 13);

  //serial_data = 'ID,horarioUTC,status,latitude,N/S,longitude,L/O,velocidade,direcao,data,<etc>
  //Loop para percorrer serial_data ignorando os dois ultimos caracteres ('OK')
  for(int i = 0; i < pos-2; i++) {

    //se encontra uma virgula
    if(serial_data[i] == ',') {
      if(comma == 1) { //se aux tiver o horarioUTC
        temp = floor(atof(aux)); //temp = hhmmss
        GPSdata.hour = temp/10000; //hour = hh
        temp -= 10000*GPSdata.hour; //temp = mmss
        GPSdata.minute = temp/100; //minute = mm
        temp -= 100*GPSdata.minute; //temp = ss
        GPSdata.second = temp; //second = ss
      }
      
      else if(comma == 4) { //se aux tiver latitude e NSEW a direcao
        temp = atof(aux); //temp = ddmm.mmmmm
        GPSdata.lat = floor(temp/100); //lat = dd (so graus)
        temp -= (100*GPSdata.lat); //temp = mm.mmmm (so os minutos)
        GPSdata.lat += temp/60; //soma os minutos/60
        if (NSEW == 'S') GPSdata.lat *= -1; //Sul e' indicado por -
      }
      
      else if(comma == 6) { //se aux tiver longitude e NSEW a direcao
        temp = atof(aux); //temp = ddmm.mmmm
        GPSdata.lon = floor(temp/100); //lon = dd (so graus)
        temp -= (100*GPSdata.lon); //temp = mm.mmmm (so os minutos)
        GPSdata.lon += temp/60; //soma os minutos/60
        if (NSEW == 'W') GPSdata.lon *= -1; //Oeste(West) e' indicado por -
      }

      else if(comma == 7) //se aux tiver a velocidade em nos
        GPSdata.speed_kph = atof(aux) * 1.852; //conversao de nos para km/h

      else if(comma == 8) //se aux tiver a direcao em graus
        GPSdata.heading = atof(aux);

      else if(comma == 9) { //se aux tiver a dataUTC
        temp = floor(atof(aux)); //temp = ddmmyy
        GPSdata.day = temp/10000; //day = dd
        temp -= 10000*GPSdata.day; //temp = mmyy
        GPSdata.month = temp/100; //month = mm
        temp -= 100*GPSdata.month; //temp = yy
        GPSdata.year = temp; //year = yy
      }
      
      comma++; //acrescentar contador de virgulas
      if((comma != 4)&&(comma != 6)) //nao reinicia o aux se ele tiver a lat ou lon
        memset(aux, 0, sizeof(aux)); //reiniciar string auxiliar
      aux_pos = 0; //reiniciar auxiliar de posicao
    }

    //caso contrario, caracter e' de interesse (ignora-se o ID, status e dados depois da 'data')
    else {
      if(comma == 1) //horario UTC em formato hhmmss.sss 
        aux[aux_pos++] = serial_data[i];
      else if(comma == 3) //latitude em formato ddmm.mmmmmm
        aux[aux_pos++] = serial_data[i];
      else if(comma == 4) //direcao Norte (+1) ou Sul (-1)
        NSEW = serial_data[i];
      else if(comma == 5) //longitude em formato ddmm.mmmmmm
        aux[aux_pos++] = serial_data[i];
      else if(comma == 6) //direcao East/Leste (+1) ou West/Oeste (-1)
        NSEW = serial_data[i];
      else if(comma == 7) //velocidade em nos
        aux[aux_pos++] = serial_data[i];
      else if(comma == 8) //direcao em graus
        aux[aux_pos++] = serial_data[i];
      else if(comma == 9) //dia UTC em formato ddmmyy
        aux[aux_pos++] = serial_data[i];
    }
  } //fim loop
  
  reset_buffer();
}

/*
 * bool deleteall_SMS( )
 * Deleta todos os SMS no cartao SIM
 */
bool SIM808_methods::deleteall_SMS(void) {

  int timeout = 0; //contador de tempo transcorrido

  //Envia comando para deletar todos os SMS
  serialGPS->write("AT+CMGDA=\"DEL ALL\"\r\n");

  //Enquanto nao se passaram 30s
  while(timeout < 6) {
    delay(5000); //demora ate 25s para deletar 150 mensagens
    timeout++; //soma contador a cada 5s
    if(serialGPS->available()) //se o serial do GPS respondeu com algo
      break; //sai do 'while' para ver a resposta
  }

  //Se nao obter resposta no tempo limite
  if(timeout == 6) {
    Serial.println("Esperou-se 30s (tempo maximo) e nao se obteve resposta...");
    Serial.println("Tente de novo");
    return false; //sai da funcao com fracasso
  }

  //Aguarda retorno de resposta 'OK'
  if(!strcmp(read_buffer("AT+CMGDA=\"DELALL\"", 17), "OK") == 0) {
    if(serial_data[1] == 'E') //serial_data = ERROR
      Serial.println("\r\nAlgo deu errado");
    else //serial_data = +CMS ERROR: <err>
      ERROR_code('S'); //mensagem de erro do tipo CMS
    return false; //resposta inesperada 'ERROR', sai da funcao
  }
  
  return true; //fim da funcao  
}

/*
 * bool delete_SMS(char *index)
 * Deleta um SMS que esta' indexado 
 * com valor 'index'
 */
bool SIM808_methods::delete_SMS(char *index) {

  delay(500);

  //Envia comando para deletar SMS indexidada 'index'
  serialGPS->write("AT+CMGD=");
  serialGPS->write(index);
  serialGPS->write("\r\n");

  delay(5000); //pode demorar ate' 5s para deletar um SMS

  //Salva no 'serial_data' a resposta do comando
  read_buffer("+", 1);

  //Verifica-se entao se os dois ultimos caracteres sao 'OK'
  if(serial_data[pos-2] == 'O') {
    if(serial_data[pos-1] == 'K')
      return true; //sai da funcao com sucesso
  }

  else //caso contrario
    ERROR_code('S'); //mensagem de erro do tipo CMS
  
  return false; //sai da funcao com fracasso
}

/*
 * int new_SMS( )
 * Verifica se a comunicacao serial recebeu uma
 * notificacao de que o SIM tem um novo SMS.
 * Caso tenha, a funcao retorna o 'index', ou seja,
 * o indentificador do novo SMS
 */
int SIM808_methods::new_SMS(void) {
  
  char c; //'char' que fara a leitura da resposta

  reset_buffer(); //por seguranca

  //Se existir algo para ser lido no serial,
  //deve-se verificar se e' uma notificacao de
  //novo SMS, recebido na forma:
  //+CMTI:"SM",i | onde:
  //"SM" = indicando novo SMS
  //i = 'index' da novo mensagem
  if(serialGPS->available()){
    while (serialGPS->available()) { //enquanto existir algo
      c = serialGPS->read(); //salva o 'char' disponivel
      if((c != '\n')&&(c != '\r')&&(c != ' ')) //se nao for um 'whitespace'
        serial_data[pos++] = c; //salva no 'serial_data'
        if (c == ':') { //quando receber o char ':'
                        //verifica se e' um aviso do tipo +CMTI:
          if(!strcmp(serial_data, "+CMTI:") == 0)
            return 0; //caso contrario, sai da funcao com fracasso
          reset_buffer(); //caso positivo, limpa 'serial_data'
        }
        //na virgula, serial_data = "SM"
        if(c == ',')
          reset_buffer(); //portanto, limpa-se o array para guardar so o 'index'
    }
    //converte o 'index' para int e termina a funcao
    return atoi(serial_data);
  }
  
  return 0; //fim da funcao com fracasso
}

/*
 * bool read_SMS_data(int index, char* UUID, int UUID_size)
 * Faz a leitura do SMS com 'index'
 * e verifica se ele tem o UUID correspondente.
 * Se tiver, faz a leitura dos dados da mensagem
 */
bool SIM808_methods::read_SMS_data(int index, char* UUID, int UUID_size) {

  char c; //'char' que fara a leitura da resposta
  char ind[4]; //array com o 'index'
  char aux[20]; //'string' auxiliar que salva os dados momentaneamente
  char number[20]; //'string' com o numero que enviou a mensagem
  int num_pos = 0; //tamanho do numero lido
  int aux_pos = 0; //indicador de posicao auxiliar da string
  int received_uuid_size = 0; //tamanho do UUID recebido
  int comma = 0; //contador de virgulas
  int quotes = 0; //contador de aspas
  int j = 0; //auxiliar para percorrer os array

  itoa(index,ind,10); //converte o int (index) para o array (ind)

  //Envia comando para ler o SMS indexado 'ind'
  serialGPS->write("AT+CMGR=");
  serialGPS->write(ind);
  serialGPS->write("\r\n");
  delay(500);

  reset_buffer(); //por seguranca

  //Enquanto existir algo para ser lido (resposta)
  //serial_data = "RECUNREAD","XXXXXXXXXXXX","","yy/mm/dd,HH:MM:SS+TZ"dataOK
  //"RECUNREAD" = unread record / mensagem nao lida
  //"XXXXXXXXXXXX" = numero de telefone que mandou a mensagem
  //"yy/mm/dd,HH:MM:SS+TZ" = dia e hora com fuso horario (TZ)
  //data = o conteudo da mensagem recebida
  //OK = indicacao que a leitura foi realizada com sucesso
  while (serialGPS->available()) {
    c = serialGPS->read(); //salva o 'char' disponivel
    if (c == '"') //se encontrar um aspas
      quotes++; //soma contador de aspas
    if (c == ',') //se encontrar uma virgula
      comma++; //soma contador de virgulas
    if ((c != '\n')&&(c != '\r')&&(c != ' ')&&(c != '"')) { //se nao for um 'whitespace'
      if((quotes == 3)&&(comma == 1)) //se for o numero que enviou o SMS
        number[num_pos++] = c; //salva na string 'number'
      else if((quotes == 8)&&(pos < 100)) //se for o dataOK
        serial_data[pos++] = c; //salva na string 'data'
    }
  }

  //Se for o SMS que se espera, o seu conteudo (serial_data) deve ser:
  //serial_data = UUID,numero de emergencia,latitude,longitude
  //Portanto, verifica-se se o tamanho do UUID (ate encontrar a 1a virgula)
  //ou ate acabar a string 'data', nesse caso sendo falso
  while ((serial_data[received_uuid_size] != ',')&&(received_uuid_size < pos))
    received_uuid_size++;

  //Se o SMS e' o esperado, isso nunca seria possivel
  //ja que o 'serial_data' tem muito mais do que so o UUID
  if(received_uuid_size == pos) {
    reset_buffer(); //limpa o 'serial_data'
    return false; //sai da funcao com fracasso
  }

  //Se o UUID nao tiver o tamanho certo
  if(received_uuid_size != UUID_size) {
    reset_buffer(); //limpa o 'serial_data'
    return false; //sai da funcao com fracasso
  }

  //Compara-se cada caracter do UUID para ver se e' igual
  while((serial_data[j] == UUID[j])&&(j < UUID_size))
    j++;

  //Se nao for igual
  if(j != UUID_size) {
    delete_SMS(ind); //deleta o SMS
    reset_buffer(); //limpa o 'serial_data'
    return false; //sai da funcao com fracasso
  }

  //Se for o UUID esperado, salva-se o numero que enviou
  //o SMS nos dados do usuario
  for(j = 0; j < num_pos; j++)
    USERdata.user_phone[j] = number[j];

  USERdata.user_phone_size = num_pos;

  comma = 0; //reinicia contador de virgulas

  //Fazer a leitura dos outros dados
  //data = UUID,numero_de_emergencia,latitude,longitude
  //Loop para percorrer data ignorando os dois ultimos caracteres ('OK')
  for(int i = 0; i < pos-2; i++) {
    if(serial_data[i] == ',') { //se encontrar virgula
      if(comma == 1) { //se ja encontrou uma virgula, entao acabou de ler o numero de emergencia
        for(j = 0; j < aux_pos; j++)
          USERdata.help_phone[j] = aux[j]; //salva o numero nos dados
      }
      else if(comma == 2) //se ja encontrou duas virgulas, entao acabou de ler a latitude
        USERdata.lat_home = atof(aux); //salva o numero nos dados
        
      aux_pos = 0; //reinicia contador auxiliar de posicao da string
      memset(aux, 0, sizeof(aux)); //reiniciar string auxiliar
      comma++; //soma contador de virgulas
    }
    else { //caso nao seja uma virgula
      if(comma == 1) //se ja contou uma virgula
        aux[aux_pos++] = serial_data[i]; //salva numero de emergencia
      else if(comma == 2) //se ja contou duas virgulas
        aux[aux_pos++] = serial_data[i]; //salva latitude
      else if(comma == 3) //se ja contou tres virgulas
        aux[aux_pos++] = serial_data[i]; //salva longitude
    }
  } //fim loop

  USERdata.lon_home = atof(aux); //salva longitude

  delete_SMS(ind); //deleta o SMS
  
  return true; //fim da funcao
}

/*
 * int read_SMS(int index, char* PHONE)
 * Faz a leitura do SMS com indexado 'index'
 * e verifica se e' do numero de interesse PHONE.
 * Se for, verifica seu conteudo.
 * Caso contrario, deleta a mensagem
 */
int SIM808_methods::read_SMS_safe(int index, int phone_size, char* PHONE) {

  char c; //'char' que fara a leitura da resposta
  char ind[4]; //array com o 'index'
  char aux[20]; //'string' auxiliar que salva os dados momentaneamente
  int aux_pos = 0; //indicador de posicao auxiliar da string
  int comma = 0; //contador de virgulas
  int quotes = 0; //contador de aspas
  int j = 0; //auxiliar para percorrer os array
  bool flag_msg = false; //indica se a mensagem ja foi lida

  itoa(index,ind,10); //converte o int (index) para o array (ind) 

  //Envia comando para ler o SMS indexado 'ind'
  serialGPS->write("AT+CMGR=");
  serialGPS->write(ind);
  serialGPS->write("\r\n");
  delay(500);

  reset_buffer(); //por seguranca

  //Enquanto existir algo para ser lido (resposta)
  while (serialGPS->available()) {
    c = serialGPS->read(); //salva o 'char' disponivel
    //ignora a resposta "AT+CMGR=ind\r\n+CMGR:"
    //quando abrir aspas, salva a resposta de interesse no 'serial_data'
    if (c == '"')
      flag_msg = true;
    if (((c != '\n')&&(c != '\r')&&(c != ' '))&&flag_msg) //se nao for um 'whitespace'
      serial_data[pos++] = c; //salva no 'serial_data'
  }

  //serial_data = "RECUNREAD","XXXXXXXXXXXX","","yy/mm/dd,HH:MM:SS+TZ"dataOK
  //"RECUNREAD" = unread record / mensagem nao lida
  //"XXXXXXXXXXXX" = numero de telefone que mandou a mensagem
  //"yy/mm/dd,HH:MM:SS+TZ" = dia e hora com fuso horario (TZ)
  //data = o conteudo da mensagem recebida
  //OK = indicacao que a leitura foi realizada com sucesso
  //Loop para percorrer serial_data ignorando os dois ultimos caracteres ('OK')
  for(int i = 0; i < pos-2; i++) {
    //se encontra uma virgula
    if(serial_data[i] == ',') {
      //se encontrou uma virgula e ja contou outra
      //significa que acabou de salvar o numero do contato
      if(comma == 1) {
        if(aux_pos == phone_size) { //se o numero tem o tamanho salvo
          while(aux[j] = PHONE[j]) //enquanto os numeros foram iguais
            j++; //percorre o array com o numero salvo e o numero que envio o SMS
          if(j == phone_size) { //se os numeros forem exatamente iguais
            memset(aux, 0, sizeof(aux)); //reiniciar string auxiliar
            aux_pos = 0; //reiniciar auxiliar de posicao
          }
          else { //se forem diferentes
            delete_SMS(ind); //deleta o SMS
            return 0; //sai da funcao com fracasso
          }
        } //fim do 'if aux_pos==phone_size'
        else { //se nao tiver o tamanho salvo
          delete_SMS(ind); //deleta o SMS
          return 0; //sai da funcao com fracasso
        }
      } //fim do 'if comma == 1'
      comma++; //acrescentar contador de virgulas
    } //fim do 'if virgula'

    //se encontrar uma aspas
    else if(serial_data[i] == '"')
      quotes++; //acrescentar contador de aspas

    //caso contrario, caracter e' de interesse (ignora-se o RECUNREAD, dia e hora)
    else {
      if(comma == 1) //depois da primeira virgula, salva-se o numero de telefone
        aux[aux_pos++] = serial_data[i];
      if(quotes == 8) //depois da oitava aspas, salva-se o conteudo do SMS
        aux[aux_pos++] = serial_data[i];
    } //fim do 'else'
  } //fim loop
  
  reset_buffer(); //limpa o 'serial_data'
  delete_SMS(ind); //deleta o SMS

  if(strcmp(aux, "Estou bem!") == 0) //se o conteudo era 'seguro'
    return 1; //retorna valor '1'
  
  return 2; //se nao era nenhum dos dois, retorna valor '2'
}

/*
 * bool send_SMS(char *PHONE, char *MESSAGE)
 * Envia um SMS com a mensagem MESSAGE para 
 * o numero de celular PHONE
 */
bool SIM808_methods::send_SMS(char *PHONE, char *MESSAGE) {

  delay(500);

  //Envia comando para selecionar o formato da mensagem
  serialGPS->write("AT+CMGF=1\r\n"); //SMS Message Format (1 = Text Mode)
  delay(500);

  //Aguarda retorno de resposta 'OK'
  if(!strcmp(read_buffer("AT+CMGF=1", 9), "OK") == 0)
    return false; //caso contrario, sai da funcao
  delay(500);

  //Envia comando para selecionar o formato dos caracteres
  serialGPS->write("AT+CSCS=\"GSM\"\r\n"); //TE Character Set (GSM = 7 bit default alphabet)
  delay(500);

  //Aguarda retorno de resposta 'OK'
  if(!strcmp(read_buffer("AT+CSCS=\"GSM\"", 13), "OK") == 0)
    return false; //caso contrario, sai da funcao

  //Envia comando para enviar SMS 
  //Comando deve ter forma +GSM=<da><text><ctrlz>, onde
  serialGPS->write("AT+CMGS=\""); //<da> = destination-adress
  serialGPS->write(PHONE); //no caso, o telefone que recebera a mensagem
  serialGPS->write("\"\r\n");
  delay(1000);

  //Aguarda '>' indicando que a mensagem pode ser escrita
  if(!wait_response('>'))
    return false; //caso contrario, sai da fucnao
    
  serialGPS->write(MESSAGE); //<text> com o corpo da mensagem
  serialGPS->write("\r\n");
  delay(500);
  
  serialGPS->write((char)26); //<ctrlz>, no caso, o caracter ASCII numero (decimal) 26
  serialGPS->write("\r\n");
  delay(5000);

  //Salva no 'serial_data' a resposta do comando
  read_buffer("+", 1);

  //A resposta tem formato +CMGS:<mr>OK, onde
  //<mr> e' um inteiro que indentifica a mensagem (ID)
  //Verifica-se entao se os dois ultimos caracteres sao 'OK'
  if(serial_data[pos-2] == 'O') {
    if(serial_data[pos-1] == 'K')
      return true;
  }

  else //caso contrario
    ERROR_code('S'); //mensagem de erro do tipo CMS
  
  return false; //sai da funcao
}

/*
 * void reset_buffer( )
 * Limpa o vetor de caracteres 'serial_data'
 * e o contador de posicao 'pos'
 */
void SIM808_methods::reset_buffer(void) {
  memset(serial_data, 0, sizeof(serial_data));
  pos = 0;
}

/*
 * char* read_buffer(char* msg_sent, int msg_size)
 * Salva em 'serial_data' a reposta do modulo a um 
 * comando 'msg_sent' pelo serial.
 * O ESP32 recebe o comando enviado e uma resposta,
 * portanto, deve-se ignorar a confirmacao do que foi
 * enviado e salvar apenas a resposta.
 * A resposta tem formato +CXXX: <data>, onde apenas
 * <data> e' de interesse.
 */
char* SIM808_methods::read_buffer(char* msg_sent, int msg_size) {

  char c; //'char' que fara a leitura da resposta

  reset_buffer(); //por seguranca

  //Enquanto existir algo para ser lido
  while (serialGPS->available()) {
    c = serialGPS->read(); //salva o 'char' disponivel
    if ((c != '\n')&&(c != '\r')&&(c != ' ')) { //se nao for um 'whitespace'
      serial_data[pos++] = c; //salva no 'serial_data'
      if(c == ':') //se for um indicador de resposta
        reset_buffer(); //limpa 'serial_data'
      if((pos == msg_size)&&(strcmp(serial_data, msg_sent) == 0)) //se for a confirmacao de comando
        reset_buffer(); //limpa 'serial_data'
    }
  }

  //E' de interesse retornar o array para comparacao com 'strcmp'
  return serial_data; //sai da funcao
}

/*
 * bool wait_response(char r)
 * Aguarda o retorno de um 'char' especifico
 * ou ate o tempo acabar
 */
bool SIM808_methods::wait_response(char r) {

  char c; //'char' que fara a leitura da resposta
  unsigned long time_begin = millis();
  unsigned long time_now = millis();
  
  //Enquanto existir algo para ser lido e tempo esperado < 20s
  while (time_now - time_begin < 20000) {
    while (serialGPS->available()) {
      c = serialGPS->read(); //salva o 'char' disponivel
      if (c == r) //verifica se ele e' o esperado
        return true; //sai da funcao
    }
    delay(1000);
    time_now = millis();
  }
  
  return false; //sai da funcao
}

/*
 * void ERROR_code(char x)
 * Mostra no 'Monitor serial' a resposta de erro
 * que o ESP32 recebeu
 */
void SIM808_methods::ERROR_code(char x) {

  //char x indentifica se o erro e' do tipo CME ou CMS
  if(x == 'E') {
    Serial.println("\r\nErro do tipo CME ERROR");
    Serial.println("Erro relacionado com equipamento movel ou network");
    Serial.println("Codigo do erro: ");
    Serial.print(serial_data);
    Serial.println("\r\nVerificar manual SIM808 Series_AT_Command Manual_V1.02");
    Serial.println("Paginas 320 a 323");
  }

  else if (x == 'S') {
    Serial.println("\r\nErro do tipo CMS ERROR");
    Serial.println("Erro relacionado com serviço de mensagem ou network");
    Serial.println("Codigo do erro: ");
    Serial.print(serial_data);
    Serial.println("\r\nVerificar manual SIM808 Series_AT_Command Manual_V1.02");
    Serial.println("Paginas 323 a 326");
  }

  else //erro estranho ao programa
    Serial.println("Erro indevido");  
}

