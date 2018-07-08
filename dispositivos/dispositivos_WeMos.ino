/*************************************** LIBRERIAS ************************************************/
//librerías necesarias para las conexiones WiFi y las peticiones http
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <ESP8266HTTPClient.h>
//librería necesaria para implementar la actualizacion via OTA
#include <ArduinoOTA.h>
//Librerias necesarias para fijar la hora en la placa
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <Time.h>
#include <TimeLib.h>
#include <Timezone.h>
//Librerias necesarias para el sensor de temperatura y humedad
#include <DHT.h>
#include <DHT_U.h>
//Libreria necesaria para el actuador persiana
#include <Ticker.h>
//Libreria necesaria para el sistema de ficheros
#include "FS.h"
/*************************************** CONSTANTES ************************************************/

/*CONSTANTES COMUNES*/

/* Entradas/Salidas Arduino */
#define PIN_RELE D6
#define PIN_PULSADOR_ILUMINACION D8
#define PIN_INTERRUPTOR_CONFIGURACION D5
#define PIN_MOTOR_SUBIR D6
#define PIN_MOTOR_BAJAR D7
#define PIN_PULSADOR_SUBIR_PERSIANA D1
#define PIN_PULSADOR_BAJAR_PERSIANA D8
#define PIN_SENSOR_TEMPERATURA_HUMEDAD D2

/*Tipos de actuadores o dispositivos */
#define ACTUADOR_ILUMINACION 1 //Dispositivo que mediante un pulsador activa un relé, encendiendo así una bombilla. Además se puede controlar de forma remota con peticiones get al servidor web que funciona en la placa
#define ACTUADOR_PERSIANA 2 //Dispositivo que controla una persiana con dos interruptores de manera fisica para poder subir y bajar y también de manera remota mediante peticiones get al servidor web.
#define SENSOR_TEMPERATURA_HUMEDAD 3 //Dispositivo que mide temperatura y humedad en una habitación
#define ACTUADOR_ENCHUFE 4 //Dispositivo que permite controlar un enchufe de manera remota mediante peticiones get al servidor web funcionando en la placa


/* Definición del significado pulsado/ no pulsado en el montaje */
#define PULSADO HIGH
#define NO_PULSADO LOW

/*Definicion de los posibles estados del Relé*/
#define ACTIVADO HIGH
#define NO_ACTIVADO LOW

/*Tamaño de la variable logDispositivoReciente*/
#define TAM_LOG 1000

#define RETRASO_REBOTE 50 // Margen definido para controlar el rebote que puede producirse en un pulsador

/*CONSTANTES ESPECIFICAS A CADA DISPOSITIVO*/

/* Actuador de iluminación*/

/*Actuador de enchufe*/

/*Actuador de persiana*/
#define TIEMPO_PARAR 20  // Tiempo, en segundos, a partir del cual se para el motor

// Estados del motor de la persiana:
#define SUBIR 1
#define BAJAR 2
#define PARAR 3

/*Sensor de temperatura*/
#define DHTTYPE DHT11
#define TIEMPO_DESCANSO 10 //segundos de duracion del sleep mode
/*************************************** VARIABLES GLOBALES ************************************************/
/*VARIABLES COMUNES*/

bool estadoWifi;//Controla el estado de la conexión wifi
String horaArranque;//Contiene la hora de arranque del dispositivo
ESP8266WebServer servidorWeb(80);//Servidor web en el puerto 80
String logDispositivoReciente;//Variable que contiene los logs del dispositivo recientes
String logDispositivoAntiguo;//Variable que contiene los logs del dispositivo cuando la variable reciente alcanza un tamaño predefinido
bool estadoInterruptor;
HTTPClient httpCliente;
/*VARIABLES ESPECIFICAS*/

/*Variables actuador Iluminacion*/
int estadoPulsadorIluminacion;
int ultimoEstadoPulsadorIluminacion = NO_PULSADO;
unsigned long tiempoUltimoReboteIluminacion = 0;

/*Variables actuador persiana*/
int estadoPersiana = PARAR;  // Estado actual del motor
int estadoAnteriorPersiana = PARAR; // para depuración, para controlar los cambios de estado
Ticker temporizador; // Objeto que controla el temporizador de parada
int ultimoEstadoPulsadorSubir = NO_PULSADO;
int ultimoEstadoPulsadorBajar = NO_PULSADO;
int estadoPulsadorSubir;
int estadoPulsadorBajar;
unsigned long tiempoUltimoRebotePulsadorSubir = 0;
unsigned long tiempoUltimoRebotePulsadorBajar = 0;

/*Variables sensor temperatura*/
DHT dht(PIN_SENSOR_TEMPERATURA_HUMEDAD, DHTTYPE);

/* Variables configuracion dispositivo */
IPAddress ipDispositivo;
IPAddress mascara;
IPAddress gateway;
char ssid [30];
char password [30];
IPAddress ipControlador;
int tipoDispositivo;
char  nombreDispositivo [30];
/*Variables comunes al actuador iluminacion y actuador enchufe*/
int estadoRele = NO_ACTIVADO;

