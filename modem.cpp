#include "modem.h"

Sim800c *p_sim800c;
void rx_callback();

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

void Sim800c::begin(uint16_t baud)
{
  p_sim800c = this; // asigno instancia
  Sim800c::module.baud(baud);
  s_state.estado_actual = no_op;
  Sim800c::module.attach(&rx_callback, SerialBase::RxIrq);
}

Sim800c::~Sim800c()
{
}

void rx_callback()
{
  //_rdy = true;
  while (p_sim800c->module.readable())
  {
    p_sim800c->rxBuff.add_c((char)p_sim800c->module.getc());
  }
}