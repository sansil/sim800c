#include "modem.h"
#include "mbed.h"

//#define MODEM_TRANSPARENT_MODE

// Use 3 (MQTT 3.0) or 4 (MQTT 3.1.1)
#define MQTT_PROTOCOL_LEVEL 4

#define MQTT_CTRL_CONNECT 0x1
#define MQTT_CTRL_CONNECTACK 0x2
#define MQTT_CTRL_PUBLISH 0x3
#define MQTT_CTRL_PUBACK 0x4
#define MQTT_CTRL_PUBREC 0x5
#define MQTT_CTRL_PUBREL 0x6
#define MQTT_CTRL_PUBCOMP 0x7
#define MQTT_CTRL_SUBSCRIBE 0x8
#define MQTT_CTRL_SUBACK 0x9
#define MQTT_CTRL_UNSUBSCRIBE 0xA
#define MQTT_CTRL_UNSUBACK 0xB
#define MQTT_CTRL_PINGREQ 0xC
#define MQTT_CTRL_PINGRESP 0xD
#define MQTT_CTRL_DISCONNECT 0xE

#define MQTT_QOS_1 0x1
#define MQTT_QOS_0 0x0

#define CONNECT_TIMEOUT_MS 6000
#define PUBLISH_TIMEOUT_MS 500
#define PING_TIMEOUT_MS 500
#define SUBACK_TIMEOUT_MS 500

// Adjust as necessary, in seconds.  Default to 5 minutes.
#define MQTT_CONN_KEEPALIVE 300

// Largest full packet we're able to send.
// Need to be able to store at least ~90 chars for a connect packet with full
// 23 char client ID.
#define MAXBUFFERSIZE (150)

#define MQTT_CONN_USERNAMEFLAG 0x80
#define MQTT_CONN_PASSWORDFLAG 0x40
#define MQTT_CONN_WILLRETAIN 0x20
#define MQTT_CONN_WILLQOS_1 0x08
#define MQTT_CONN_WILLQOS_2 0x18
#define MQTT_CONN_WILLFLAG 0x04
#define MQTT_CONN_CLEANSESSION 0x02

Serial pc(USBTX, USBRX); // tx, rx
RawSerial modem(PA_9, PA_10);
//SerialBase::TxIrq
DigitalOut led1(LED1);
DigitalOut led2(PB_13);
DigitalOut led3(PB_14);
volatile int buff_ind = 0;
volatile bool data_rdy = false;
volatile char buff[50];
rxBuffer modem_buff;
void modem_hanlde();

Sim800c simcom(PA_9, PA_10, PB_13, PB_14, modem_hanlde);
Timeout flipper;
uint8_t connectPacket(uint8_t *packet);
// void rx_callback()
// {
//   data_rdy = true;
//   while (modem.readable())
//   {
//     modem_buff.add_c((char)modem.getc());
//   }
// }

void printTime()
{
  printf("Hora recibida: ");
  printf(simcom.timestamp);
  printf("\n\r");
}
void printIMEI()
{
  printf("IMEI obtendido: ");
  printf(simcom.m_imei);
  printf("\n\r");
}

void printGSMLoc()
{
  char buf_aux[30];
  printf("Longitud y Latitud: ");
  sprintf(buf_aux, "%f, %f", simcom.lon, simcom.lat); //4 is mininum width, 6 is precision
  printf("%s\n\r", buf_aux);
  printf("\n\r");
}

void printRSSI()
{
  printf("RSSI obtenido: ");
  printf("%d", simcom.m_rssi);
  printf("\n\r");
}
bool pedir_time = false;
bool pedir_imei = false;
bool pedir_gsmLoc = false;
bool pedir_rssi = false;
bool pedir_tcp_config = false;
uint8_t buffer[150];
void modem_hanlde()
{

  if (simcom.error_modem == OK_MDM)
  {
    printf("tarea en modem terminada\n\r");
    if (!pedir_time)
    {
      printf("pidiendo hora\n\t");
      simcom.set_request(modem_getcclk, printTime);
      pedir_time = true;
      pedir_imei = true;
    }
    else if (pedir_imei)
    {
      simcom.set_request(modem_getimei, printIMEI);
      pedir_imei = false;
      pedir_gsmLoc = true;
    }
    else if (pedir_gsmLoc)
    {
      simcom.set_request(modem_getgsmloc, printGSMLoc);
      pedir_gsmLoc = false;
      pedir_rssi = true;
    }
    else if (pedir_rssi)
    {
      printf("pidiendo RSSI\n\r");
      simcom.set_request(modem_getrssi, printRSSI);
      pedir_rssi = false;
      pedir_tcp_config = true;
    }
    else if (pedir_tcp_config)
    {
      printf("mensaje tcp\n\r");
      uint8_t len = connectPacket(buffer);
      for (int i = 0; i < len + 3; i++)
      {
        printf("%d\n\r", buffer[i]);
      }
      simcom.message.payload = (char *)buffer;
      printf("largo real mensaje %d \n\r", strlen(simcom.message.payload));
      simcom.message.largo = len;
      simcom.set_request(modem_sendtcp, NULL);
      printf("mensaje a enviar: %s \n\rlargo: %d\n\r", (char *)buffer, len);
      // pedir_tcp_config = false;
    }
  }
}
volatile bool timeo = false;
void flip()
{
  timeo = !timeo;
}
int main()
{
  // led2 = 1;
  // led3 = 1;
  // wait(2);
  // led3 = 0;
  pc.printf("Starting sim800c demo .\n\r");

  //simcom.begin(9600);
  //flipper.attach(&flip, 2.0); // setup flipper to call flip after 2 seconds
#ifdef MODEM_TRANSPARENT_MODE
  modem.baud(9600);
#else
  simcom.begin(9600);
  flipper.attach(&flip, 2.0); // setup flipper to call flip after 2 seconds
  simcom.set_request(modem_on, NULL);
#endif
  // modem.attach(&rx_callback, SerialBase::RxIrq);

  while (1)
  {
#ifdef MODEM_TRANSPARENT_MODE
    if (pc.readable())
    {
      modem.putc(pc.getc());
    }
    if (modem.readable())
    {
      pc.putc((char)modem.getc());
    }
#else

    simcom.task();
    if (timeo)
    {
      timeo = false;
      printf("timeout en main \r\n");
    }
#endif
    // if (simcom.rxBuff.new_data)
    // {
    //   simcom.rxBuff.new_data = false;
    //   printf("%s", simcom.rxBuff.buff);
    //   data_rdy = false;
    // }
  }
}

