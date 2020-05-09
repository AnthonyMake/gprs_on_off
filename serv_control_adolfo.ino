/*
Control de servicio de agua.
2019-08-19

al secuencia que sigue el programa es la siguiente:
1. Enciende modem (conecta internet, conecta a ubidots)
2. Envia posicion GPS como registro de activiodad.
3. Consulta dato de valvula en ubidots.
4. Aplica dato a la senal de valvula.
5. Apaga modem
6. Espera un tiempo determinado -wait_time-
7. Vuelve al punto 1.

*/

#include <SoftwareSerial.h>

// comunicacion serial con modulo telit
SoftwareSerial modem(10,11); // RX, TX

// pin para encender el modulo telit
#define PowerModem 7

// Key para acceder a cuenta ubidots
String TOKEN = "BBFF-X3qG90U80mYLQnoyy4FELtOhQ1iZeU"; 
// accesso a cuenta ubidots
// user: serv_control190813
// pass: som_pass 
//
// gmail
// mail: serv.control190813@gmail.com
// pass: som_pass


// Data del cliente - cliente_XXXX
String USER_AGENT = "cliente_0001"; //device name in ubidots
char *client = "cliente_0001";

// Posicion GPS
char *lat = "-33.400987";
char *lng = "-70.569238";

// tiempo de actualizacion del estado 
// del servicio, en milisegundos
int wait_time = 1000;

int pin_valve = 4; // conexion senal valvula

// variables auxiliares
char *dev_name;
char *dev_var;
char *dev_value;
char buff[200];

void setup() {

  //Inicia los puertos seriales
  Serial.begin(9600); // Arduino-PC
  modem.begin(9600);  // Arduino-moduloTelit

  pinMode(pin_valve,OUTPUT);
}

void loop() {

  // limpia buffer inicial 
  modem.listen(); 
  
  // inicializa modem -177- 
  modem_init();
  
  // envia estado del dispositivo y posicion gps 
  modem_send(client,"last_conn", 1, lat, lng);
  
  // vaciando buffer
  modem.listen();

  // solo espera entre consultas
  delay(1000);

  // obtiene estado de la valvula y lo almacena en la variable servicio
  int servicio = modem_get(client,"servicio");
  Serial.print("received = ");
  Serial.println(servicio);

  // Cambia la senal de la valvula en base al valor recibido.
  // *reverse logic*
  if(servicio == 1){
    digitalWrite(pin_valve,LOW);
  }else{
    digitalWrite(pin_valve,HIGH);
  }
  
  modem.listen();

  // apaga el modem  
  modem_off();
  
  // mensajes de debug
  Serial.print("awaiting ");Serial.print(wait_time);Serial.println("ms until next cycle. ALL_DONE!\n\n");
  delay(wait_time);

}



int modem_get(char* nam, char* var){
  // Consulta datos a ubidots
  // modem_get(device_label, variable_label)

  // solicitud  de datos
  String to_get = ""; //{useragent}|LV|{token}|{device_label}:{variable_label}|end
  to_get += USER_AGENT + "|LV|" + TOKEN + "|" + nam + ":" + var + "|end";
  
  // comando AT para enviar data desde modem
  modem.println("AT#SSEND=1"); //Enviar datos
  serial_thing(0);
  
  // envia solicitud.
  modem.print(to_get);
  modem.write(0x1A); // caracter de fin de instruccion.

  Serial.println(to_get);

  Serial.print("Getting data for ");Serial.print(nam);Serial.print("|");Serial.print(var);
  int att = 0;

  // intenta obtener el dato hasta 10 veces
  while(att < 10){
    
    modem.println("AT#SRECV=1,50"); //comando AT para recibir datos
    Serial.print(".");
    serial_thing(0);
    
    // busca entre los datos recibidos OK y SRECV
    if (strstr(buff,"OK") && strstr(buff,"SRECV") ){
      Serial.println("DONE");
      //just in case, muestra lo que hay en buffer
      // Serial.println("-----------------------");
      // for (int i=0; i<200 ;i++){
      //   Serial.print(buff[i]);
      // }
      // Serial.println("-----------------------");

      // retorna el dato recibido
      if(strstr(buff,"OK|0"))
        return 0;
      if(strstr(buff,"OK|1"))
        return 1;
      
      break;
    }
    att++;
    if (att>5)
      Serial.println("FAIL"); // a la quinta vez empezara a decir que fallo
  }
  
}

