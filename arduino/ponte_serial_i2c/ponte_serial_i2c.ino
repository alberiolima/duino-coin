/*
 * Ponte Serial para I2C
 * Liga um master-I2C e vários escravos-I2C
 * Necessário para ligar vários escravos I2C em uma unica porta serial, possivelmente serial-usb
 * Desenvolvido por Albério Lima
 * 
 * Ex: de pacote a receber 
 * $I2C,1,969fad934741416d4f4a92a5bdc932bbb39b34b7,cd012ce0f29d4dd364e935ad01d035bc0454e0e8,6\n
 * 1) $I2C => Cabeçalho do pacote
 * 2) 1    => Endereço I2C do escravo (possivelmente um attiny85
 * 3) o restante é o job recebido do servidor duino-coin => 969fad934741416d4f4a92a5bdc932bbb39b34b7,cd012ce0f29d4dd364e935ad01d035bc0454e0e8,6
 * \n Quebra de linha indica o final do pacote
 * 
 * Ex do resultado:
 * $I2C,1,410,2332124,DUCOID3630323437060916\n
*/
#include <Wire.h>

#define I2C_MAX_ESCRAVOS      50
#define I2C_MASTER_ENDERECO  100
#define I2C_CLOCK 100000
#define SERIAL_BAUD 57600

/* Variáveis públicas */
String rx_buffer = "";
boolean recJob = false;
boolean enviouJob = false;
uint8_t i2c_escravos[I2C_MAX_ESCRAVOS];
uint8_t posicaoCosulta = 0;

void setup() {

  /* Inicia comunicação Serial */
  Serial.begin(SERIAL_BAUD);
  
  /* Inicia comunicação I2C */
  #ifdef I2C_MASTER_ENDERECO
    Wire.begin(I2C_MASTER_ENDERECO);
  #endif  
  #ifdef I2C_CLOCK
    Wire.setClock(I2C_CLOCK);
  #endif  
  
  /* Inicia tabela de escravos */
  for ( uint8_t i = 0; i < I2C_MAX_ESCRAVOS;i++) {
    i2c_escravos[i] = 0;
  }
    
}

void loop() {
  
  /* Recebe trabalho via Serial */
  recJob = recebeuTrabalho();  
  
  /* Envia Job para escravo */
  if (recJob) {
    enviouJob = enviaTrabalho();
  }
  
  /* Consulta escravo */
  if ( i2c_escravos[posicaoCosulta] != 0 ) {
    if ( Wire.requestFrom(1, 1) > 0 ) {
      char c = Wire.read();
      if ( c != '\n' ) { 
        String tx_buffer = "$I2C,"+String(i2c_escravos[posicaoCosulta])+","+c;
        while ( c != '\n' ) { 
          Wire.requestFrom(i2c_escravos[posicaoCosulta], 1);
          if (Wire.available()) { 
            c = Wire.read();
            tx_buffer += c;
          }
        }        
        
        /* Envia resultado para do trabalho */
        Serial.print(tx_buffer);
        
        /* Retira endereço da tabela de consulta */
        i2c_escravos[posicaoCosulta];
      }
    }
  }

  /* Incrementa para próximo endereço escravo a consultar */
  posicaoCosulta++;
  if ( posicaoCosulta >= I2C_MAX_ESCRAVOS ) {
    posicaoCosulta = 0;
  }
  
}

boolean recebeuTrabalho() {
  while (Serial.available()) {
    while (Serial.available()) {
      char c = Serial.read();
      rx_buffer += c;
      if (c == '\n' ){
        return true;
      } else if (c == '$'){
        rx_buffer = c; 
      }
    }
    delay(2);
  }
  return false;
}

boolean enviaTrabalho() {
  /* Verifica se o pacote está válido */
  if (rx_buffer.substring(0,5)!="$I2C,") {
    Serial.println("invalido");
  }
  /* Remove o cabeçalho do pacote */
  rx_buffer = rx_buffer.substring(5);
  
  /* Pega o endereço I2C */
  int endI2C = rx_buffer.substring(0,rx_buffer.indexOf(',')).toInt();

  /* Aloja o endereço numa posição para consulta */
  for ( uint8_t i = 0; i < I2C_MAX_ESCRAVOS; i++ ) {
    if ( i2c_escravos[i] == 0 ) {
      i2c_escravos[i] = (uint8_t)endI2C;
    }
  }
    
  /* Remove o endereço I2C */
  rx_buffer = rx_buffer.substring(rx_buffer.indexOf(',')+1);
  
  /* Verifica comunicação com o endereço do escravo */
  Wire.beginTransmission(endI2C);
  byte error = Wire.endTransmission();
  if (error != 0) {
    return false;  
  }
  
  /* Envia restante do pacote para o escravo */
  for ( uint16_t i = 0; i < rx_buffer.length(); i++ ) {
    Wire.beginTransmission(endI2C); 
    while (!Wire.write(rx_buffer[i])) {
      delay(2);
    }
    Wire.endTransmission();  
  }
  return true;
}
