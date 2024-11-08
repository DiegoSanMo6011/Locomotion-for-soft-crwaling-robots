
const int pinPulso = 8;  // Pin  8 para el pulso digital

void setup() {
  Serial.begin(9600);

  
  // Configura el pin de pulso digital
  pinMode(pinPulso, OUTPUT);
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
  activarValvula(true);           // Activa la válvula
  delay(tiempoActivado);          // Espera el tiempo activado
  activarValvula(false);          // Desactiva la válvula
  delay(tiempoDesactivado);       // Espera el tiempo desactivado
}

void loop() {
  // Simular la gestión de la válvula (aquí puedes ajustar los tiempos)
  gestionRetardo(1000, 1000);  // Encendido por 50 ms, apagado por 50 ms

} 