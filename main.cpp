#include "modem.h"
#include "mbed.h"

//#define MODEM_TRANSPARENT_MODE

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