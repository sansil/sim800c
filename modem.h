#ifndef MODEM_H
#define MODEM_H

#include "mbed.h"
#include "public_info.h"

// const uint16_t MAX_LENGTH_RX_BUFF = 255;
#define MAX_INTENTOS_MDM 15
#define MAX_LENGTH_RX_BUFF 255
#define APN_NAME "\"antel.lte\""     //"gprs.ancel"//"prepago.ancel"//"cuenca.vpnantel"antel.lte
#define URL_BASE "https://reqres.in" ///api/register {
// "email" : "sydney@fife",
//           "password" : "pistol"
// }

struct rxBuffer
{
  volatile char buff[MAX_LENGTH_RX_BUFF];
  volatile uint16_t ind = 0;
  volatile bool new_data = false;
  void add_c(char c);
  void clear();
  uint16_t get_size_buffer(uint16_t offset);
};

typedef enum error_modem_t
{
  ERROR_MDM = 0,
  BUSY_MDM,
  OK_MDM
} error_modem_t;

typedef enum request_t
{
  modem_on = 0,
  modem_begin,
  modem_getcclk,
  modem_setcclk,
  modem_getimei,
  modem_sendgprs,
  modem_sendtcp,
  modem_checktcp_connection,
  modem_readtcp,
  modem_start_tcp_config,
  modem_closetcp,
  modem_getrssi,
  modem_getgsmloc,
  modem_off,
  modem_download_ftp
} request_t; // request a pedirle desde el exterior

typedef enum modem_state_t
{
  en_regulador,
  pw_key_pulse_on,
  pw_key_pulse_off,
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
  test_gprs_connection,
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
  gprs_status, //At+CGATT?
  cgatt,
  tcp_cstt,
  tcp_ciicr,
  tcp_cifsr,
  tcp_cipshut,
  tcp_cipmux,
  tcp_ciprxget,
  tcp_cipstart,
  tcp_cipclose,
  tcp_cipstatus,
  tcp_cipstatus_connected,
  tcp_cipsend,
  tcp_cipsending,
  tcp_available,
  tcp_read,
  gsm_loc,
  no_op,
  error_mdm,
  esperando_respuesta,
  procesar_respuesta,
  procesar_mensaje,
  wait_timeout,
  finish_task
} modem_state_t; // request a pedirle desde el exterior

typedef struct s_state_t
{
  modem_state_t estado_actual;
  modem_state_t estado_proximo;
  modem_state_t estado_anterior;
  char resp_esperada[30];
} s_state_t;

typedef struct message_t
{
  char *payload;
  uint16_t largo;
} message_t;

typedef enum modem_event_t
{
  TCP_MESSAGE_RECIVED = 0,
  TCP_MESSAGE_SEND,
  TCP_CONNECTED,
  TCP_DISCONNECTED,
  IMEI_RDY,
  RSSI_RDY,
  TIMESTAMP_RDY,
  GSM_LOC_RDY,
  MDM_RDY
} modem_event_t;

//TODO: Comando a agregar :
// 2) TCP/IP manage
// 3) Estado de conxion? AT+CGATT?
// 4) Network time sync? AT+CNTPCID=1, AT+CNTP="pool.ntp.org"
class Sim800c
{
private:
  /* data */
  uint16_t count_intentos = 0;
  void send_msg(const char *msg);
  void send_break_msg(const char *msg);
  void write_msg(char *msg, uint16_t large);
  void flush();
  Timeout timeout;
  int largo_respuesta_http; //ver de quitar
  void timeout_callback();
  DigitalOut m_pinReg;
  DigitalOut m_powKey;
  s_state_t s_state;
  modem_event_t event;

public:
  rxBuffer rxBuff;
  RawSerial module;
  message_t message;
  message_t message_in;
  volatile bool timeout_flag{0};
  error_modem_t error_modem;
  char m_imei[20];
  int m_rssi{0};
  char timestamp[30];
  float lat{0};
  float lon{0};
  bool mode_high_level_fuction = false;  // si es true entonces configuro y uso funciones http de alto nivel, sino vuelvo
  void (*callback)();                    // se ejecuta por ejemplo al obtner el imei o el timestamp
  void (*event_handle)(modem_event_t e); // aviso a main ppal cuando termino un task
  Sim800c(PinName tx, PinName rx, PinName pinReg = PA_0, PinName powKey = PA_1, void (*modem_handler)(modem_event_t e) = NULL) : event_handle(modem_handler), m_pinReg(pinReg), m_powKey(powKey), module(tx, rx){};
  void begin(uint16_t baud);
  void task();
  void set_request(request_t req, void (*data)())
  {
    error_modem = BUSY_MDM;
    callback = NULL;
    count_intentos = 0;
    switch (req)
    {
    case modem_on:
      s_state.estado_actual = en_regulador;
      break;
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
    case modem_getgsmloc:
      callback = data;
      s_state.estado_actual = gsm_loc;
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
    case modem_start_tcp_config:
      s_state.estado_actual = tcp_cipshut;
      break;
    case modem_sendtcp:
      callback = data;
      s_state.estado_proximo = tcp_cipsend;
      s_state.estado_actual = tcp_cipstatus_connected; // check if connect ok
      break;
    case modem_readtcp:
      callback = data;
      s_state.estado_proximo = tcp_available; // check if connect ok
      s_state.estado_actual = tcp_cipstatus_connected;
      break;
    case modem_checktcp_connection:
      callback = data;
      s_state.estado_actual = tcp_cipstatus_connected;
      break;
    case modem_closetcp:
      callback = data;
      s_state.estado_actual = tcp_cipclose;
      break;
    default:
      break;
    }
  }
  ~Sim800c();
};

#endif
