#include <Wire.h>
#include <Adafruit_ADS1X15.h>

Adafruit_ADS1115 ads;  // Crea un objeto ADS1115

float VoltajeSensor, PresionKpa;
float VoltajeSensor1, PresionKpa1;

const int pinPulso = 8;  // Pin  8 para el pulso digital

void setup() {
  Serial.begin(9600);

  // Inicia la comunicación con el ADS1115
  ads.begin();
  
  // Configura el pin de pulso digital
  pinMode(pinPulso, OUTPUT);
}

// Función para leer el sensor de presión y devolver voltaje y presión
void leerSensorPresion(int canal, float &voltaje, float &presion) {
  short adcValue = ads.readADC_SingleEnded(canal);
  voltaje = ads.computeVolts(adcValue);
  
  presion = (25 * (voltaje)) - 12.5;

}

// Función para activar/desactivar la válvula eléctrica
void activarValvula(bool estado) {
  if (estado) {
    digitalWrite(pinPulso, HIGH);  // Encender la válvula
  } else {
    digitalWrite(pinPulso, LOW);   // Apagar la válvula
  }
}

// Función para gestionar el retardo entre la activación y desactivación de la válvula
void gestionRetardo(int tiempoActivado, int tiempoDesactivado) {
  activarValvula(true);
            // Activa la válvula
  delay(tiempoActivado);          // Espera el tiempo activado
  activarValvula(false);          // Desactiva la válvula
  delay(tiempoDesactivado);       // Espera el tiempo desactivado
}

void loop() {
  // Simular la gestión de la válvula (aquí puedes ajustar los tiempos)
  gestionRetardo(1000, 1000);  // Encendido por 50 ms, apagado por 50 ms

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

  Serial.print("Sensor In - Voltaje: ");
  Serial.print(VoltajeSensor1);
  Serial.print(" V, Presion: ");
  Serial.print(PresionKpa1);
  Serial.println(" kPa");
  Serial.println("----------------------------------");

  
  
  
} 