void setup() {
  Serial.begin(115200);
  pinMode(PIN_INTERRUPTOR_CONFIGURACION, INPUT_PULLUP);
  pinMode(BUILTIN_LED, OUTPUT);
  SPIFFS.begin();
  /*******************  MODO CONFIGURACION  *********************/
  //Si el interruptor está activado o no existe fichero de configuracion, entra el modo configuracion
  Serial.println("MODO CONFIGURACION");
  if (digitalRead(PIN_INTERRUPTOR_CONFIGURACION) == HIGH || !SPIFFS.exists("/fichero_configuracion.txt")) {
    estadoInterruptor = true;
    if (!SPIFFS.exists("/fichero_configuracion.txt")) {
      Serial.println("No existe el fichero de configuracion");
      crearFicheroConfiguracion();
    }
    estadoWifi = true;
    leerFicheroConfiguracion();
    ota_setup();//Configuracion del metodo OTA para actualizar el codigo en la placa
    configuracionPuntoDeAcceso();//Configura una red WiFi abierta para configurar mediante un formulario el funcionamiento de la placa
  }
  /*******************  MODO OPERACIONES  *********************/
  else {
    estadoInterruptor = false;
    Serial.println("MODO OPERACIONES");
    Serial.println("Existe el fichero de configuracion");
    leerFicheroConfiguracion();//Lee el fichero de configuracion ya existente
    ota_setup();//Configuracion del metodo OTA para actualizar el codigo en la placa
    estadoWifi = configuracionWifi(); //Conexion a la red WiFi
    //Se ejecuta si la conexión Wifi ha tenido éxito
    if (estadoWifi == true) {
      fijarHora();//Obtencion de hora mediante peticion a servidor ntp y que después queda fijada en la placa para su posterior uso
      configuracionServidorWeb();//Configuracion y arranque del Servidor Web
      incluirLog("El dispositivo ha arrancado correctamente");
      String info = "El dispositivo ";
      info += nombreDispositivo;
      info += " ha arrancado correctamente";
      enviarLog(info);
    }
    else {
      Serial.println("El dispositivo no ha arrancado correctamente");
    }
    //Segun la variable tipoDispositivo la placa se configura para que su funcionamiento sea el de uno de los dispositivos disponibles
    switch (tipoDispositivo) {
      case ACTUADOR_ILUMINACION:
        actuadorIluminacion_setup();
        break;
      case ACTUADOR_ENCHUFE:
        actuadorEnchufe_setup();
        break;
      case ACTUADOR_PERSIANA:
        actuadorPersiana_setup();
        break;
      case SENSOR_TEMPERATURA_HUMEDAD:
        sensorTemperaturaYHumedad_setup();
        break;
      default:
        Serial.println("No está bien configurado, vaya al modo configuracion");
    }
  }
}
void loop() {
  if (estadoWifi == true) {
    ArduinoOTA.handle();//Lanza el manejador para controlar las actualizaciones OTA
    servidorWeb.handleClient();//Lanza el manejador que controla el servidor web y las peticiones recibidas al mismo
  }
  if (!estadoInterruptor) {
    //Segun la variable tipoDispositivo se define el funcionamiento de la placa como uno de los dispositivos disponibles
    switch (tipoDispositivo) {
      case ACTUADOR_ILUMINACION:
        actuadorIluminacion_loop();
        break;
      case ACTUADOR_ENCHUFE:
        actuadorEnchufe_loop();
        break;
      case ACTUADOR_PERSIANA:
        actuadorPersiana_loop();
        break;
      case SENSOR_TEMPERATURA_HUMEDAD:
        sensorTemperaturaYHumedad_loop();
        break;
    }
  }
}
/*************FUNCIONES DEL DISPOSITIVO ACTUADOR DE ILUMINACION ************/
void actuadorIluminacion_setup() {
  pinMode(PIN_PULSADOR_ILUMINACION, INPUT);//Pin del pulsador configurado como entrada
  pinMode (PIN_RELE, OUTPUT);//Pin del relé configurado como salida
  digitalWrite(PIN_RELE, LOW);//estado inicial del relé
  incluirLog("Setup del actuador Iluminacion completada");
}
void actuadorIluminacion_loop() {
  int lecturaPulsador = digitalRead(PIN_PULSADOR_ILUMINACION);
  if (lecturaPulsador != ultimoEstadoPulsadorIluminacion) //Cambio en el Pulsador
  {
    tiempoUltimoReboteIluminacion = millis(); //Actualiza tiempoUltimoReboteIluminacion con el tiempo actual
  }
  //Aqui se comprueba que la duracion de la pulsacion supera el umbral de rebote impuesto, eliminando los rebotes en el pulsador
  if ((millis() - tiempoUltimoReboteIluminacion) > RETRASO_REBOTE) {
    //Si el Pulsador ha sufrido un cambio y si el estado es pulsado, se producirá a un cambio del estado del relé
    if (lecturaPulsador != estadoPulsadorIluminacion) {
      estadoPulsadorIluminacion = lecturaPulsador;
      if (estadoPulsadorIluminacion == PULSADO) {
        estadoRele = !estadoRele; //Si el Pulsador es accionado, se cambia el estado del relé para asi apagarlo tras tenerlo encendido y vicecersa.
        //Si el nuevo estado del relé es ACTIVADO, el pin recibe tensión HIGH
        if ( estadoRele == ACTIVADO) {
          incluirLog("Rele activado mediante pulsador en el actuador iluminacion");
          digitalWrite(PIN_RELE, HIGH);
          digitalWrite(BUILTIN_LED, HIGH);
          enviarEstadoIluminacion("Encendida");
        }
        //Si el nuevo estado del relé es NO_ACTIVADO, el pin recibe tensión LOW
        else if (estadoRele == NO_ACTIVADO) {
          incluirLog("Rele desactivado mediante pulsador en el actuador iluminacion");
          digitalWrite(PIN_RELE, LOW);
          digitalWrite(BUILTIN_LED, LOW);
          enviarEstadoIluminacion("Apagada");
        }
      }
    }
  }
  ultimoEstadoPulsadorIluminacion = lecturaPulsador;//El estado del pulsador es almacenado para la siguiente ejecución
}
void enviarEstadoIluminacion(String estadoIluminacion) {
  String ip = "http://";
  ip += convertirIpAString(ipControlador);
  ip += "/guardarEstadoActuadorIluminacion.php";
  httpCliente.begin(ip);
  httpCliente.addHeader("Content-Type", "application/x-www-form-urlencoded");
  String peticion = "estadoIluminacion=";
  peticion += estadoIluminacion;
  peticion += "&nombreDispositivo=";
  peticion += nombreDispositivo;
  httpCliente.POST(peticion);
  //httpCliente.writeToStream(&Serial); //Muestra por pantalla la respuesta del controlador
  httpCliente.end();
}
String paginaIluminacion_html() {
  String pagina = "<html>\n<head><meta http-equiv='Content-Type' content='text/html; charset=utf-8'/>";
  pagina += "\n<title>Casa Domotica</title>\n</head>\n<body>";
  pagina += "<div style='text-align:center;'>";
  pagina += "<h2>Dispositivo iluminacion</h2>";
  pagina += "Estado iluminacion = ";
  pagina += estadoRele == LOW ? "APAGADA" : "ENCENDIDA";
  pagina += "<br/>";
  pagina += "<button onClick=location.href='./operaciones?iluminacion=encender'>ENCENDER</button>";
  pagina += "<button onClick=location.href='./operaciones?iluminacion=apagar'>APAGAR</button>";
  pagina += "<br/><br/>";
  pagina += "<a href='./operaciones?iluminacion=actualizar'";
  pagina += ">Refrescar</a>";
  pagina += "</div>\n</body></html>";
  return pagina;
}
void controlIluminacion_http() {
  String valorIluminacion = servidorWeb.arg("iluminacion");//El argumento valorIluminacion incluido en la peticion web es obtenido y almacenado
  //Si el valor del parametro coincide con "encender", se activa el relé
  if ( valorIluminacion == "encender") {
    digitalWrite(PIN_RELE, HIGH);
    digitalWrite(BUILTIN_LED, HIGH);
    estadoRele = ACTIVADO;
    enviarEstadoIluminacion("Encendida");
    incluirLog("Iluminacion encendida mediante peticion get");
    servidorWeb.send ( 200, "text/html", paginaIluminacion_html() );
  }
  //Si el valor del parametro coincide con "apagar", se desactiva el relé
  else if ( valorIluminacion == "apagar") {
    digitalWrite(PIN_RELE, LOW);
    digitalWrite(BUILTIN_LED, LOW);
    estadoRele = NO_ACTIVADO;
    enviarEstadoIluminacion("Apagada");
    incluirLog("Iluminacion apagada mediante peticion get");
    servidorWeb.send ( 200, "text/html", paginaIluminacion_html() );
  }
  //Si el parametro lleva otro valor que no sea "encender" o "apagar", se envia la pagina destinada a manejar la iluminacion
  else {
    servidorWeb.send ( 200, "text/html", paginaIluminacion_html() );
  }
}
/*************FUNCIONES DEL DISPOSITIVO ACTUADOR DE ENCHUFE ************/
void actuadorEnchufe_setup() {
  pinMode (PIN_RELE, OUTPUT);
  //estado inicial del relé
  digitalWrite(PIN_RELE, LOW);
  incluirLog("Setup del actuador enchufe completada");
}
void actuadorEnchufe_loop() {

}
void enviarEstadoEnchufe(String estadoEnchufe) {
  String ip = "http://";
  ip += convertirIpAString(ipControlador);
  ip += "/guardarEstadoActuadorEnchufe.php";
  httpCliente.begin(ip);
  httpCliente.addHeader("Content-Type", "application/x-www-form-urlencoded");
  String peticion = "estadoEnchufe=";
  peticion += estadoEnchufe;
  peticion += "&nombreDispositivo=";
  peticion += nombreDispositivo;
  httpCliente.POST(peticion);
  //httpCliente.writeToStream(&Serial); //Muestra por pantalla la respuesta del controlador
  httpCliente.end();
}
String paginaEnchufe_html() {
  String pagina = "<html>\n<head><meta http-equiv='Content-Type' content='text/html; charset=utf-8'/>";
  pagina += "\n<title>Casa Domotica</title>\n</head>\n<body>";
  pagina += "<div style='text-align:center;'>";
  pagina += "<h2>Actuador enchufe</h2>";
  pagina += "Estado enchufe = ";
  pagina += estadoRele == LOW ? "Desconectado" : "Conectado";
  pagina += "<br/>";
  pagina += "<button onClick=location.href='./operaciones?enchufe=conectar'>CONECTAR</button>";
  pagina += "<button onClick=location.href='./operaciones?enchufe=desconectar'>DESCONECTAR</button>";
  pagina += "<br/><br/>";
  pagina += "<a href='./operaciones?enchufe=actualizar'";
  pagina += ">Refrescar</a>";
  pagina += "</div>\n</body></html>";
  return pagina;
}
void controlEnchufe_http() {
  String valorEnchufe = servidorWeb.arg("enchufe");//El argumento enchufe incluido en la peticion web es obtenido y almacenado
  //Si el valor del parametro coincide con "conectar", se activa el relé
  if ( valorEnchufe == "conectar") {
    digitalWrite(PIN_RELE, HIGH);
    digitalWrite(BUILTIN_LED, HIGH);
    estadoRele = ACTIVADO;
    enviarEstadoEnchufe("Conectado");
    incluirLog("Peticion get con el argumento enchufe recibido en el servidor");
    servidorWeb.send ( 200, "text/html", paginaEnchufe_html() );
  }
  //Si el valor del parametro coincide con "desconectar", se desactiva el relé
  else if ( valorEnchufe == "desconectar") {
    digitalWrite(PIN_RELE, LOW);
    digitalWrite(BUILTIN_LED, LOW);
    estadoRele = NO_ACTIVADO;
    enviarEstadoEnchufe("Desconectado");
    incluirLog("Enchufe desconectado mediante peticion get");
    servidorWeb.send ( 200, "text/html", paginaEnchufe_html() );
  }
  //Si el parametro lleva otro valor que no sea "conectar" o "desconectar", se envia la pagina destinada a manejar el enchufe
  else {
    servidorWeb.send ( 200, "text/html", paginaEnchufe_html() );
  }
}

