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

void Sim800c::write_msg(char *msg, uint16_t large)
{
  int largito = 0;
  for (int i = 0; i < large; i++)
  {
    Sim800c::module.putc((int)msg[i]);
    largito = i;
    printf("largito: %d\n\r", largito);
  }
  printf("largito: %d", largito);
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
  case en_regulador:
    m_pinReg = 1; //enable reg
    m_powKey = 0; //powKey en 1
    s_state.estado_anterior = s_state.estado_actual;
    s_state.estado_actual = wait_timeout;
    s_state.estado_proximo = pw_key_pulse_off;
    timeout.attach(&timeout_cmd, 0.3);
    break;
  case pw_key_pulse_off:
    m_powKey = 1; //pulse in 1
    s_state.estado_anterior = s_state.estado_actual;
    s_state.estado_actual = wait_timeout;
    s_state.estado_proximo = pw_key_pulse_on;
    timeout.attach(&timeout_cmd, 1.2); //1.5 seg
    break;
  case pw_key_pulse_on:
    m_powKey = 0; //pulse in 1
    s_state.estado_anterior = s_state.estado_actual;
    s_state.estado_actual = wait_timeout;
    s_state.estado_proximo = inicio;
    timeout.attach(&timeout_cmd, 3.2); //3.2 segun hoja de datos para q este ready seg
    break;
  case inicio:
    rxBuff.clear();
    Sim800c::send_msg("AT");
    strcpy(s_state.resp_esperada, "OK");
    s_state.estado_anterior = s_state.estado_actual;
    s_state.estado_actual = esperando_respuesta;
    s_state.estado_proximo = rm_eco;
    timeout.attach(&timeout_cmd, 1.0);
    break;
  case rm_eco:
    rxBuff.clear();
    Sim800c::send_msg("ATE0"); //ATE0
    s_state.estado_anterior = s_state.estado_actual;
    s_state.estado_actual = esperando_respuesta;
    s_state.estado_proximo = registro_red; //registro_red;
    strcpy(s_state.resp_esperada, "OK");
    timeout.attach(&timeout_cmd, 2.0);
    count_intentos = 0; //llevo contador a cero(por si anteriormente hubo error) ya preguntar por red no es un error en si.
    break;
  case registro_red:
    rxBuff.clear();
    send_msg("AT+CREG?");
    printf("[SIM800c] Regsitrando en RED...\r\n");
    s_state.estado_anterior = s_state.estado_actual;
    s_state.estado_actual = esperando_respuesta;
    s_state.estado_proximo = hora_red;
    strcpy(s_state.resp_esperada, "+CREG: 0,1\r\n\r\nOK");
    timeout.attach(&timeout_cmd, 2.0);
    break;
  case fecha_hora:
    rxBuff.clear();
    send_msg("AT+CCLK?");
    printf("mandando AT+CCLK\n\r");
    s_state.estado_anterior = s_state.estado_actual;
    s_state.estado_actual = esperando_respuesta;
    s_state.estado_proximo = finish_task;
    strcpy(s_state.resp_esperada, "\r\n\r\nOK");
    timeout.attach(&timeout_cmd, 2.0);
    break;
  case imei:
    rxBuff.clear();
    send_msg("AT+GSN");
    s_state.estado_anterior = s_state.estado_actual;
    s_state.estado_actual = esperando_respuesta;
    s_state.estado_proximo = finish_task;
    strcpy(s_state.resp_esperada, "OK");
    timeout.attach(&timeout_cmd, 2.0);
    break;
  case rssi:
    rxBuff.clear();
    send_msg("AT+CSQ");
    //Serial.println("obteniendo rssi");
    s_state.estado_anterior = s_state.estado_actual;
    s_state.estado_actual = esperando_respuesta;
    s_state.estado_proximo = finish_task;
    strcpy(s_state.resp_esperada, "\r\n\r\nOK");
    timeout.attach(&timeout_cmd, 2.0);
    break;
  case hora_red:
    rxBuff.clear();
    send_msg("AT+CLTS?");
    //Serial.println("hora de red automatica?");
    s_state.estado_anterior = s_state.estado_actual;
    s_state.estado_actual = esperando_respuesta;
    if (mode_high_level_fuction)
    {
      s_state.estado_proximo = test_gprs_connection; //http_config_1; viejo
    }
    else
    {
      s_state.estado_proximo = cgatt;
    }
    strcpy(s_state.resp_esperada, "\r\n\r\nOK");
    timeout.attach(&timeout_cmd, 2.0);
    break;
  case guardar_busqueda_hora_red:
    //esta funcion deberia ser llamada solo una vez si es
    //que ya no estab configurada de antes
    rxBuff.clear();
    send_msg("AT+CLTS=1;&W");
    //Serial.println("seteando valor a registros del modem");
    s_state.estado_anterior = s_state.estado_actual;
    s_state.estado_actual = esperando_respuesta;
    s_state.estado_proximo = software_reset;
    strcpy(s_state.resp_esperada, "OK");
    timeout.attach(&timeout_cmd, 2.0);
    break;
  case software_reset:
    rxBuff.clear();
    send_msg("AT+CFUN=1,1");
    //delay(3000);
    //Serial.println("reiniciando modem");
    s_state.estado_anterior = s_state.estado_actual;
    s_state.estado_actual = esperando_respuesta;
    s_state.estado_proximo = inicio;
    strcpy(s_state.resp_esperada, "OK");
    timeout.attach(&timeout_cmd, 12.0);
    break;

  /**** FIN INICIO DE MODEM ****************/

  /**** Comienzo configuracion DE MODEM***/
  case http_config_1:
    rxBuff.clear();
    send_msg("AT+SAPBR=3,1,\"Contype\",\"GPRS\"");
    printf("Comienzo de config http");
    s_state.estado_anterior = s_state.estado_actual;
    s_state.estado_actual = esperando_respuesta;
    s_state.estado_proximo = http_config_2;
    strcpy(s_state.resp_esperada, "OK");
    timeout.attach(&timeout_cmd, 2.0);
    break;
  case http_config_2:
    rxBuff.clear();
    send_break_msg("AT+SAPBR=3,1,\"APN\",");
    send_msg(APN_NAME);
    printf("poniendo apn\n\r");
    s_state.estado_anterior = s_state.estado_actual;
    s_state.estado_actual = esperando_respuesta;
    s_state.estado_proximo = http_config_3;
    strcpy(s_state.resp_esperada, "OK");
    timeout.attach(&timeout_cmd, 2.0);
    break;
  case http_config_3:
    rxBuff.clear();
    send_msg("AT+SAPBR=1,1");
    s_state.estado_anterior = s_state.estado_actual;
    s_state.estado_actual = esperando_respuesta;
    s_state.estado_proximo = test_gprs_connection;
    strcpy(s_state.resp_esperada, "OK");
    timeout.attach(&timeout_cmd, 2.0);
    break;
  case test_gprs_connection:
    rxBuff.clear();
    send_msg("AT+SAPBR=2,1");
    printf("pregunto si tengo internet\n\r");
    s_state.estado_anterior = s_state.estado_actual;
    s_state.estado_actual = esperando_respuesta;
    s_state.estado_proximo = finish_task; //finish_task;
    strcpy(s_state.resp_esperada, "OK");
    timeout.attach(&timeout_cmd, 12.0);
    break;
  /**** FIN INICIO DE CONFIGURACION DE MODEM***/

  /**** Comienzo envio de datos***/
  case http_send_msg_1:
    rxBuff.clear();
    send_msg("AT+HTTPINIT");
    printf("comienzo iniciando http");
    s_state.estado_anterior = s_state.estado_actual;
    s_state.estado_actual = esperando_respuesta;
    s_state.estado_proximo = http_send_msg_2;
    strcpy(s_state.resp_esperada, "OK");
    timeout.attach(&timeout_cmd, 2.0);
    break;
  case http_send_msg_2:
    rxBuff.clear();
    send_break_msg("AT+HTTPPARA=\"URL\",");
    send_break_msg(URL_BASE);
    send_break_msg("save-data");
    send_msg("\"");
    //Serial.println("seteando url");
    s_state.estado_anterior = s_state.estado_actual;
    s_state.estado_actual = esperando_respuesta;
    s_state.estado_proximo = http_send_msg_3;
    strcpy(s_state.resp_esperada, "OK");
    timeout.attach(&timeout_cmd, 2.0);
    break;
  case http_send_msg_3:
    rxBuff.clear();
    send_msg("AT+HTTPPARA=\"CONTENT\",\"application/x-www-form-urlencoded\"");
    //Serial.println("seteando content");
    s_state.estado_anterior = s_state.estado_actual;
    s_state.estado_actual = esperando_respuesta;
    s_state.estado_proximo = http_send_msg_4;
    strcpy(s_state.resp_esperada, "OK");
    timeout.attach(&timeout_cmd, 2.0);
    break;
  case http_send_msg_4:
    rxBuff.clear();
    send_msg("AT+HTTPPARA=\"CID\",1");
    //Serial.println("seteando CID");
    s_state.estado_anterior = s_state.estado_actual;
    s_state.estado_actual = esperando_respuesta;
    s_state.estado_proximo = http_send_msg_5;
    strcpy(s_state.resp_esperada, "OK");
    timeout.attach(&timeout_cmd, 2.0);
    break;
  case http_send_msg_5:
    char largo[5];
    rxBuff.clear();
    send_break_msg("AT+HTTPDATA="); // partir mensaje
    sprintf(largo, "%d", strlen(message.payload));
    send_break_msg(largo);
    send_msg(",10000"); //timeout for internal parameter of modem
    // Serial.print("mensaje a enviar: ");
    // Serial.println(payload);
    // Serial.println(largo);
    s_state.estado_anterior = s_state.estado_actual;
    s_state.estado_actual = esperando_respuesta;
    s_state.estado_proximo = http_send_msg_6;
    strcpy(s_state.resp_esperada, "DOWNLOAD");
    timeout.attach(&timeout_cmd, 7.0);
    break;
  case http_send_msg_6:
    rxBuff.clear();
    send_msg(message.payload); // 1 es post, 0 es get
    printf("encolando data\r\n");
    s_state.estado_anterior = s_state.estado_actual;
    s_state.estado_actual = esperando_respuesta;
    s_state.estado_proximo = http_send_msg_7;
    strcpy(s_state.resp_esperada, "OK");
    timeout.attach(&timeout_cmd, 2.0);
    break;
  case http_send_msg_7:
    rxBuff.clear();
    send_msg("AT+HTTPACTION=1"); // 1 es post, 0 es get
    //Serial.println("seteando metodo POST");
    s_state.estado_anterior = s_state.estado_actual;
    s_state.estado_actual = esperando_respuesta;
    s_state.estado_proximo = http_send_msg_8;
    strcpy(s_state.resp_esperada, "+HTTPACTION: 1,");
    timeout.attach(&timeout_cmd, 2.0);
    break;
  case http_send_msg_8:
    memset(largo, '\0', 5);
    sprintf(largo, "%d", largo_respuesta_http);
    rxBuff.clear();
    send_break_msg("AT+HTTPREAD=0,"); // mensaje partido
    send_msg(largo);
    //Serial.println("seteando metodo POST");
    s_state.estado_anterior = s_state.estado_actual;
    s_state.estado_actual = esperando_respuesta;
    s_state.estado_proximo = http_send_msg_9;
    strcpy(s_state.resp_esperada, "OK");
    timeout.attach(&timeout_cmd, 2.0);
    break;
  case http_send_msg_9:
    rxBuff.clear();
    send_msg("AT+HTTPTERM");
    s_state.estado_anterior = s_state.estado_actual;
    s_state.estado_actual = esperando_respuesta;
    s_state.estado_proximo = finish_task;
    strcpy(s_state.resp_esperada, "OK");
    timeout.attach(&timeout_cmd, 12.0);
    printf("Cerrando conexion");
    break;
  /**** FIN ENVIO DE DATOS***/

  /*** TCP COMMANDS ***/
  case cgatt:
    // gprs attached?
    rxBuff.clear();
    printf("comienzo de tcp config\n\r");
    send_msg("AT+CGATT?");
    s_state.estado_anterior = s_state.estado_actual;
    s_state.estado_actual = esperando_respuesta;
    s_state.estado_proximo = tcp_cipshut;
    strcpy(s_state.resp_esperada, "+CGATT: 1");
    timeout.attach(&timeout_cmd, 5.0);
    break;
  case tcp_cipshut:
    // cierro conexiones anteriores
    rxBuff.clear();
    send_msg("AT+CIPSHUT");
    s_state.estado_anterior = s_state.estado_actual;
    s_state.estado_actual = esperando_respuesta;
    s_state.estado_proximo = tcp_cipstatus;
    strcpy(s_state.resp_esperada, "SHUT OK");
    timeout.attach(&timeout_cmd, 15.0);
    break;
  case tcp_cipstatus:
    rxBuff.clear();
    send_msg("AT+CIPSTATUS");
    s_state.estado_anterior = s_state.estado_actual;
    s_state.estado_actual = esperando_respuesta;
    s_state.estado_proximo = tcp_cipmux;
    strcpy(s_state.resp_esperada, "STATE: IP INITIAL");
    timeout.attach(&timeout_cmd, 2.0);
    break;
    //no permito conexiones multiples
  case tcp_cipmux:
    rxBuff.clear();
    send_msg("AT+CIPMUX=0");
    s_state.estado_anterior = s_state.estado_actual;
    s_state.estado_actual = esperando_respuesta;
    s_state.estado_proximo = tcp_ciprxget;
    strcpy(s_state.resp_esperada, "OK");
    timeout.attach(&timeout_cmd, 2.0);
    break;
  case tcp_ciprxget: // manuallly fetch data
    rxBuff.clear();
    printf("rxget\n\r");
    send_msg("AT+CIPRXGET=1");
    s_state.estado_anterior = s_state.estado_actual;
    s_state.estado_actual = esperando_respuesta;
    s_state.estado_proximo = tcp_cstt;
    strcpy(s_state.resp_esperada, "OK");
    timeout.attach(&timeout_cmd, 2.0);
    break;
  case tcp_cstt:
    // configuro apn
    rxBuff.clear();
    printf("cstt\n\r");
    send_break_msg("AT+CSTT=");
    send_break_msg(APN_NAME);
    send_msg(",\"\",\"\"");
    //send_msg("AT+CSTT=\"antel.lte\",\"\",\"\"");
    s_state.estado_anterior = s_state.estado_actual;
    s_state.estado_actual = esperando_respuesta;
    s_state.estado_proximo = tcp_ciicr;
    strcpy(s_state.resp_esperada, "OK");
    timeout.attach(&timeout_cmd, 5.0);
    break;
    //wireless setup
  case tcp_ciicr:
    printf("ciicr\n\r");
    rxBuff.clear();
    send_msg("AT+CIICR");
    s_state.estado_anterior = s_state.estado_actual;
    s_state.estado_actual = esperando_respuesta;
    s_state.estado_proximo = tcp_cifsr;
    strcpy(s_state.resp_esperada, "OK");
    timeout.attach(&timeout_cmd, 15.0);
    break;
    //get IP
  case tcp_cifsr:
    printf("ciicr\n\r");
    rxBuff.clear();
    send_msg("AT+CIFSR");
    s_state.estado_anterior = s_state.estado_actual;
    s_state.estado_actual = esperando_respuesta;
    s_state.estado_proximo = tcp_cipstart;
    strcpy(s_state.resp_esperada, ".");
    timeout.attach(&timeout_cmd, 5.0);
    break;
  case tcp_cipstart:
    printf("Conectando TCP...\n\r");
    rxBuff.clear();
    send_break_msg("AT+CIPSTART=\"TCP\",");
    send_break_msg(MQTT_SERVER);
    send_break_msg(",");
    send_msg(MQTT_PORT);
    s_state.estado_anterior = s_state.estado_actual;
    s_state.estado_actual = esperando_respuesta;
    s_state.estado_proximo = finish_task;
    strcpy(s_state.resp_esperada, "CONNECT OK");
    timeout.attach(&timeout_cmd, 8.0);
    break;

    /**TCP SEND COMMANDS**/
    //TODO: agregar caso en el que la respuesta no es connect OK, ej de respuesta CLOSED

  case tcp_cipstatus_connected:
    rxBuff.clear();
    printf("status\n\r");
    send_msg("AT+CIPSTATUS");
    s_state.estado_anterior = s_state.estado_actual;
    s_state.estado_actual = esperando_respuesta;
    s_state.estado_proximo = tcp_cipsend;
    strcpy(s_state.resp_esperada, "STATE: CONNECT OK");
    timeout.attach(&timeout_cmd, 2.0);
    break;
  case tcp_cipsend:
    memset(largo, '\0', 5);
    rxBuff.clear();
    sprintf(largo, "%d", message.largo);
    printf("largo msg a enviar: %s", largo);
    send_break_msg("AT+CIPSEND=");
    send_msg(largo);
    s_state.estado_anterior = s_state.estado_actual;
    s_state.estado_actual = esperando_respuesta;
    s_state.estado_proximo = tcp_cipsending;
    strcpy(s_state.resp_esperada, ">");
    timeout.attach(&timeout_cmd, 2.0);
    break;
  case tcp_cipsending:
    rxBuff.clear();
    printf("enviando msg TCP...\n\r");
    write_msg(message.payload, message.largo);
    //send_msg(message.payload);
    s_state.estado_anterior = s_state.estado_actual;
    s_state.estado_actual = esperando_respuesta;
    s_state.estado_proximo = tcp_available; //finish_task
    strcpy(s_state.resp_esperada, "SEND OK");
    timeout.attach(&timeout_cmd, 2.0);
    break;

    /**READ TCP COMMANDS**/
  case tcp_available:
    rxBuff.clear();
    printf("Preguntando si hay datos a leer\n\r");
    send_msg("AT+CIPRXGET=4");
    s_state.estado_anterior = s_state.estado_actual;
    s_state.estado_actual = esperando_respuesta;
    s_state.estado_proximo = tcp_read;
    strcpy(s_state.resp_esperada, "\r\n\r\nOK"); //+CIPRXGET: 4,
    timeout.attach(&timeout_cmd, 3.0);
    break;
  case tcp_read:
    memset(largo, '\0', 5);
    rxBuff.clear();
    sprintf(largo, "%d", largo_respuesta_http);
    printf("leyendo msg TCP...\n\r");
    send_break_msg("AT+CIPRXGET=2,");
    send_msg(largo);
    s_state.estado_anterior = s_state.estado_actual;
    s_state.estado_actual = esperando_respuesta;
    s_state.estado_proximo = finish_task;
    strcpy(s_state.resp_esperada, "+CIPRXGET: 2,");
    timeout.attach(&timeout_cmd, 4.0);
    break;

    //TCP close
  case tcp_cipclose:
    rxBuff.clear();
    send_break_msg("AT+CIPCLOSE");
    s_state.estado_anterior = s_state.estado_actual;
    s_state.estado_actual = esperando_respuesta;
    s_state.estado_proximo = finish_task;
    strcpy(s_state.resp_esperada, "OK");
    timeout.attach(&timeout_cmd, 2.0);
    break;
  /**GSM LOCATION***/
  case gsm_loc:
    rxBuff.clear();
    send_msg("AT+CIPGSMLOC=1,1");
    s_state.estado_anterior = s_state.estado_actual;
    s_state.estado_actual = esperando_respuesta;
    s_state.estado_proximo = finish_task;
    strcpy(s_state.resp_esperada, "OK");
    timeout.attach(&timeout_cmd, 5.0);
    break;

  /** PROCESAMIENTO **/
  case esperando_respuesta:

    //printf("[SIM800c] Esperando respuesta\r\n");
    if (rxBuff.new_data)
    {
      //printf("datos\n\r");
      printf((char *)rxBuff.buff);
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
      printf("[MODEM] TimeOut en CMD\n\r");

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
      //printf("%s", rxBuff.buff);
      //printf("IMEI obtenido: ");
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
      //printf("%s", Sim800c::m_imei);
      //printf("\r\n");
      break;
    case fecha_hora:
      memset(Sim800c::timestamp, '\0', 30);
      //printf("Hora obtenida: ");
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
      //printf(Sim800c::timestamp);
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
    case test_gprs_connection:
      if (strstr((char *)rxBuff.buff, "+SAPBR: 1,1") != 0)
      {
        printf("[MODEM_DEBUG] Conexion a internet establecida.\n\r");
        s_state.estado_actual = finish_task;
      }
      else
      {
        s_state.estado_actual = http_config_1;
      }
      break;
    case tcp_available:
      printf("tcp available\n\r");
      p = strstr((char *)rxBuff.buff, "+CIPRXGET: 4,");
      //printf(p);
      p = p + 13;                  // me saco primer coma de arriba
      v = strstr(p, "\r\n\r\nOK"); // luego de esta coma esta el largo a leer
      v = '\0';
      message_in.largo = atoi(p);
      printf("largo a leer: %d \r\n", message_in.largo);

      if (message_in.largo > 0)
      {
        s_state.estado_actual = s_state.estado_proximo;
      }
      else
      {
        s_state.estado_actual = finish_task;
      }
      break;
    case tcp_read:
      // while ()
      break;
    case gsm_loc:
      //+CIPGSMLOC: 0,-56.190739,-34.903355,2019/03/20,18:00:40
      p = strtok((char *)rxBuff.buff, ",");
      if (!p)
        break;
      p = strtok(NULL, ",");
      lon = atof(p);
      p = strtok(NULL, ",");
      lat = atof(p);
      // char buf_aux[10];
      // sprintf(buf_aux, "%f", lon); //4 is mininum width, 6 is precision
      // printf("%s\n\r", buf_aux);
      // sprintf(buf_aux, "%f", lat);
      // printf("%s\n\r", buf_aux);
      s_state.estado_actual = s_state.estado_proximo;
      if (callback != NULL)
        callback();
      break;
    default:
      s_state.estado_actual = s_state.estado_proximo;
      break;
    }
    break;
  case wait_timeout:
    if (timeout_flag)
    {
      timeout_flag = false;
      s_state.estado_actual = s_state.estado_proximo;
    }
    break;
  case finish_task:
    error_modem = OK_MDM;
    s_state.estado_actual = no_op; //no_op;
    status_handler();
    printf("mision completa\n\r");
    break;
  case no_op:
    //wait(1);
    break;
  case error_mdm:
    error_modem = ERROR_MDM;
    printf("mision Fracasada\n\r");
    wait(1);
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

/********* TCP FUNCTIONS  ************************************/

// boolean Adafruit_FONA::TCPconnect(char *server, uint16_t port)
// {
//   flushInput();

//   // close all old connections
//   if (!sendCheckReply(F("AT+CIPSHUT"), F("SHUT OK"), 20000))
//     return false;

//   // single connection at a time
//   if (!sendCheckReply(F("AT+CIPMUX=0"), ok_reply))
//     return false;

//   // manually read data
//   if (!sendCheckReply(F("AT+CIPRXGET=1"), ok_reply))
//     return false;

//   DEBUG_PRINT(F("AT+CIPSTART=\"TCP\",\""));
//   DEBUG_PRINT(server);
//   DEBUG_PRINT(F("\",\""));
//   DEBUG_PRINT(port);
//   DEBUG_PRINTLN(F("\""));

//   mySerial->print(F("AT+CIPSTART=\"TCP\",\""));
//   mySerial->print(server);
//   mySerial->print(F("\",\""));
//   mySerial->print(port);
//   mySerial->println(F("\""));

//   if (!expectReply(ok_reply))
//     return false;
//   if (!expectReply(F("CONNECT OK")))
//     return false;

//   // looks like it was a success (?)
//   return true;
// }

// boolean Adafruit_FONA::TCPclose(void)
// {
//   return sendCheckReply(F("AT+CIPCLOSE"), ok_reply);
// }

// boolean Adafruit_FONA::TCPconnected(void)
// {
//   if (!sendCheckReply(F("AT+CIPSTATUS"), ok_reply, 100))
//     return false;
//   readline(100);

//   DEBUG_PRINT(F("\t<--- "));
//   DEBUG_PRINTLN(replybuffer);

//   return (strcmp(replybuffer, "STATE: CONNECT OK") == 0);
// }

// boolean Adafruit_FONA::TCPsend(char *packet, uint8_t len)
// {

//   DEBUG_PRINT(F("AT+CIPSEND="));
//   DEBUG_PRINTLN(len);
// #ifdef ADAFRUIT_FONA_DEBUG
//   for (uint16_t i = 0; i < len; i++)
//   {
//     DEBUG_PRINT(F(" 0x"));
//     DEBUG_PRINT(packet[i], HEX);
//   }
// #endif
//   DEBUG_PRINTLN();

//   mySerial->print(F("AT+CIPSEND="));
//   mySerial->println(len);
//   readline();

//   DEBUG_PRINT(F("\t<--- "));
//   DEBUG_PRINTLN(replybuffer);

//   if (replybuffer[0] != '>')
//     return false;

//   mySerial->write(packet, len);
//   readline(3000); // wait up to 3 seconds to send the data

//   DEBUG_PRINT(F("\t<--- "));
//   DEBUG_PRINTLN(replybuffer);

//   return (strcmp(replybuffer, "SEND OK") == 0);
// }

// uint16_t Adafruit_FONA::TCPavailable(void)
// {
//   uint16_t avail;

//   if (!sendParseReply(F("AT+CIPRXGET=4"), F("+CIPRXGET: 4,"), &avail, ',', 0))
//     return false;

//   DEBUG_PRINT(avail);
//   DEBUG_PRINTLN(F(" bytes available"));

//   return avail;
// }

// uint16_t Adafruit_FONA::TCPread(uint8_t *buff, uint8_t len)
// {
//   uint16_t avail;

//   mySerial->print(F("AT+CIPRXGET=2,"));
//   mySerial->println(len);
//   readline();
//   if (!parseReply(F("+CIPRXGET: 2,"), &avail, ',', 0))
//     return false;

//   readRaw(avail);

// #ifdef ADAFRUIT_FONA_DEBUG
//   DEBUG_PRINT(avail);
//   DEBUG_PRINTLN(F(" bytes read"));
//   for (uint8_t i = 0; i < avail; i++)
//   {
//     DEBUG_PRINT(F(" 0x"));
//     DEBUG_PRINT(replybuffer[i], HEX);
//   }
//   DEBUG_PRINTLN();
// #endif

//   memcpy(buff, replybuffer, avail);

//   return avail;
// }
