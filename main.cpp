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

Sim800c simcom(PA_9, PA_10);
Timeout flipper;
// void rx_callback()
// {
//   data_rdy = true;
//   while (modem.readable())
//   {
//     modem_buff.add_c((char)modem.getc());
//   }
// }

volatile bool timeo = false;
void flip()
{
  timeo = !timeo;
}
int main()
{
  led2 = 1;
  led3 = 1;
  wait(2);
  led3 = 0;
  pc.printf("Starting sim800c demo .\n\r");

  //simcom.begin(9600);
  //flipper.attach(&flip, 2.0); // setup flipper to call flip after 2 seconds
#ifdef MODEM_TRANSPARENT_MODE
  modem.baud(9600);
#else
  simcom.begin(9600);
  flipper.attach(&flip, 2.0); // setup flipper to call flip after 2 seconds
  simcom.set_request(modem_begin, NULL);
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