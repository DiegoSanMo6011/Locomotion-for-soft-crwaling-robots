#include <Wire.h>
#include <Adafruit_ADS1X15.h>

Adafruit_ADS1115 ads;  // Crea un objeto ADS1115

float VoltajeSensor, PresionKpa;
float VoltajeSensor1, PresionKpa1;


void setup() {
  Serial.begin(9600);

  // Inicia la comunicación con el ADS1115
  ads.begin();
  
}

// Función para leer el sensor de presión y devolver voltaje y presión
void leerSensorPresion(int canal, float &voltaje, float &presion) {
  short adcValue = ads.readADC_SingleEnded(canal);
  voltaje = ads.computeVolts(adcValue);
  presion = ((voltaje)-2.5)/0.02;

}





void loop() {

  // Leer los valores de voltaje y presión de los dos sensores
  leerSensorPresion(0, VoltajeSensor, PresionKpa);
  leerSensorPresion(1, VoltajeSensor1, PresionKpa1);

  // Imprimir en el monitor serial el voltaje y la presión de cada sensor
  Serial.println("------- Lectura de Sensores -------");
  Serial.print("Sensor Out - Voltaje: ");
  Serial.print(VoltajeSensor);
  Serial.print(" V, Presion: ");
  Serial.print(PresionKpa);
  Serial.println(" kPa");
/*
  Serial.print("Sensor In - Voltaje: ");
  Serial.print(VoltajeSensor1);
  Serial.print(" V, Presion: ");
  Serial.print(PresionKpa1);
  Serial.println(" kPa");
*/
  Serial.println("----------------------------------");
  delay(1000);

  
  
  
} 