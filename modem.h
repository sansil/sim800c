#ifndef MODEM_H
#define MODEM_H

#include "mbed.h"

const uint16_t MAX_LENGTH_RX_BUFF = 255;

struct rxBuffer
{
  volatile char buff[MAX_LENGTH_RX_BUFF];
  volatile uint16_t ind = 0;
  volatile bool new_data = false;
  void add_c(char c);
  void clear();
};

//TODO: agregar evento de buffer lleno y notificar al thread ppal.
void rxBuffer::add_c(char c)
{
  if (rxBuffer::ind < MAX_LENGTH_RX_BUFF)
  {
    rxBuffer::buff[rxBuffer::ind] = c;
    rxBuffer::ind++;
  }
  rxBuffer::new_data = true;
}

void rxBuffer::clear()
{
  memset((char *)rxBuffer::buff, '\0', MAX_LENGTH_RX_BUFF);
  rxBuffer::ind = 0;
}

typedef enum error_modem_t
{
  ERROR_MDM = 0,
  BUSY_MDM,
  OK_MDM
} error_modem_t;

typedef enum request_t
{
  modem_begin = 0,
  modem_getcclk,
  modem_setcclk,
  modem_getimei,
  modem_sendgprs,
  modem_getrssi,
  modem_off,
  modem_download_ftp
} request_t; // request a pedirle desde el exterior

typedef enum modem_state_t
{
  inicio,
  registro_red,
  imei,
  fecha_hora,
  hora_red,
  guardar_busqueda_hora_red,
  rm_eco,
  rssi,
  apagar_mdm,
  software_reset,
  http_config_1,
  http_config_2,
  http_config_3,
  http_config_4,
  http_send_msg_1,
  http_send_msg_2,
  http_send_msg_3,
  http_send_msg_4,
  http_send_msg_5,
  http_send_msg_6,
  http_send_msg_7,
  http_send_msg_8,
  http_send_msg_9,
  config_ftp_1,
  config_ftp_2,
  config_ftp_3,
  config_ftp_4,
  config_ftp_5,
  download_ftp_1,
  download_ftp_2,
  download_ftp_3,
  download_ftp_4,
  download_ftp_5,
  no_op,
  error_mdm,
  esperando_respuesta,
  procesar_respuesta,
  procesar_mensaje
} modem_state_t; // request a pedirle desde el exterior

typedef struct s_state_t
{
  modem_state_t estado_actual;
  modem_state_t estado_proximo;
  modem_state_t estado_anterior;
  char resp_esperada[30];
} s_state_t;

class Sim800c
{
private:
  /* data */
  RawSerial module;

public:
  rxBuffer rxBuff;
  error_modem_t error_modem;
  s_state_t s_state;
  Sim800c(PinName tx, PinName rx) : module(tx, rx){};
  ~Sim800c();
};

Sim800c::~Sim800c()
{
}

#endif
