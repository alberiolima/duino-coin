/*
  Baseado no código (DoinoCoin_Tiny_Slave.ino) de Luiz H. Cassettari
  Testado utlizando ATTinyCore
  Attiny85 com bootloader optboot(serial) com crystal externo de 20mhz
  Pino TX 0 (PB0)
  Pino RX 1 (PB1)
  OBS1:É necessário modificar o AVR_Minter.py modificando o baudrate da porta serial de 115200 para 57600
  Resultado obtido foi de 277H/s com diff 6
  No digispark 16.5mhz (sem crystal) 229 H/s com diff 6
*/

#include <avr/boot.h>
#include "sha1.h"
#ifndef SIGRD
  #define SIGRD 5
#endif

#define BUFFER_MAX        88
#define HASH_BUFFER_SIZE  20
#define CHAR_END          '\n'
#define CHAR_DOT          ','

enum {
  statusRecebendoTrabalho = 0,
  statusTrabalhando,
  statusConcluido
};

static const char DUCOID[] PROGMEM = "DUCOID";

static char buffer[BUFFER_MAX];
static uint8_t buffer_position;
static uint8_t buffer_length;
static uint8_t statusOperacao = statusRecebendoTrabalho;
uint8_t qCHAR_DOT = 0;

void setup() {
  Serial.begin(57600);
  Serial.setTimeout(10000);
  Serial.flush();
}

void loop() {
  /* Recebe trabalho */
  if (Serial.available()) {
    qCHAR_DOT = 0;
    buffer_length = 0;
    while ((statusOperacao == statusRecebendoTrabalho)&&(Serial.available())) {
      while ((statusOperacao == statusRecebendoTrabalho)&&(Serial.available())) {
        char c = Serial.read();
        buffer[buffer_length++] = c;
        if (buffer_length >= BUFFER_MAX) buffer_length--;
        if (c == CHAR_DOT) {
          qCHAR_DOT++;
        }
        if ((c == CHAR_END)||(qCHAR_DOT>=3)) {
          statusOperacao = statusTrabalhando;
        }
      }
      delay(2);
    }
    //Serial.write(buffer,buffer_length); //debug
  }

  millis();
  /* Executa trabalho */
  if (statusOperacao == statusTrabalhando ) {
    do_job();    
  }

  /* Envia trabalho */
  if (statusOperacao == statusConcluido ) {
    Serial.write(buffer, buffer_length);    
    statusOperacao = statusRecebendoTrabalho;    
  }
}

void do_job() {
  uint32_t startTime = micros();
  int job = work();
  uint32_t elapsedTime = micros() - startTime;
    
  char cstr[64];
  memset(buffer, 0, sizeof(buffer));
  
  // Job
  memset(cstr, 0, sizeof(cstr));
  itoa(job, cstr, 2);
  strcpy(buffer, cstr);
  buffer[strlen(buffer)] = CHAR_DOT;

  // Time
  memset(cstr, 0, sizeof(cstr));
  ultoa(elapsedTime, cstr, 2);
  strcpy(buffer + strlen(buffer), cstr);  
  buffer[strlen(buffer)] = CHAR_DOT;  

  // DUCOID
  strcpy_P(buffer + strlen(buffer), DUCOID);
  for (size_t i = 0; i < 8; i++)   {
    uint8_t sig = boot_signature_byte_get(0x0E + i + ( i > 5 ? 1 : 0));
    itoa(sig, cstr, 16);
    if (sig < 16) strcpy(buffer + strlen(buffer), "0");
    strcpy(buffer + strlen(buffer), cstr);
  }

  buffer[strlen(buffer)] = CHAR_END;
  buffer_length = strlen(buffer);  

  statusOperacao = statusConcluido;
  
}

int work() {
  char delimiters[] = ",";
  char *lastHash = strtok(buffer, delimiters);
  char *newHash = strtok(NULL, delimiters);
  char *diff = strtok(NULL, delimiters);
  return work(lastHash, newHash, atoi(diff));
}

//#define HTOI(c) ((c<='9')?(c-'0'):((c<='F')?(c-'A'+10):((c<='f')?(c-'a'+10):(0))))
#define HTOI(c) ((c<='9')?(c-'0'):((c<='f')?(c-'a'+10):(0)))
#define TWO_HTOI(h, l) ((HTOI(h) << 4) + HTOI(l))
//byte HTOI(char c) {return ((c<='9')?(c-'0'):((c<='f')?(c-'a'+10):(0)));}
//byte TWO_HTOI(char h, char l) {return ((HTOI(h) << 4) + HTOI(l));}

// DUCO-S1A hasher
uint32_t work(char * lastblockhash, char * newblockhash, uint16_t difficulty) {
  if (difficulty > 655) return 0;
  for (int i = 0; i < HASH_BUFFER_SIZE; i++) { 
    newblockhash[i] = TWO_HTOI(newblockhash[2 * i], newblockhash[2 * i + 1]);
  }
  difficulty = difficulty * 100 + 1;
  for (uint16_t ducos1res = 0; ducos1res < difficulty; ducos1res++) {
    Sha1.init();
    Sha1.print(lastblockhash);
    Sha1.print(ducos1res);
    if (memcmp(Sha1.result(), newblockhash, HASH_BUFFER_SIZE) == 0) {
      return ducos1res;
    }
  }
  return 0;
}
