#include <Wire.h>
#include <Adafruit_ADS1X15.h>

Adafruit_ADS1115 ads;  // Crea un objeto ADS1115

float VoltajeSensor, PresionKpa;
float VoltajeSensor1, PresionKpa1;

const int pinPulso = 8;  // Pin 8 para la válvula

bool sistemaActivo = false;  // Indica si el sistema está activo

void setup() {
  Serial.begin(9600);

  // Inicia la comunicación con el ADS1115
  ads.begin();
  
  // Configura el pin de salida para la válvula
  pinMode(pinPulso, OUTPUT);

  // Asegúrate de que la válvula esté apagada al inicio
  digitalWrite(pinPulso, LOW);

  Serial.println("Sistema inicializado. Use 'ON' para encender y 'OFF' para apagar.");
}

// Función para leer el sensor de presión y devolver voltaje y presión
void leerSensorPresion(int canal, float &voltaje, float &presion) {
  short adcValue = ads.readADC_SingleEnded(canal);
  voltaje = ads.computeVolts(adcValue);
  presion = ((voltaje) - 2.5) / 0.02;
}

// Función para activar/desactivar la válvula eléctrica
void activarValvula(bool estado) {
  if (estado) {
    digitalWrite(pinPulso, HIGH);  // Encender la válvula
    Serial.println("Válvula encendida.");
  } else {
    digitalWrite(pinPulso, LOW);  // Apagar la válvula
    Serial.println("Válvula apagada.");
  }
}

// Función para activar/desactivar el sistema completo
void activarSistema(bool estado) {
  if (estado) {
    sistemaActivo = true;
    Serial.println("Sistema encendido.");
  } else {
    sistemaActivo = false;
    activarValvula(false);  // Asegura que la válvula esté apagada al desactivar el sistema
    Serial.println("Sistema apagado.");
  }
}

void loop() {
  // Leer comando desde el monitor serial
  if (Serial.available()) {
    String comando = Serial.readStringUntil('\n');
    comando.trim();  // Elimina espacios en blanco o saltos de línea

    if (comando.equalsIgnoreCase("ON")) {
      activarSistema(true);  // Enciende el sistema
    } else if (comando.equalsIgnoreCase("OFF")) {
      activarSistema(false);  // Apaga el sistema
    } else {
      Serial.println("Comando no reconocido. Use 'ON' o 'OFF'.");
    }
  }

  // Si el sistema está activo, lee los sensores y activa la válvula según sea necesario
  if (sistemaActivo) {
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

    // Control simple para la válvula (puedes ajustar la lógica según sea necesario)
    activarValvula(true);
    delay(1860);  // Válvula encendida por 1 segundo
    activarValvula(false);
    delay(140);  // Válvula apagada por 1 segundo
  }
}

