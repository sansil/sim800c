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
  uint16_t count_intentos = 0;

public:
  rxBuffer rxBuff;
  RawSerial module;
  error_modem_t error_modem;
  s_state_t s_state;
  char m_imei[20];
  int m_rssi;
  char timestamp[30];
  void (*callback)();
  Sim800c(PinName tx, PinName rx) : module(tx, rx){};
  void begin(uint16_t baud);
  void set_request(request_t req, void (*data)())
  {
    error_modem = BUSY_MDM;
    callback = NULL;
    count_intentos = 0;
    switch (req)
    {
    case modem_begin:
      s_state.estado_actual = inicio;
      break;
    case modem_getcclk:
      callback = data;
      s_state.estado_actual = fecha_hora;
      break;
    case modem_getimei:
      callback = data;
      s_state.estado_actual = imei;
      break;
    case modem_getrssi:
      callback = data;
      s_state.estado_actual = rssi;
      break;
    case modem_off:
      s_state.estado_actual = apagar_mdm;
      break;
    case modem_sendgprs:
      s_state.estado_actual = http_send_msg_1;
      break;
    case modem_download_ftp:
      s_state.estado_actual = download_ftp_1;
      break;
    default:
      break;
    }
  }
  ~Sim800c();
};

#endif