static uint8_t *stringprint(uint8_t *p, const char *s, uint16_t maxlen = 0)
{
  // If maxlen is specified (has a non-zero value) then use it as the maximum
  // length of the source string to write to the buffer.  Otherwise write
  // the entire source string.
  uint16_t len = strlen(s);
  if (maxlen > 0 && len > maxlen)
  {
    len = maxlen;
  }
  /*
  for (uint8_t i=0; i<len; i++) {
    Serial.write(pgm_read_byte(s+i));
  }
  */
  p[0] = len >> 8;
  p++;
  p[0] = len & 0xFF;
  p++;
  strncpy((char *)p, s, len);
  return p + len;
}

// The current MQTT spec is 3.1.1 and available here:
//   http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc398718028
// However this connect packet and code follows the MQTT 3.1 spec here (some
// small differences in the protocol):
//   http://public.dhe.ibm.com/software/dw/webservices/ws-mqtt/mqtt-v3r1.html#connect
uint8_t connectPacket(uint8_t *packet)
{
  uint8_t *p = packet;
  uint16_t len;

  // fixed header, connection messsage no flags
  p[0] = (MQTT_CTRL_CONNECT << 4) | 0x0;
  p += 2;
  // fill in packet[1] last

#if MQTT_PROTOCOL_LEVEL == 3
  p = stringprint(p, "MQIsdp");
#elif MQTT_PROTOCOL_LEVEL == 4
  p = stringprint(p, "MQTT");
#else
#error "MQTT level not supported"
#endif

  p[0] = MQTT_PROTOCOL_LEVEL;
  p++;

  // always clean the session
  p[0] = MQTT_CONN_CLEANSESSION;

  // set the will flags if needed
  // if (will_topic && pgm_read_byte(will_topic) != 0)
  // {

  //   p[0] |= MQTT_CONN_WILLFLAG;

  //   if (will_qos == 1)
  //     p[0] |= MQTT_CONN_WILLQOS_1;
  //   else if (will_qos == 2)
  //     p[0] |= MQTT_CONN_WILLQOS_2;

  //   if (will_retain == 1)
  //     p[0] |= MQTT_CONN_WILLRETAIN;
  // }

  //if (pgm_read_byte(username) != 0)
  p[0] |= MQTT_CONN_USERNAMEFLAG;
  // if (pgm_read_byte(password) != 0)
  p[0] |= MQTT_CONN_PASSWORDFLAG;
  p++;

  p[0] = MQTT_CONN_KEEPALIVE >> 8;
  p++;
  p[0] = MQTT_CONN_KEEPALIVE & 0xFF;
  p++;

  if (MQTT_PROTOCOL_LEVEL == 3)
  {
    p = stringprint(p, clientid, 23); // Limit client ID to first 23 characters.
  }
  else
  {
    //if (pgm_read_byte(clientid) != 0)
    //{
    p = stringprint(p, clientid);
    // }
    // else
    // {
    //   p[0] = 0x0;
    //   p++;
    //   p[0] = 0x0;
    //   p++;
    //   //DEBUG_PRINTLN(F("SERVER GENERATING CLIENT ID"));
    // }
  }

  // if (will_topic && pgm_read_byte(will_topic) != 0)
  // {
  //   p = stringprint(p, will_topic);
  //   p = stringprint(p, will_payload);
  // }

  // if (pgm_read_byte(username) != 0)
  // {
  p = stringprint(p, username);
  // }
  //if (pgm_read_byte(password) != 0)
  //{
  p = stringprint(p, password);
  //}

  len = p - packet;

  packet[1] = len - 2; // don't include the 2 bytes of fixed header data
  //DEBUG_PRINTLN(F("MQTT connect packet:"));
  //DEBUG_PRINTBUFFER(buffer, len);
  return len;
}

// how much data we save in a subscription object
// and how many subscriptions we want to be able to track.
// #if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega328P__)
// #define MAXSUBSCRIPTIONS 5
// #define SUBSCRIPTIONDATALEN 20
// #else
// #define MAXSUBSCRIPTIONS 15
// #define SUBSCRIPTIONDATALEN 100
// #endif