void modem_send(char* nam, char* var, int some_int, char* lat, char* lng){
  // envia datos a ubidots

  //this WA convert an int into char
  //idk how to make it func
  char val[10];
  itoa(some_int, val,10);

  //mbed/1.2|POST|p4uuT2OIIFJwv7ncTVfoVqcfImwRQW|my-device=>temperature:14,position:0$lat=6.2$lng=-76.3|end  
  
  // formamos solicitud de envio
  String to_send = "";
  to_send += USER_AGENT + "|POST|" + TOKEN + "|" + nam + "=>" + var + ":" + val + ",positon:0$lat=" + lat + "$lng=" + lng + "|end";

  // commando AT para envio de data
  modem.println("AT#SSEND=1"); //Enviar datos
  serial_thing(0);
  
  // se envia solicitud
  modem.print(to_send);
  modem.write(0x1A); // caracter de fin de instruccion

  Serial.print("Sending data to ");
  Serial.print(nam);Serial.print(".");Serial.print(var);Serial.print("=");Serial.print(val);
  serial_thing(0);

  // se intenta multiples veces recibir el dato hasta que la respuesta contenga 'OK'
  while(1){
    modem.println("AT#SRECV=1,15"); //recibir datos
    Serial.print(".");
    serial_thing(0);

    delay(2000);
    
    if (strstr(buff,"OK")){
      Serial.println("DONE");
      //just in case
      //Serial.println("-----------------------");
      //for (int i=0; i<200 ;i++){
      //  Serial.print(buff[i]);
      //}
      //Serial.println("-----------------------");
      break;
    }

  }
  
}


void modem_init(){
  // enciende el modem y lo conecta a 
  // la red
  
  Serial.print("Turn on Modem: ");
  pinMode(PowerModem, OUTPUT);
  digitalWrite(PowerModem, HIGH);  //Para iniciar el modem
  delay(2000);
  digitalWrite(PowerModem, LOW);   //Para iniciar el modem
  delay(2000);
  digitalWrite(PowerModem, HIGH);  //Para iniciar el modem
  
  Serial.println("DONE"); // modem encendido
  
  serial_thing(0);

  serial_thing(0);

  // seteo proveedor 'APN'
  modem.println("AT+CGDCONT=1,\"IP\",\"imovil.entelpcs.cl\"");

  serial_thing(0);

  // solicita IP
  modem.println("ATE0");
  serial_thing(0);

  // intenta reiteradas veces hata obtener una IP
  Serial.print("Getting IP: ");
  while(1){
    modem.println("AT#SGACT=1,1");   //pide IP
    Serial.print(".");
    serial_thing(0);

    delay(2000);
    
    if (strstr(buff,"OK")){
      Serial.println("DONE");

      Serial.println("-----------------------");
      for (int i=0; i<200 ;i++){
        Serial.print(buff[i]);
      }
      Serial.println("-----------------------");
      break;
    }
  
  }

  // conexion con el sitio ubidots
  Serial.print("Open socket to ubidots TCP: ");
  modem.println("AT#SD=1,0,9012,\"translate.ubidots.com\",0,0,1");
  serial_thing(0);
  //this never goes wrong
  Serial.println("DONE");
  
}


// funcion auxiliar que sirve para 
// espera comunicacion serial e intentar determinadas veces.
void serial_thing(boolean debug){
  delay(300);
  for (int i=0; i<200 ;i++)
    buff[i] = '\0';
  
  if(debug)
    Serial.println("awaiting response...");
  
  unsigned long tim = millis();
  unsigned long t_count = 0;
  while(modem.available() == 0){
    t_count = millis() - tim;
   
    if (t_count > 4000 ){
      Serial.println("nothing to show before 3s.");
      break;
    }
  }

  int ct = 0;
  while(modem.available()>0){
    buff[ct] = modem.read();
    ct++;
    delay(1);
  }
  
  if(debug){
    Serial.print("\nI've got this after  ");
    Serial.print(t_count);
    Serial.println("ms");

    for (int i=0; i<ct ;i++){
      Serial.print(buff[i]);  
    }

    Serial.println("\nend.");
  }  
}

// apagado de modem
void modem_off(){

  Serial.print("Turn off modem: ");
  modem.println("AT#SH=1"); //Finaliza conexion
  serial_thing(0);

  digitalWrite(PowerModem,LOW);
  delay(1000);
  Serial.println("DONE\n\n");
}
