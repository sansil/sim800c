#include "modem.h"
#include "mbed.h"

Serial pc(USBTX, USBRX); // tx, rx
//RawSerial modem(PA_9, PA_10);
//SerialBase::TxIrq
DigitalOut led1(LED1);
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

  pc.printf("Starting sim800c demo.\n\r");
  simcom.begin(9600);
  flipper.attach(&flip, 2.0); // setup flipper to call flip after 2 seconds
  // modem.baud(9600);
  // modem.attach(&rx_callback, SerialBase::RxIrq);
  simcom.set_request(modem_begin, NULL);
  while (1)
  {
    // if (pc.readable())
    // {
    //   modem.putc(pc.getc());
    // }
    simcom.task();
    if (timeo)
    {
      timeo = false;
      printf("timeou en main \r\n");
    }
    // if (simcom.rxBuff.new_data)
    // {
    //   simcom.rxBuff.new_data = false;
    //   printf("%s", simcom.rxBuff.buff);
    //   data_rdy = false;
    // }
  }
}