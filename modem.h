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

#endif