/*************FUNCIONES DEL DISPOSITIVO ACTUADOR DE PERSIANA ************/
void actuadorPersiana_setup() {
  pinMode( PIN_MOTOR_SUBIR, OUTPUT);
  pinMode( PIN_MOTOR_BAJAR, OUTPUT);
  pinMode(PIN_PULSADOR_SUBIR_PERSIANA, INPUT);
  pinMode(PIN_PULSADOR_BAJAR_PERSIANA, INPUT);
  digitalWrite(PIN_MOTOR_SUBIR, NO_PULSADO);
  digitalWrite(PIN_MOTOR_BAJAR, NO_PULSADO);
  incluirLog("Setup del actuador persiana completada");
}
void actuadorPersiana_loop() {
  controlPulsadores(PIN_PULSADOR_SUBIR_PERSIANA);
  controlPulsadores(PIN_PULSADOR_BAJAR_PERSIANA);
  // Mostrar mensaje cuando haya cambios de estado en la persiana
  if (estadoAnteriorPersiana != estadoPersiana) {
    estadoAnteriorPersiana = estadoPersiana;
    switch (estadoPersiana) {
      case SUBIR: Serial.println("Estado SUBIR"); break;
      case BAJAR: Serial.println("Estado BAJAR"); break;
      case PARAR: Serial.println("Estado PARAR"); break;
    }
  }
}
void controlPulsadores(int const pulsador) {
  if (pulsador == PIN_PULSADOR_SUBIR_PERSIANA) {
    int lecturaPulsadorSubir = digitalRead(PIN_PULSADOR_SUBIR_PERSIANA);
    if (lecturaPulsadorSubir != ultimoEstadoPulsadorSubir) //Cambio en el Pulsador
    {
      tiempoUltimoRebotePulsadorSubir = millis(); //Actualiza tiempoUltimoRebote con el tiempo actual
    }
    //Aqui se comprueba que la duracion de la pulsacion supera el umbral de rebote impuesto, eliminando los rebotes en el pulsador
    if ((millis() - tiempoUltimoRebotePulsadorSubir) > RETRASO_REBOTE) {
      //Si el pulsador ha sufrido un cambio y si el estado es pulsado, se producirá un cambio del estado del relé
      if (lecturaPulsadorSubir != estadoPulsadorSubir) {
        estadoPulsadorSubir = lecturaPulsadorSubir;
        if (estadoPulsadorSubir == PULSADO) {
          Serial.println("Llamada a la funcion controlMotor");
          controlMotor(SUBIR);
        }
      }
    }
    ultimoEstadoPulsadorSubir = lecturaPulsadorSubir;//El estado del pulsador es almacenado para la siguiente ejecución
  }
  else if (pulsador == PIN_PULSADOR_BAJAR_PERSIANA) {
    int lecturaPulsadorBajar = digitalRead(PIN_PULSADOR_BAJAR_PERSIANA);
    if (lecturaPulsadorBajar != ultimoEstadoPulsadorBajar) //Cambio en el Pulsador
    {
      tiempoUltimoRebotePulsadorBajar = millis(); //Actualiza tiempoUltimoRebote con el tiempo actual
    }
    //Aqui se comprueba que la duracion de la pulsacion supera el umbral de rebote impuesto, eliminando los rebotes en el pulsador
    if ((millis() - tiempoUltimoRebotePulsadorBajar) > RETRASO_REBOTE) {
      //Si el pulsador ha sufrido un cambio y si el estado es pulsado, se producirá a un cambio del estado del relé
      if (lecturaPulsadorBajar != estadoPulsadorBajar) {
        estadoPulsadorBajar = lecturaPulsadorBajar;
        if (estadoPulsadorBajar == PULSADO) {
          Serial.println("Llamada a la funcion controlMotor");
          controlMotor(BAJAR);
        }
      }
    }
    ultimoEstadoPulsadorBajar = lecturaPulsadorBajar;//El estado del pulsador es almacenado para la siguiente ejecución
  }
}
//Controla el motor de la persiana en funcion de la orden que se le envie
void controlMotor(const int operacion) {
  switch (operacion) {
    case SUBIR:
      switch (estadoPersiana) {
        case SUBIR:   // Si ya estaba subiendo
          digitalWrite(PIN_MOTOR_SUBIR, LOW);  // Parar motor de subida
          estadoPersiana = PARAR;
          break;
        case BAJAR:   // Si estaba bajando
          digitalWrite(PIN_MOTOR_BAJAR, LOW);  // Parar motor de bajada
          digitalWrite(PIN_MOTOR_SUBIR, HIGH);  // Subir motor
          temporizador.once(TIEMPO_PARAR, controlMotor, PARAR);  // Ejecutar la función controlMotor(PARAR) después de TIEMPO_PARAR segundos. De esta forma no dejamos los relés activados enternamente
          estadoPersiana = SUBIR;
          break;
        case PARAR:   // Si estaba parado
          digitalWrite(PIN_MOTOR_SUBIR, HIGH);  // Subir motor
          temporizador.once(TIEMPO_PARAR, controlMotor, PARAR);  // Ejecutar la función controlMotor(PARAR) después de TIEMPO_PARAR segundos. De esta forma no dejamos los relés activados enternamente
          estadoPersiana = SUBIR;
          break;
      }
      break;
    case BAJAR:
      switch (estadoPersiana) {
        case BAJAR:   // Si ya estaba bajando
          digitalWrite(PIN_MOTOR_BAJAR, LOW);  // Parar motor de bajada
          estadoPersiana = PARAR;
          break;
        case SUBIR:   // Si estaba subiendo
          digitalWrite(PIN_MOTOR_SUBIR, LOW);  // Parar motor de subida
          digitalWrite(PIN_MOTOR_BAJAR, HIGH);  // Bajar motor
          temporizador.once(TIEMPO_PARAR, controlMotor, PARAR);  // Ejecutar la función controlMotor(PARAR) después de TIEMPO_PARAR segundos. De esta forma no dejamos los relés encendidos enternamente
          estadoPersiana = BAJAR;
          break;
        case PARAR:   // Si estaba parado
          digitalWrite(PIN_MOTOR_BAJAR, HIGH);  // Bajar motor
          temporizador.once(TIEMPO_PARAR, controlMotor, PARAR);  // Ejecutar la función controlMotor(PARAR) después de TIEMPO_PARAR segundos. De esta forma no dejamos los relés encendidos enternamente
          estadoPersiana = BAJAR;
          break;
      }
      break;
    case PARAR:
      switch (estadoPersiana) {
        case BAJAR:   // Si estaba bajando
          digitalWrite(PIN_MOTOR_BAJAR, LOW);  // Parar motor de bajada
          break;
        case SUBIR:   // Si estaba subiendo
          digitalWrite(PIN_MOTOR_SUBIR, LOW);  // Parar motor de subida
          break;
        case PARAR:   // Si estaba parado
          // No hacer nada especial
          break;
      }
      estadoPersiana = PARAR;
      break;
  }
  if (estadoPersiana == PARAR)
    digitalWrite(BUILTIN_LED, HIGH);  // Apagar led interno
  else
    digitalWrite(BUILTIN_LED, LOW);  // Encender led interno
}
void controlPersiana_http() {
  String valorPersiana = servidorWeb.arg("persiana");//El argumento persiana incluido en la peticion web es obtenido y almacenado
  //Si el valor del parametro coincide con "subir", llama a la funcion controlMotor con la accion subir como parametro
  if ( valorPersiana == "subir") {
    controlMotor(SUBIR);
    incluirLog("Persiana subida mediante peticion get");
    servidorWeb.send ( 200, "text/html", paginaPersiana_html() );
  }
  //Si el valor del parametro coincide con "apagar", se desactiva el relé
  else if ( valorPersiana == "bajar") {
    controlMotor(BAJAR);
    incluirLog("Persiana bajada mediante peticion get");
    servidorWeb.send ( 200, "text/html", paginaPersiana_html() );
  }
  else if (valorPersiana == "parar") {
    controlMotor(PARAR);
    incluirLog("Persiana parada mediante peticion get");
    servidorWeb.send ( 200, "text/html", paginaPersiana_html() );
  }
  //Si el parametro lleva otro valor que no sea "subir", "bajar" o "parar", se envia la pagina destinada a manejar la persiana
  else {
    servidorWeb.send ( 200, "text/html", paginaPersiana_html() );
  }
}
String paginaPersiana_html() {
  String pagina = "<html>\n<head><meta http-equiv='Content-Type' content='text/html; charset=utf-8'/>";
  pagina += "\n<title>Casa Domotica</title>\n</head>\n<body>";
  pagina += "<div style='text-align:center;'>";
  pagina += "<h2>Actuador persiana</h2>";
  pagina += "Estado persiana = ";
  switch (estadoPersiana) {
    case SUBIR:
      pagina += "Subida";
      break;
    case BAJAR:
      pagina += "Bajada";
      break;
    case PARAR:
      pagina += "Parada";
      break;
  }
  pagina += "<br/>";
  pagina += "<button onClick=location.href='./operaciones?persiana=subir'>SUBIR</button>";
  pagina += "<button onClick=location.href='./operaciones?persiana=bajar'>BAJAR</button>";
  pagina += "<button onClick=location.href='./operaciones?persiana=parar'>PARAR</button>";
  pagina += "<br/><br/>";
  pagina += "<a href='./operaciones?persiana=actualizar'";
  pagina += ">Refrescar</a>";
  pagina += "</div>\n</body></html>";
  return pagina;
}
/*************FUNCIONES DEL DISPOSITIVO SENSOR DE TEMPERATURA Y HUMEDAD************/
void sensorTemperaturaYHumedad_setup() {
  delay(2000);
  medirTemperatura();
  medirHumedad();
  // Deep sleep
  Serial.println("ESP8266 en sleep mode");
  ESP.deepSleep(TIEMPO_DESCANSO * 1000000);
}
void medirTemperatura() {
  float t = dht.readTemperature();
  if (isnan(t)) {
    Serial.println("Error al leer del sensor DTH!");
  }
  else {
    enviarDatosMedidos(String(t), "temperatura");
  }
}
void medirHumedad() {
  float h = dht.readHumidity();
  if (isnan(h)) {
    Serial.println("Error al leer del sensor DTH!");
  }
  else {
    enviarDatosMedidos(String(h), "humedad");
  }
}
//Al incluir la funcion deepSleep, esta efectua un reset cada vez que se despierta, debiendo estar en el setup. Debido a esto, nunca va a entrar a la funcion loop, por lo que todo el codigo debe estar en el setup, que es lo que lee cada vez que se despierta antes de volver a dormirse
void sensorTemperaturaYHumedad_loop() {
}

