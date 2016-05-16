
//////////
/////////  All Serial Handling Code, 
/////////  It's Changeable with the 'serialVisual' variable
/////////  Set it to 'true' or 'false' when it's declared at start of code.  
/////////

void serialOutput()                             // Funcion que decide como sera la salida en la consola Serial.
{   
   if (serialVisual == true)
   {  
       arduinoSerialMonitorVisual('-', Signal);   // goes to function that makes Serial Monitor Visualizer  // hace que la funcion se muestre en la consola
   } 
   else
   {
        sendDataToSerial('S', Signal);            // goes to sendDataToSerial function // Hace que la señal con el valor entero se muestre en consola
   }        
}

void serialOutputWhenBeatHappens()               // Funcion que decide como sera la salida de las variables BPM y IBI
{    

 if (serialVisual == true)                     // Compara si serialVisual es true para poder imprimir en consola el contenido del If
 {  
    if(!archivo.open("Temperatura.txt", O_RDWR | O_CREAT | O_AT_END))
    {
      sd.errorHalt("Error! no se puede abrir el archivo Temperatura.txt");
      digitalWrite(3, LOW);
    }
    else
    {
      Serial.print("*** Latido Ocurrido *** ");  //ASCII Que se imprime unicamente en la consola
      Serial.print("BPM: ");
      Serial.print(BPM);
      archivo.print(BPM);                        // Guarda en el archivo el valor numerico de la variable BPM
      archivo.println(+ ",");     
      Serial.print("  ");
      archivo.close();  
    }
 } 
 else
 {
        sendDataToSerial('B',BPM);   // send heart rate with a 'B' prefix 
        sendDataToSerial('Q',IBI);   // send time between beats with a 'Q' prefix
 }   
}

//  Sends Data to Pulse Sensor Processing App, Native Mac App, or Third-party Serial Readers. 
void sendDataToSerial(char symbol, int data )
{
    Serial.print(symbol);
    Serial.println(data);  
}

//  Code to Make the Serial Monitor Visualizer Work
void arduinoSerialMonitorVisual(char symbol, int data )
{    
  const int sensorMin = 0;      // sensor minimum, discovered through experiment
  const int sensorMax = 1024;    // sensor maximum, discovered through experiment

  int sensorReading = data;
  // map the sensor range to a range of 12 options:
  int range = map(sensorReading, sensorMin, sensorMax, 0, 11);

  switch (range)                // Dependiendo del rango en los valores imprime los casos
  {
    case 0:     
      Serial.println("");     /////ASCII Art Madness
      break;
    case 1:   
      Serial.println("---");
      break;
    case 2:    
      Serial.println("------");
      break;
    case 3:    
      Serial.println("---------");
      break;
    case 4:   
      Serial.println("------------");
      break;
    case 5:   
      Serial.println("--------------|-");
      break;
    case 6:   
      Serial.println("--------------|---");
      break;
    case 7:   
      Serial.println("--------------|-------");
      break;
    case 8:  
      Serial.println("--------------|----------");
      break;
    case 9:    
      Serial.println("--------------|----------------");
      break;
    case 10:   
      Serial.println("--------------|-------------------");
      break;
    case 11:   
      Serial.println("--------------|-----------------------");
      break;
  } 
}
