#include "modem.h"

Sim800c *p_sim800c;
void rx_callback();
void timeout_cmd();
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

void Sim800c::send_msg(const char *msg)
{
  Sim800c::flush();
  Sim800c::module.printf(msg);
  Sim800c::module.printf("\r\n");
}

void Sim800c::send_break_msg(const char *msg)
{
  Sim800c::flush();
  Sim800c::module.printf(msg);
}

void Sim800c::flush()
{
  while (p_sim800c->module.readable())
  {
    p_sim800c->module.getc();
  }
}

void timeout_cmd()
{
  p_sim800c->timeout_flag = true;
}

void Sim800c::task()
{

  switch (s_state.estado_actual)
  {
  case inicio:
    Sim800c::send_msg("AT");
    strcpy(s_state.resp_esperada, "OK");
    s_state.estado_anterior = s_state.estado_actual;
    s_state.estado_actual = esperando_respuesta;
    s_state.estado_proximo = rm_eco;
    rxBuff.clear();
    timeout.attach(&timeout_cmd, 1.0);
    break;
  case rm_eco:
    Sim800c::send_msg("ATE0");
    s_state.estado_anterior = s_state.estado_actual;
    s_state.estado_actual = esperando_respuesta;
    s_state.estado_proximo = fecha_hora; //registro_red;
    strcpy(s_state.resp_esperada, "OK");
    timeout.attach(&timeout_cmd, 2.0);
    rxBuff.clear();
    break;
  case registro_red:
    send_msg("AT+CREG?");
    printf("[SIM800c] Regsitrando en RED...\r\n");
    s_state.estado_anterior = s_state.estado_actual;
    s_state.estado_actual = esperando_respuesta;
    s_state.estado_proximo = hora_red;
    strcpy(s_state.resp_esperada, "+CREG: 0,1\r\n\r\nOK");
    timeout.attach(&timeout_cmd, 2.0);
    rxBuff.clear();
    break;
  case fecha_hora:
    send_msg("AT+CCLK?");
    s_state.estado_anterior = s_state.estado_actual;
    s_state.estado_actual = esperando_respuesta;
    s_state.estado_proximo = no_op;
    strcpy(s_state.resp_esperada, "\r\n\r\nOK");
    timeout.attach(&timeout_cmd, 2.0);
    rxBuff.clear();
    break;
  case imei:
    send_msg("AT+GSN");
    s_state.estado_anterior = s_state.estado_actual;
    s_state.estado_actual = esperando_respuesta;
    s_state.estado_proximo = no_op;
    strcpy(s_state.resp_esperada, "OK");
    timeout.attach(&timeout_cmd, 2.0);
    rxBuff.clear();
    break;
  case hora_red:
    send_msg("AT+CLTS?");
    //Serial.println("hora de red automatica?");
    s_state.estado_anterior = s_state.estado_actual;
    s_state.estado_actual = esperando_respuesta;
    s_state.estado_proximo = http_config_1;
    strcpy(s_state.resp_esperada, "\r\n\r\nOK");
    timeout.attach(&timeout_cmd, 2.0);
    rxBuff.clear();
    break;
  case guardar_busqueda_hora_red:
    //esta funcion deberia ser llamada solo una vez si es
    //que ya no estab configurada de antes
    send_msg("AT+CLTS=1;&W");
    //Serial.println("seteando valor a registros del modem");
    s_state.estado_anterior = s_state.estado_actual;
    s_state.estado_actual = esperando_respuesta;
    s_state.estado_proximo = software_reset;
    strcpy(s_state.resp_esperada, "OK");
    timeout.attach(&timeout_cmd, 2.0);
    rxBuff.clear();
    break;
  case software_reset:
    send_msg("AT+CFUN=1,1");
    //delay(3000);
    //Serial.println("reiniciando modem");
    s_state.estado_anterior = s_state.estado_actual;
    s_state.estado_actual = esperando_respuesta;
    s_state.estado_proximo = inicio;
    strcpy(s_state.resp_esperada, "OK");
    timeout.attach(&timeout_cmd, 12.0);
    rxBuff.clear();
    break;

    /**** FIN INICIO DE MODEM ****************/

    /**** Comienzo configuracion DE MODEM***/
  case http_config_1:
    send_msg("AT+SAPBR=3,1,\"Contype\",\"GPRS\"");
    printf("Comienzo de config http");
    s_state.estado_anterior = s_state.estado_actual;
    s_state.estado_actual = esperando_respuesta;
    s_state.estado_proximo = http_config_2;
    strcpy(s_state.resp_esperada, "OK");
    timeout.attach(&timeout_cmd, 2.0);
    rxBuff.clear();
    break;
  case http_config_2:
    send_break_msg("AT+SAPBR=3,1,\"APN\",");
    send_msg(APN_NAME);
    printf("poniendo apn\n\r");
    s_state.estado_anterior = s_state.estado_actual;
    s_state.estado_actual = esperando_respuesta;
    s_state.estado_proximo = http_config_3;
    strcpy(s_state.resp_esperada, "OK");
    timeout.attach(&timeout_cmd, 2.0);
    rxBuff.clear();
    break;
  case http_config_3:
    send_msg("AT+SAPBR=1,1");
    s_state.estado_anterior = s_state.estado_actual;
    s_state.estado_actual = esperando_respuesta;
    s_state.estado_proximo = http_config_4;
    strcpy(s_state.resp_esperada, "OK");
    timeout.attach(&timeout_cmd, 2.0);
    rxBuff.clear();
    break;
  case http_config_4:
    send_msg("AT+SAPBR=2,1");
    printf("pregunto si tengo internet\n\r");
    s_state.estado_anterior = s_state.estado_actual;
    s_state.estado_actual = esperando_respuesta;
    s_state.estado_proximo = no_op;
    strcpy(s_state.resp_esperada, "OK");
    timeout.attach(&timeout_cmd, 12.0);
    rxBuff.clear();
    break;
  /**** FIN INICIO DE CONFIGURACION DE MODEM***/

  /**** Comienzo envio de datos***/
  case esperando_respuesta:
    //printf("[SIM800c] Esperando respuesta\r\n");
    if (p_sim800c->rxBuff.new_data)
    {
      /* agrego valor recibido por uart al buffer*/
      if (strstr((char *)rxBuff.buff, s_state.resp_esperada) != 0)
      {
        timeout.detach();
        s_state.estado_actual = procesar_respuesta;
      }
      else if (strstr((char *)rxBuff.buff, "ERROR") != 0)
      {
        printf("[SIM800c] ERROR CMD\r\n");
        timeout.detach();
        wait(1);
        error_modem = ERROR_MDM;

        count_intentos++;
        if (count_intentos >= MAX_INTENTOS_MDM)
        {
          s_state.estado_actual = error_mdm;
        }
        else
        {
          s_state.estado_actual = s_state.estado_anterior; //s_state.estado_anterior;
        }
      }
    }
    if (timeout_flag)
    {
      printf("timeout\n\r");
      timeout_flag = false;
      count_intentos++;
      if (count_intentos >= MAX_INTENTOS_MDM)
      {
        s_state.estado_actual = error_mdm;
        break;
      }
      else
      {
        s_state.estado_actual = s_state.estado_anterior;
      }
    }
    break;

  case procesar_respuesta:
    //printf("vamos bien\r\n");
    switch (s_state.estado_anterior)
    {
    case imei:
      char *p, *v;
      int i;
      printf("%s", rxBuff.buff);
      printf("IMEI obtenido: ");
      v = strstr((char *)(rxBuff.buff), "\r\n");
      v = v + 2;           //lo coloco en el primer caracter
      p = strstr(v, "\n"); //p esta en el utlimo salto de barra
      if (NULL != v)
      {
        i = 0;
        while (v < p)
        {
          Sim800c::m_imei[i++] = *(v++);
        }
        Sim800c::m_imei[i - 1] = '\0';
      }
      s_state.estado_actual = s_state.estado_proximo;
      if (callback != NULL)
        callback(); //llamo a funcion de callback y cargo hora
      printf("%s", Sim800c::m_imei);
      printf("\r\n");
      break;
    case fecha_hora:
      memset(Sim800c::timestamp, '\0', 30);
      printf("Hora obtenida: ");
      v = strstr((char *)(rxBuff.buff), "+CCLK: ");
      v = v + 7;                     //lo coloco en el primer caracter
      p = strstr((char *)(v), "\n"); //p esta en el utlimo "
      if (NULL != v)
      {
        i = 0;
        while (v < p)
        {
          Sim800c::timestamp[i++] = *(v++);
        }
        Sim800c::timestamp[i] = '\0';
      }
      s_state.estado_actual = s_state.estado_proximo;
      if (callback != NULL)
        callback(); //llamo a funcion de callback y cargo hora
      printf(Sim800c::timestamp);
      break;
    case hora_red:
      v = strstr((char *)(rxBuff.buff), "+CLTS: ");
      v = v + 7; //lo coloco en el primer caracter
      if ((char)*v == (char)'1')
      {
        //Serial.println("clts bien configurado");
        s_state.estado_actual = s_state.estado_proximo;
      }
      else
      {
        //Serial.println("clts mal configurado");
        s_state.estado_actual = guardar_busqueda_hora_red;
      }
      //s_state.estado_actual = s_state.estado_proximo;
      break;
    case rssi:
      v = strtok((char *)rxBuff.buff, ","); // aca apunto a la primera parte del mensaje (antes de la coma)
      if (NULL != (p = strstr(v, "+CSQ:")))
      {
        p = p + 6;
        //Serial.print(F("Calidad de Señal: "));
        m_rssi = atoi(p);
        if (m_rssi == 0)
        {
          m_rssi = -150;
        }
        else if (m_rssi == 1)
        {
          m_rssi = -111;
        }
        else if (m_rssi >= 2 && m_rssi <= 30)
        {
          m_rssi = 2 * m_rssi - 114;
        }
        else if (m_rssi >= 31 && m_rssi <= 98)
        {
          m_rssi = -52;
        }
        else
        {
          m_rssi = 99;
        }
        //Serial.println(m_rssi);
        if (callback != NULL)
          callback(); //llamo a funcion de callback
      }
      s_state.estado_actual = s_state.estado_proximo;
      break;
    case http_send_msg_7:
      p = strstr((char *)rxBuff.buff, ",");
      p++;                // me saco primer coma de arriba
      v = strstr(p, ","); // luego de esta coma esta el largo a leer
      largo_respuesta_http = atoi(v);
      //Serial.print("largo de mensaje a leer: ");
      //Serial.println(largo_respuesta_http);
      if (largo_respuesta_http > 0)
      {
        s_state.estado_actual = s_state.estado_proximo;
      }
      else
      {
        s_state.estado_actual = http_send_msg_9;
      }
      break;
    default:
      s_state.estado_actual = s_state.estado_proximo;
      break;
    }
    break;
  case error_mdm:
    break;
  default:
    break;
  }
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
  p_sim800c->rxBuff.new_data = true;
}