/*************FUNCIONES COMUNES************/
bool configuracionWifi() {
  String ipConectada = "";
  bool resultado;
  WiFi.mode(WIFI_STA);//Indica el modo en el que se configura la conexion WiFi. Modo estacion
  WiFi.begin(ssid, password);
  int i = 0;

  // Espera hasta que hay conexion
  while (WiFi.status() != WL_CONNECTED && i++ < 30) {
    delay(500);
    Serial.print(".");
  }
  //Si el bucle acaba antes de 15 segundos, es que la conexión wifi se ha producido
  if (i < 31) {
    //La IP del gateway y la del servidor DNS es la misma
    WiFi.config(ipDispositivo, gateway,  mascara, gateway);//Fija una ip estática para el servidor web
    Serial.println("Conectado a red WiFi: ");
    Serial.println(ssid);
    Serial.println("Direccion IP: ");
    Serial.println(WiFi.localIP());
    resultado = true;
  }
  else {
    Serial.println("Han expirado los 15 segundos para establecer la conexion WiFi");
    resultado = false;
  }
  return resultado;
}
void configuracionPuntoDeAcceso() {
  const char *ssidAcceso = "ConfigDispositivoDomotico";
  IPAddress wifiIp(192, 168, 168, 168);
  IPAddress wifiNet(255, 255, 255, 0);
  IPAddress wifiGW(192, 168, 168, 168);
  Serial.println(ssidAcceso);
  WiFi.softAP("ConfigDispositivoDomotico");
  WiFi.softAPConfig(wifiIp, wifiGW, wifiNet);//Indica una ip fija para el servidor web que sirve el formulario de configuracion
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("Dirección IP del servidor: ");
  Serial.println(myIP);
  configuracionServidorWeb();//Configura el funcionamiento del servidor web
  Serial.println("Servidor arrancado");
}
void configuracionServidorWeb() {
  /*Segun la URL de la peticion un manejador u otro se hace cargo*/
  servidorWeb.on("/", handle_Root);
  servidorWeb.on("/operaciones", handle_Operaciones);
  servidorWeb.on("/log", handle_Log);
  servidorWeb.on("/configuracion", handle_Configuracion);
  servidorWeb.begin();//Arranca el servidor web
}
//Obtiene la hora de un servidor ntp y la fija en la placa
void fijarHora() {
  WiFiUDP ntpUDP;
  time_t local;
  String ipNtp = convertirIpAString(ipControlador);
  char servidorNtp [30];
  ipNtp.toCharArray(servidorNtp, 30);
  NTPClient timeClient(ntpUDP, servidorNtp, 60 * 60, 60 * 1000);
  //Horario Europa Central
  TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 60};     // Hora de Verano de Europa Central CEST es UTC +2
  TimeChangeRule CET = {"CET ", Last, Sun, Oct, 3, 180};      // Hora Estandar de Europa Central CET es UTC +1
  Timezone CE(CEST, CET);
  // Actualizar el cliente NTP y obtener la marca de tiempo UNIX UTC
  timeClient.update();
  unsigned long utc =  timeClient.getEpochTime();

  //Convierte la marca de tiempo UTC a hora local mediante las reglas de tiempo fijadas para Europa Central.
  local = CE.toLocal(utc);
  setTime(local);//Fija en el reloj de la placa la hora obtenida mediante NTP
  horaArranque = obtenerHora();

}
//Hace una peticion http a una pagina php del controlador del sistema domotico que almacena los datos
void enviarDatosMedidos(String dato, String tipoSensor) {
  Serial.print("Enviar datos medidos  de tipo: ");
  Serial.print(tipoSensor);
  Serial.print(" Valor: ");
  Serial.println(dato);
  String ip = "http://";
  ip += convertirIpAString(ipControlador);
  ip += "/guardarDatosSensor.php";
  httpCliente.begin(ip);
  httpCliente.addHeader("Content-Type", "application/x-www-form-urlencoded");
  String peticion = "dispositivo=";
  peticion += nombreDispositivo;
  peticion += "&";
  peticion += tipoSensor;
  peticion +=  "=";
  peticion += dato;
  Serial.print("Esta es la peticion POST: ");
  Serial.println(peticion);
  int codigoHttp = httpCliente.POST(peticion);
  if (codigoHttp != 200) {
    Serial.println("No se ha producido la peticion, volvemos a intentarlo");
    codigoHttp = httpCliente.POST(peticion);
  }
  httpCliente.writeToStream(&Serial); //Muestra por pantalla la respuesta del controlador
  httpCliente.end();
}
//Introduce en la variable destinada para ello el log que pasamos por parametro.
void incluirLog(String info) {
  //Solo si hay conexion wifi se incluye el log
  if (estadoWifi == true) {
    //Este algoritmo controla el aumento indefinido de las variables que contienen los logs del dispositivo para que no crezcan de manera interminable y terminen con la memoria del mismo.
    if (logDispositivoReciente.length() > TAM_LOG) {
      logDispositivoAntiguo = logDispositivoReciente;
      logDispositivoReciente = "";
    }
    //añadimos el log a la variable reciente
    logDispositivoReciente += "[";
    logDispositivoReciente += obtenerHora();
    logDispositivoReciente += "]: ";
    logDispositivoReciente += info;
    logDispositivoReciente += "\n";
  }
}
//Envia la informacion pasada como parametro al controlador, que lo almacena en un fichero
void enviarLog(String log) {
  String ip = "http://";
  ip += convertirIpAString(ipControlador);
  ip += "/grabarLog.php";
  httpCliente.begin(ip);
  httpCliente.addHeader("Content-Type", "application/x-www-form-urlencoded");
  String peticion = "dispositivo=";
  peticion += nombreDispositivo;
  peticion += "&info=";
  peticion += log;
  httpCliente.POST(peticion);
  //httpCliente.writeToStream(&Serial); //Muestra por pantalla la respuesta del controlador
  httpCliente.end();
}
//Recupera la hora en el momento que es pedida y le da formato
String obtenerHora() {
  time_t t = now();//Recupera la hora del reloj de la placa

  //Contruye el formato fecha y hora para cada log
  String hora = String(day(t));
  hora += "/";
  hora += String(month(t));
  hora += "/";
  hora += String(year(t));
  hora += " ";
  hora += dosDigitos(hour(t));
  hora += ":";
  hora += dosDigitos(minute(t));
  hora += ":";
  hora += dosDigitos(second(t));
  return hora;
}
String dosDigitos(int numero) {
  String numeroString = "";
  if (numero >= 0 && numero < 10)
  {
    numeroString += '0';
  }

  numeroString += String(numero);
  return numeroString;
}
//Devuelve la pagina principal del servidor en el modo operaciones
String paginaPrincipal_html() {
  String pagina = "<html>\n<head><meta http-equiv='Content-Type' content='text/html; charset=utf-8'/>";
  pagina += "\n<title>Casa Domotica</title>\n</head>\n<body><h1>PAGINA PRINCIPAL</h1>";
  switch (tipoDispositivo) {
    case ACTUADOR_ILUMINACION:
      pagina += "<a href='./operaciones?iluminacion=mostrar'";
      pagina += ">Actuador iluminacion</a>";
      pagina += "<br>";
      break;
    case ACTUADOR_ENCHUFE:
      pagina += "<a href='./operaciones?enchufe=mostrar'";
      pagina += ">Actuador enchufe</a>";
      pagina += "<br>";
      break;
    case ACTUADOR_PERSIANA:
      pagina += "<a href='./operaciones?persiana=mostrar'";
      pagina += ">Actuador persiana</a>";
      pagina += "<br>";
      break;
    case SENSOR_TEMPERATURA_HUMEDAD:
      break;
  }
  pagina += "<a href='./operaciones?reset=1'";
  pagina += ">Resetear</a>";
  pagina += "</body></html>";
  return pagina;
}
//Devuelve la pagina con los logs almacenados
String paginaLogs_html() {
  String pagina = "<html>\n<head><meta http-equiv='Content-Type' content='text/html; charset=utf-8'/>";
  pagina += "\n<title>Casa Domotica</title>\n</head>\n<body><h1>Logs del dispositivo</h1>";
  pagina += "<h2>Hora de arranque del dispositivo: ";
  pagina += horaArranque;
  pagina += "</h2>";
  pagina += logDispositivoAntiguo;
  pagina += logDispositivoReciente;
  pagina += "</body></html>";
  return pagina;
}
//Devuelve la pagina con el formulario que permite al usuario configurar el funcionamiento y la conexion de la placa
String obtenerFormularioConfiguracion_html() {
  String pagina = "<html>\n<head><meta http-equiv='Content-Type' content='text/html; charset=utf-8'/>";
  pagina += "\n<title>Casa Domotica</title>\n";

  //funcion javascript que controla que todos los campos estén rellenados
  pagina += "<script type='text/javascript'>";
  pagina += "function validarFormulario() {";
  pagina += "var tipoDispositivo = document.getElementById('tipoDispositivo').selectedIndex;";
  pagina += "var nombreDispositivo = document.getElementById('nombreDispositivo').value;";
  pagina += "var ipDispositivo = document.getElementById('ipDispositivo').value;";
  pagina += "var mascara = document.getElementById('mascara').value;";
  pagina += "var gateway = document.getElementById('gateway').value;";
  pagina += "var ssid = document.getElementById('ssid').value;";
  pagina += "var pass = document.getElementById('pwd').value;";
  pagina += "var ipControlador = document.getElementById('ipControlador').value;";
  pagina += "if(tipoDispositivo == null){alert('[ERROR] Debe escoger un valor para el tipo de Dispositivo'); return false;}";
  pagina += "else if(nombreDispositivo == null || nombreDispositivo.length == 0){alert('[ERROR] Debe introducir un nombre para el dispositivo'); return false;}";
  pagina += "else if(ipDispositivo == null || ipDispositivo.length == 0){alert('[ERROR] Debe introducir una IP para el dispositivo'); return false;}";
  pagina += "else if(mascara == null || mascara.length == 0){alert('[ERROR] Debe introducir una IP para la mascara de la red'); return false;}";
  pagina += "else if(gateway == null || gateway.length == 0){alert('[ERROR] Debe introducir una IP para el gateway de la red'); return false;}";
  pagina += "else if(ssid == null || ssid.length == 0){alert('[ERROR] Debe introducir el nombre de su red WiFi'); return false;}";
  pagina += "else if(pass == null || pass.length == 0){alert('[ERROR] Debe introducir la contraseña de su red WiFi'); return false;}";
  pagina += "else if(ipControlador == null || ipControlador.length == 0){alert('[ERROR] Debe introducir la IP del controlador'); return false;}";
  pagina += "return true;}";
  pagina += "</script>";

  pagina += "</head>\n<body><h1>FORMULARIO DE CONFIGURACION</h1>";
  pagina += "<form action='/configuracion?opcion=guardar' method='post' onsubmit='return validarFormulario()'>";
  pagina += "<style type='text/css'>";
  pagina += ".tg  {border-collapse:collapse;border-spacing:0;border-color:#999;}";
  pagina += ".tg td{font-family:Arial, sans-serif;font-size:14px;padding:10px 5px;border-style:solid;border-width:1px;overflow:hidden;word-break:normal;border-color:#999;color:#444;background-color:#F7FDFA;}";
  pagina += ".tg th{font-family:Arial, sans-serif;font-size:14px;font-weight:normal;padding:10px 5px;border-style:solid;border-width:1px;overflow:hidden;word-break:normal;border-color:#999;color:#fff;background-color:#26ADE4;}";
  pagina += ".tg .tg-baqh{text-align:center;vertical-align:top}";
  pagina += ".tg .tg-lqy6{text-align:right;vertical-align:top}";
  pagina += ".tg .tg-yw4l{vertical-align:top}";
  pagina += "</style>";

  pagina += "<table class='tg'>";
  //Fila primera
  pagina += "<tr>";
  pagina += "<th class='tg-baqh'>Nombre</th><th class='tg-yw4l'>Valor</th><th class='tg-yw4l'>Descripción</th>";
  pagina += "</tr>";

  //Fila segunda
  pagina += "<tr>";
  //Primera columna
  pagina += "<td class='tg-yw4l'>";
  pagina += "<label for='tipoDispositivo'>Tipo de dispositivo</label>";
  pagina += "</td>";
  //Segunda columna
  pagina += "<td class='tg-yw4l'>";
  pagina += "<select id='tipoDispositivo' name='tipoDispositivo'>";
  if (tipoDispositivo == 0) {
    pagina += "<option value='' selected>- selecciona -</option>";
  }
  pagina += "<option value='1' ";
  pagina += obtenerDispositivoSeleccionado(tipoDispositivo, 1);
  pagina += ">ACTUADOR ILUMINACION</option> <option value='2'";
  pagina += obtenerDispositivoSeleccionado(tipoDispositivo, 2);
  pagina += ">ACTUADOR PERSIANA</option> <option value='3'";
  pagina += obtenerDispositivoSeleccionado(tipoDispositivo, 3);
  pagina += ">SENSOR DE TEMPERATURA Y HUMEDAD</option> <option value='4' ";
  pagina += obtenerDispositivoSeleccionado(tipoDispositivo, 4);
  pagina += ">ACTUADOR ENCHUFE</option></select>";
  pagina += "</td>";

  //tercera columna
  pagina += "<td class='tg-yw4l'>";
  pagina += "Elija el comportamiento de este dispositivo";
  pagina += "</td>";
  pagina += "</tr>";

  //tercera fila
  pagina += "<tr>";
  //primera columna
  pagina += "<td class='tg-yw4l'>";
  pagina += "<label for='nombreDispositivo'> Nombre del dispositivo: </label>";
  pagina += "</td>";
  //segunda columna
  pagina += "<td class='tg-yw4l'>";
  pagina += "<input type='text' id='nombreDispositivo' name='nombreDispositivo' value='";
  pagina += nombreDispositivo;
  pagina += "'/>";
  pagina += "</td>";
  //tercera columna
  pagina += "<td class='tg-yw4l'>";
  pagina += "Nombre que quiere darle al dispositivo, sin espacios, tildes o ñ. Por ejemplo: Iluminacion_Salon";
  pagina += "</td>";
  pagina += "</tr>";

  //cuarta fila
  pagina += "<tr>";
  //primera columna
  pagina += "<td class='tg-yw4l'>";
  pagina += "<label for='ipDispositivo'> IP del dispositivo: </label>";
  pagina += "</td>";
  //segunda columna
  pagina += "<td class='tg-yw4l'>";
  pagina += "<input type='text' id='ipDispositivo' name='ipDispositivo' value='";
  pagina += convertirIpAString(ipDispositivo);
  pagina += "'/>";
  pagina += "</td>";
  //tercera columna
  pagina += "<td class='tg-yw4l'>";
  pagina += "Es la IP que tendrá el dispositivo para acceder a él dentro de la red local. Cada dispositivo debe tener una IP distinta. Debe ser una del rango de su red que no esté usada por otro dispositivo de la red. Por ejemplo 192.168.1.100";
  pagina += "</td>";
  pagina += "</tr>";

  //quinta fila
  pagina += "<tr>";
  //primera columna
  pagina += "<td class='tg-yw4l'>";
  pagina += "<label for='mascara'> Máscara de la red: </label>";
  pagina += "</td>";
  //segunda columna
  pagina += "<td class='tg-yw4l'>";
  pagina += "<input type='text' id='mascara' name='mascara' value='";
  pagina += convertirIpAString(mascara);
  pagina += "'/>";
  pagina += "</td>";
  //tercera columna
  pagina += "<td class='tg-yw4l'>";
  pagina += "Normalmente suele ser el número 255.255.255.0";
  pagina += "</td>";
  pagina += "</tr>";

  //sexta fila
  pagina += "<tr>";
  //primera columna
  pagina += "<td class='tg-yw4l'>";
  pagina += "<label for='gateway'> Router de la red: </label>";
  pagina += "</td>";
  //segunda columna
  pagina += "<td class='tg-yw4l'>";
  pagina += "<input type='text' id='gateway' name='gateway' value='";
  pagina += convertirIpAString(gateway);
  pagina += "'/>";
  pagina += "</td>";
  //tercera columna
  pagina += "<td class='tg-yw4l'>";
  pagina += "Normalmente suele ser una IP que termina en 1, por ejemplo 192.168.1.1";
  pagina += "</td>";
  pagina += "</tr>";

  //septima fila
  pagina += "<tr>";
  //primera columna
  pagina += "<td class='tg-yw4l'>";
  pagina += "<label for='ssid'> SSID de la red:</label>";
  pagina += "</td>";
  //segunda columna
  pagina += "<td class='tg-yw4l'>";
  pagina += "<input type='text' id='ssid' name='ssid' value='";
  pagina += ssid;
  pagina += "'/>";
  pagina += "</td>";
  //tercera columna
  pagina += "<td class='tg-yw4l'>";
  pagina += "Nombre que tiene la red WiFi a la que va a conectar este dispositivo";
  pagina += "</td>";
  pagina += "</tr>";

  //octava fila
  pagina += "<tr>";
  //primera columna
  pagina += "<td class='tg-yw4l'>";
  pagina += "<label for='pwd'> Contraseña de la red: </label>";
  pagina += "</td>";
  //segunda columna
  pagina += "<td class='tg-yw4l'>";
  pagina += "<input type='password' id='pwd' name='psw' value='";
  pagina += password;
  pagina += "'/>";
  pagina += "</td>";
  //tercera columna
  pagina += "<td class='tg-yw4l'>";
  pagina += "Contraseña de su red WiFi";
  pagina += "</td>";
  pagina += "</tr>";

  //novena fila
  pagina += "<tr>";
  //primera columna
  pagina += "<td class='tg-yw4l'>";
  pagina += "<label for='ipControlador'> IP del controlador: </label>";
  pagina += "</td>";
  //segunda columna
  pagina += "<td class='tg-yw4l'>";
  pagina += "<input type='text' id='ipControlador' name='ipControlador' value='";
  pagina += convertirIpAString(ipControlador);
  pagina += "'/>";
  pagina += "</td>";
  //tercera columna
  pagina += "<td class='tg-yw4l'>";
  pagina += "Dirección IP que tenga asociada el controlador del sistema domótico dentro de la red doméstica";
  pagina += "</td>";
  pagina += "</tr>";

  pagina += "</table>";
  pagina += "<div class='button'> <button type='submit'> Enviar</button></div>";
  pagina += "</form></body></html>";
  return pagina;
}
//Devuelve la pagina que avisa al usuario de como proceder para entrar al modo operaciones una vez rellenado el formulario
String obtenerPaginaAvisoInterruptorConfiguracion_html() {
  String pagina = "<html>\n<head><meta http-equiv='Content-Type' content='text/html; charset=utf-8'/>";
  pagina += "\n<title>Casa Domotica</title>\n</head>\n<body><h1>Por favor, modifique la posición del interruptor y posteriormente pinche en el enlace para resetear la placa</h1>";
  pagina += "<a href='./operaciones?reset=1'";
  pagina += ">Resetear</a>";
  pagina += "</body></html>";
  return pagina;
}
//Devuelve la pagina principal cuando estamos en el modo configuracion
String paginaPrincipalModoConfiguracion_html() {
  String pagina = "<html>\n<head><meta http-equiv='Content-Type' content='text/html; charset=utf-8'/>";
  pagina += "\n<title>Casa Domotica</title>\n</head>\n<body><h1>Pulse el enlace al formulario de configuración para poder definir los parametros del dispositivo</h1>";
  pagina += "<a href='./configuracion'";
  pagina += ">Formulario de configuracion</a>";
  pagina += "<br>";
  pagina += "<a href='./operaciones?reset=1'";
  pagina += ">Resetear</a>";
  pagina += "</body></html>";
  return pagina;
}
//Manejador que se encarga de las peticiones http en el modo operaciones
void handle_Operaciones() {
  //Dependiendo de que argumento contenga la url de la peticion, indica el uso de distintas operaciones
  //Si la peticion contiene el argumento "iluminacion", indica que la operacion es la de controlar la iluminacion
  if ( servidorWeb.hasArg("iluminacion") ) {
    controlIluminacion_http();
    incluirLog("Peticion get con el argumento iluminacion recibido en el servidor");
  }
  //Si la petición contiene el argumento "reset", indica que la operacion es la de resetear la placa
  else if (servidorWeb.hasArg("reset") ) {
    controlReset_http();
  }
  //Si la petición contiene el argumento "enchufe", indica que la operacion es la de controlar el enchufe
  else if (servidorWeb.hasArg("enchufe")) {
    incluirLog("Peticion get con el argumento enchufe recibido en el servidor");
    controlEnchufe_http();
  }
  //Si la petición contiene el argumento "persiana", indica que la operacion es la de controlar la persiana
  else if (servidorWeb.hasArg("persiana")) {
    incluirLog("Peticion get con el argumento persiana recibido en el servidor");
    controlPersiana_http();
  }
  //Sirve una pagina con el estado del actuador que indiquemos como parametro en la peticion
  else if (servidorWeb.hasArg("estado")) {
    String estadoActuador = servidorWeb.arg("estado");
    if (estadoActuador == "actuadorIluminacion") {
      if (estadoRele == HIGH) {
        servidorWeb.send(200, "text/html", "Encendida");
      }
      else if (estadoRele == LOW) {
        servidorWeb.send(200, "text/html", "Apagada");
      }
    }
    else if (estadoActuador == "actuadorEnchufe") {
      if (estadoRele == HIGH) {
        servidorWeb.send(200, "text/html", "Conectado");
      }
      else if (estadoRele == LOW) {
        servidorWeb.send(200, "text/html", "Desconectado");
      }
    }
  }
  //Si la peticion no contiene ninguno de los anteriores argumentos, envia la pagina principal
  else {
    servidorWeb.send ( 200, "text/html", paginaPrincipal_html() );
  }
}
//Manejador que se encarga de mostrar el formulario y de recoger los datos y crear el fichero de configuracion una vez que el usuario lo envía
void handle_Configuracion() {
  //El argumento opcion sirve para controlar que el formulario ha sido enviado
  if (servidorWeb.hasArg("opcion")) {
    // Creamos el fichero fichero_configuracion.txt para escribir los argumentos enviados en el formulario
    File fichero = SPIFFS.open("/fichero_configuracion.txt", "w");
    if (!fichero) {
      Serial.println("Error al crear el fichero de configuracion");
    }
    else {
      fichero.println(servidorWeb.arg("tipoDispositivo"));
      fichero.println(servidorWeb.arg("nombreDispositivo"));
      fichero.println(servidorWeb.arg("ipDispositivo"));
      fichero.println(servidorWeb.arg("mascara"));
      fichero.println(servidorWeb.arg("gateway"));
      fichero.println(servidorWeb.arg("ssid"));
      fichero.println(servidorWeb.arg("psw"));
      fichero.println(servidorWeb.arg("ipControlador"));
      fichero.close();
    }

    leerFicheroConfiguracion();
    Serial.println("Creado correctamente el fichero mediante el formulario");
    servidorWeb.send(200, "text/html", obtenerPaginaAvisoInterruptorConfiguracion_html());
  }
  //Si no hay argumento opcion, se manda el formulario
  else {
    Serial.println("Se manda el formulario");
    servidorWeb.send(200, "text/html", obtenerFormularioConfiguracion_html());
  }
}
//Decide la pagina que muestra como principal nuestro servidor web segun el modo en el que se encuentre el dispositivo
void handle_Root() {
  if (estadoInterruptor) {
    servidorWeb.send(200, "text/html", paginaPrincipalModoConfiguracion_html());
  }
  else {
    servidorWeb.send(200, "text/html", paginaPrincipal_html());
    incluirLog("Peticion de la pagina principal al servidor");
  }
}
void handle_Log() {
  servidorWeb.send(200, "text/html", paginaLogs_html());
}
void controlReset_http() {
  String info = "El dispositivo ";
  info += nombreDispositivo;
  info += " se ha reiniciado";
  enviarLog(info);
  ESP.reset();
}
//Se encarga de obtener el tipoDispositivo con el valor obtenido del fichero de configuracion
void obtenerTipoDispositivo(int tipoDisp) {

  if (tipoDisp == ACTUADOR_ILUMINACION) {
    tipoDispositivo = ACTUADOR_ILUMINACION;
  }
  else if (tipoDisp == ACTUADOR_ENCHUFE) {
    tipoDispositivo = ACTUADOR_ENCHUFE;
  }
  else if (tipoDisp == ACTUADOR_PERSIANA) {
    tipoDispositivo = ACTUADOR_PERSIANA;
  }
  else if (tipoDisp == SENSOR_TEMPERATURA_HUMEDAD) {
    tipoDispositivo = SENSOR_TEMPERATURA_HUMEDAD;
  }
}
//Hace al tipoDispositivo leido del fichero salir preseleccionado en desplegable del formulario.
String obtenerDispositivoSeleccionado(int tipoDispositivo, int valor) {

  if (tipoDispositivo == valor)
    return "selected";

  else
    return "";
}
//Lee el fichero de configuracion y guarda los valores en variables
void leerFicheroConfiguracion() {
  Serial.println("Leyendo fichero de configuracion");
  File fichero = SPIFFS.open("/fichero_configuracion.txt", "r");
  if (!fichero) {
    Serial.println("Error al intentar leer el fichero de configuracion");
  }
  else {
    //tipoDispositivo
    String linea = fichero.readStringUntil('\n');
    linea.trim();
    Serial.println("Tipo de dispositivo: ");
    Serial.println(linea);
    obtenerTipoDispositivo(linea.toInt());
    Serial.print("Tipo de dispositivo: ");
    Serial.println(tipoDispositivo);

    //NombreDispositivo
    linea = fichero.readStringUntil('\n');
    linea.trim();
    Serial.print("Nombre del dispositivo: ");
    Serial.println(linea);
    linea.toCharArray(nombreDispositivo, 30);
    Serial.println("Char array de Nombre del dispositivo: ");
    Serial.println(nombreDispositivo);

    //ipDispositivo
    linea = fichero.readStringUntil('\n');
    linea.trim();
    Serial.print("IP del dispositivo: ");
    Serial.println(linea);
    ipDispositivo = convertirStringAIP(linea);

    //Mascara de red
    linea = fichero.readStringUntil('\n');
    linea.trim();
    Serial.print("Mascara de red: ");
    Serial.println(linea);
    mascara = convertirStringAIP(linea);

    //Gateway de la red
    linea = fichero.readStringUntil('\n');
    linea.trim();
    Serial.print("Gateway de la red: ");
    Serial.println(linea);
    gateway = convertirStringAIP(linea);

    //SSID de la red
    linea = fichero.readStringUntil('\n');
    linea.trim();
    Serial.print("SSID de la red: ");
    Serial.println(linea);
    linea.toCharArray(ssid, 30);
    Serial.println("Char array de SSID: ");
    Serial.println(ssid);

    //PASS de la red
    linea = fichero.readStringUntil('\n');
    linea.trim();
    Serial.print("Password de la red: ");
    Serial.println(linea);
    linea.toCharArray(password, 30);
    Serial.println("Char array de password: ");
    Serial.println(password);

    //Ip controlador de la red
    linea = fichero.readStringUntil('\n');
    linea.trim();
    Serial.print("IP del controlador de la red: ");
    Serial.println(linea);
    ipControlador = convertirStringAIP(linea);

    fichero.close();
  }

}
//Crea un fichero de configuracion por defecto
void crearFicheroConfiguracion() {
  Serial.println("Creacion del fichero de configuracion por defecto");

  File fichero = SPIFFS.open("/fichero_configuracion.txt", "w");
  if (!fichero) {
    Serial.println("file creation failed");
  }
  else {
    fichero.println("0");//tipoDispositivo
    fichero.println("DispositivoSinNombre");//NombreDispositivo
    fichero.println("192.168.1.175");//ipDispositivo
    fichero.println("255.255.255.0");//mascara de red
    fichero.println("192.168.1.1");//gateway
    fichero.println("");//SSID
    fichero.println("");//PASS
    fichero.println("192.168.1.104");//ipControlador
    fichero.close();

  }
  Serial.println("Fichero de configuracion creado");

}
//Convierte una IPAddress en un string para poder imprimirla con facilidad
String convertirIpAString(IPAddress ip) {
  return String(ip[0]) + String(".") + \
         String(ip[1]) + String(".") + \
         String(ip[2]) + String(".") + \
         String(ip[3])  ;
}
//Convierte un string en una IPAddress para poder utilizarla en las funciones necesarias
IPAddress convertirStringAIP(String ip) {
  IPAddress dirIP;
  int i = 0;
  char cstringToParse[100];
  ip.toCharArray(cstringToParse, 100);
  char * item = strtok(cstringToParse, ".");
  while (item != NULL && i < 4) {
    dirIP[i] = atol(item);
    item = strtok(NULL, ".");
    i++;
  }
  return dirIP;
}
void ota_setup() {
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    //Si se va a actualizar SPIFFS, se debería desmontar SPIFFS usando SPIFFS.end()
    Serial.println("Empieza la actualizacion " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nActualizacion OTA terminada");
  });
  ArduinoOTA.onProgress([](unsigned int progreso, unsigned int total) {
    Serial.printf("Progreso: %u%%\r", (progreso / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    switch (error) {
      case OTA_AUTH_ERROR:
        Serial.println("Autenticacion erronea");
        break;
      case OTA_BEGIN_ERROR:
        Serial.println("Comienzo erroneo");
        break;
      case OTA_CONNECT_ERROR:
        Serial.println("Conexion erronea");
        break;
      case OTA_RECEIVE_ERROR:
        Serial.println("Recepcion erronea");
        break;
      case OTA_END_ERROR:
        Serial.println("Finalizacion erronea");
        break;
      default:
        Serial.println("Error desconocido");
        break;

    }
  });
  ArduinoOTA.setHostname(nombreDispositivo);
  ArduinoOTA.begin();
}

