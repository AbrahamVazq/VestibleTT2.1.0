////////////////////////////////////////////////////////////////////////////////////////////////////////
#include <SPI.h>
#include <SdFat.h>
#include <Time.h>
#include <Arduino.h>
#include <ble_mini.h>

#if defined (__AVR_ATmega168__) || defined (__AVR_ATmega328P__)  
  AltSoftSerial BLEMini;  
#else
  #define BLEMini Serial1
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////
/*Creamos objetos de la clase SdFat */
SdFat sd;                         // Creamos un objeto de la clase SdFat llamado sd
SdFile archivo;                   // Creamos un objeto de la clase SdFile llamado archivo
////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Variables
int pulsePin = A0;                // Pin donde va conectado el cable de señal del sensor de pulso (Cable purpura)
int tempPin = A2;                 // Definimos la entrada anlogica en pin A0 
int blinkPin = 13;                // Pin donde el LED sera el primero en iniciar el brillo
int fadePin = 5;                  // Pin que simulara el efecto del latido en cada brillo
int fadeRate = 0;                 // Variable usada para opacar el led en un pin con PWM
float tempC;                      // variable para el dato del Sensor
const int chipSelect = 4;         // Pin para el ChipSelect
const int btnPin = 8;             // Pin donde esta conectado un boton de Pausa para separar datos
int btnEstado = 0;                // Este es el estado inicial del boton de pausa
                
// Variables volatiles, usadas en las rutinas de interrupcion
volatile int BPM;                   // Variable entera que recibe el dato que detecta el Sensor de pulso antes de ser tratado cada 2mS
volatile int Signal;                // Variable que mantiene el dato entrante 
volatile int IBI = 600;             // Variable del intervalo de tiempo entre cada beat, Must be seeded!
volatile boolean Pulse = false;     // Variable se vuelve 'true' cuando se ha detectado el latido del usuario "vivo",
                                    // falso cuando el latido no es de un cuerpo vivo*
volatile boolean QS = false;        // Variable se vuelve 'true' cuando el arduino encuentra un pulso 

// Consideraciones para la salida Seral --> ( Configurable a las necesidades )
static boolean serialVisual = true;   // Inicializado en Falso, configuralo en 'true' para ver en la consola de arduino el puslo en ASCII
                                      

void setup()
{

  while (!Serial) {} 
  
  pinMode(blinkPin,OUTPUT);         // Led en este pin Brillara en un latido
  pinMode(fadePin,OUTPUT);          // Led en este pin se opacara en un latido
  pinMode(3,OUTPUT);                // Pin que nos dira si el lector esta en uso
  pinMode(7,OUTPUT);                // Recibe la señal de pausa desde el LED
  pinMode(8,INPUT);                 // Boton que envia la señal de pausa 

  BLEMini.begin(57600); 
  //Serial.begin(57600); 
  setTime(16,00,00,9,3,2016);       //Incializamos una fecha y hora de referencia
  Serial.begin(115200);

  //BLEMini.begin(57600); 
  //Serial.begin(57600);
    
  interruptSetup();                 // Inicia la lectura del Sensor de pulso cada 2m

  /*Si estas alimentando el Sensor de Pulso con un voltaje menor del que maneja Arduino Leonardo
   Descomenta la siguiente linea y aplica el voltaje en el pin A-REF
         analogReference(EXTERNAL); */

  /* Inicializa el mudulo SD por medio de la libreria SdFat, imprimira el error en caso de encontrarlo
     usa la velocidad media como la libreria nativa
     Cambia a SPI_FULL_SPEED si es necesario para mejorar la funcion. */

  /* Inicializa SdFat */
  if (!sd.begin(chipSelect, SPI_HALF_SPEED))   // Si sd.begin no se peude inicializar lanza el error
  {
    sd.initErrorHalt();                        // Funcion que delata el error de detecion y lectura del modulo SD
  }
  /*Abre el archivo para escritura la sintaxis se parece mucho a la libreria Nativa SD.h */
  if (!archivo.open("Temperatura.txt", O_RDWR | O_CREAT | O_AT_END))     // Compara si el archivo se ha abierto si no, lanza el error
  {
    sd.errorHalt("Error! no se puede abrir el archivo Temperatura.txt"); // Imprime un error de incapacidad para abrir el archivo
  }
  /* Si el archivo se abrio correctamente entonces escribe en el */
  Serial.print("Ahora estamos escribiendo en el Archivo.");
  archivo.println("-= Temperatura Corporal =-");            // Escribe al inicio del archivo la leyenda dentro
  archivo.println("[");                                     // Escribe al inicio del archivo un corchete abierto
  
  archivo.close();                                          // Funcion que cierra el Archivo
  Serial.println("Archivo Cerrado.");
}


unsigned char buf[16] = {0};
unsigned char len = 0;

/////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////// M A I N //////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

void loop()
{
    serialOutput() ;                      // Funcion que manda a llamar los datos a la consola
    btnEstado = digitalRead(btnPin);      // variable que contiene el valor analogico del boton de pausa
  
    if(btnEstado == HIGH)                 // Si el boton es presionado llama a la funcion interrumpe
    {
      interrumpe();                       // La funcion interrumpe pausa por 5 segundos la lecutra de los datos 
                                          // el tiempo puede cambiar segun se requiera
    }
    else
    {
      temperatura();                      // Funcion que hace la rutina de leer del sensor de temperatura
      delay(20);                          // Duerme 20mS
      pulso();                            // Funcion que hace la rutina de leer del sensor de pulso
      delay(20);
    }

}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

void ledFadeToBeat()
{
    fadeRate -= 15;                         // Inicializa el valor de opacidad del LED
    fadeRate = constrain(fadeRate,0,255);   // Mantiene la opacidad del led teniendo valores negativos
    analogWrite(fadePin,fadeRate);          // Opaca el LED
}
/////////////////////////////////////////////////////////////////////////////////////////////////////
void pulso()
{
    if (QS == true)
    {                 // A Heartbeat Was Found // Un latido ha sido detectado
                                       // BPM and IBI have been Determined // 
                                       // Quantified Self "QS" true when arduino finds a heartbeat
      fadeRate = 255;                  // Opaca el led
                                       // Asigna a la variable 'fadeRate' el valor 255 para opcar el LED en cada pulso
      serialOutputWhenBeatHappens();   // Un pulso ha ocurrido, y se muestra en la consola
      QS = false;                      // Reinicia el valor de la bandera para la siguiente ocacion 
    }
     
  ledFadeToBeat();                       // Funcion que hace que el opacado del LED ocurra
  delay(20);                             // Duerme 20 mS
}

/////////////////////////////////////////////////////////////////////////////////////////////////////

void temperatura()
{
   /*Creamos una objeto t de la clase tiempo */
    time_t t = now();
  /* Leemos el valor desde el sensor */
    tempC = analogRead(tempPin);

  /* Convierte el valor a temperatura*/
    tempC = (5.0 * tempC * 100.0)/1024.0;
  /* Datos que se mostraran en consola serial.
      Los datos se imprimen unicamente en la cosola de Arduino*/
    Serial.print(tempC);
    Serial.print(" Grados Celsius a las ");
    Serial.print(+ " ");
    Serial.print(hour(t));
    Serial.print(+ ":");
    Serial.print(minute(t));
    Serial.print(+ ":");
    Serial.print(second(t));
    Serial.print("\n");
    
   /* Re-abrimos el archivo para lectura */
   if (!archivo.open("Temperatura.txt", O_RDWR | O_CREAT | O_AT_END))     // Compara si el archivo se ha abierto si no, lanza el error
  {
    sd.errorHalt("Error! no se puede abrir el archivo Temperatura.txt"); // Imprime un error de incapacidad para abrir el archivo
    digitalWrite(3, LOW);
  }
  else
  {
    digitalWrite(3, HIGH);
  }
  
  /*Si el archivo se abrio correctamente, escribimos en el.
    Los datos se imprimen en el archivo.*/
  Serial.print("La lectura sensa: ");
  archivo.print("\"");
  archivo.print(hour(t));
  archivo.print(+ ":");
  archivo.print(minute(t));
  archivo.print(+ ":");
  archivo.print(second(t));
  archivo.print("\"");
  archivo.print(+ " ");
  archivo.print(tempC);
  archivo.print(+ ",");
  archivo.close();                // Cierra el archivo para poder ser abierto mas adelante
   delay(1000);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
void interrumpe()
{
    digitalWrite(7, HIGH);        // Enciende el LED en el pin 7
    delay(500);                   // Duerme 500 mS
    digitalWrite(7,LOW);          // Apaga el LED en el pin 7
    Serial.println(+ "]");        // Imprime en consola el caracter de corchete cerrado
    Serial.println("Pausando escritura de Archivo en un bloque e iniciando otro.");
    Serial.println(+ "[");      
    archivo.println(+ "]");       // Imprime en el ARCHIVO un corchete Cerrado
    BLE();    
    archivo.println(+ "[");       // Imprime en el ARCHIVO un corchete Abierto
    delay(5000);                  // Duerme 5 segundos
}
////////////////////////////////////////////////////////////////////////////////////////////////////////
void BLE()
{
  while ( BLEMini.available())
  {
    Serial.write( BLEMini.read() );
  }

  while ( Serial.available() )
  {
    unsigned char c = Serial.read();
    if (c != 0x0A)
    {
      if (len < 16)
        buf[len++] = c;
    }
    else
    {
      buf[len++] = 0x0A;
      
      for (int i = 0; i < len; i++)
         BLEMini.write(buf[i]);
      len = 0;
    }
  }

     /* Re-abrimos el archivo para lectura */
   if (!archivo.open("Temperatura.txt", O_RDWR | O_CREAT | O_AT_END))     // Compara si el archivo se ha abierto si no, lanza el error
  {
    sd.errorHalt("Error! no se puede abrir el archivo Temperatura.txt"); // Imprime un error de incapacidad para abrir el archivo
    digitalWrite(3, LOW);
  }
  else
  {
    digitalWrite(3, HIGH);
  }

  // read from the file until there's nothing else in it:
  int data;
  while ((data = archivo.read()) >= 0) {
    Serial.write(data);
  }
  // close the file:
 
  BLEMini.write(data);
//  BLEMini.write('H');

    archivo.close();
//    delay(1000);
}

