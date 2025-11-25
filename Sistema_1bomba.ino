#include <Wire.h>
#include <Adafruit_ADS1X15.h>

Adafruit_ADS1115 ads;

/*---------- PINES ESP32 ----------*/
// Pines I2C del ESP32
const int SDA_PIN = 21;
const int SCL_PIN = 22;

// PWM hacia la bomba (un solo pin PWM válido en ESP32)
const int pinPWM = 17;   // Ajusta si usas otro pin

/*---------- CURVA DEL SENSOR DE PRESIÓN ----------*/
// Ajusta estos valores según tu sensor
const float V_OFFSET  = 2.5f;    // Voltaje a 0 kPa
const float V_PER_kPa = 0.02f;   // Voltios por cada kPa

/*---------- PARÁMETROS DE MUESTREO / ENSAYO ----------*/
const float Ts = 0.05;                 // Tiempo de muestreo 50 ms
const unsigned long Ts_ms = 50;

const int PWM_MIN  = 0;               // Rango operativo estable de tu bomba
const int PWM_MAX  = 255;
const int PWM_STEP = 5;               // Paso de PWM para caracterización

const unsigned long STEP_DURATION_MS = 5000; // 5 s en cada nivel de PWM

/*---------- VARIABLES DE ESTADO ----------*/
unsigned long tPrev = 0;
unsigned long lastStepChange = 0;
int  currentPWM   = PWM_MIN;
bool testFinished = false;

/*---------- PROTOTIPO -----------*/
float leerPresion_kPa();          // lee sensor y devuelve kPa

/*========================= SETUP =========================*/
void setup() {
  Serial.begin(115200);
  delay(10000);

  // Iniciar I2C en ESP32
  Wire.begin(SDA_PIN, SCL_PIN);

  // Iniciar ADS1115
  if (!ads.begin(0x48)) {
    Serial.println("❌ No se encontró ADS1115. Revisa las conexiones I2C.");
    while (1);
  }

  // Ganancia del ADS: ±6.144 V (hasta ~5 V)
  ads.setGain(GAIN_TWOTHIRDS);

  // PWM hacia la bomba
  pinMode(pinPWM, OUTPUT);
  analogWrite(pinPWM, 0);   // bomba apagada al inicio

  // Inicializar tiempos
  tPrev          = millis();
  lastStepChange = millis();
  currentPWM     = PWM_MIN;

  // Cabecera para CSV
  Serial.println(F("t_ms,PWM,Presion_kPa"));
}

/*========================== LOOP =========================*/
void loop() {
  unsigned long now = millis();

  // Si ya terminamos todo el barrido, apagamos bomba y nos quedamos quietos
  if (testFinished) {
    analogWrite(pinPWM, 0);
    return;
  }

  // 1) Actualizar nivel de PWM cada STEP_DURATION_MS
  if (now - lastStepChange >= STEP_DURATION_MS) {
    lastStepChange = now;
    currentPWM += PWM_STEP;

    if (currentPWM > PWM_MAX) {
      // Fin de la prueba
      currentPWM = 0;
      analogWrite(pinPWM, 0);
      Serial.println(F("# Fin del barrido de PWM. Prueba completada."));
      testFinished = true;
    }
  }

  // 2) Muestreo cada Ts_ms (50 ms)
  if (now - tPrev >= Ts_ms) {
    tPrev = now;

    // Leer presión
    float P = leerPresion_kPa();

    // Aplicar PWM actual a la bomba
    analogWrite(pinPWM, currentPWM);

    // Enviar datos en formato CSV
    Serial.print(now);        Serial.print(',');
    Serial.print(currentPWM); Serial.print(',');
    Serial.println(P, 3);     // presión con 3 decimales
  }
}

/*---------------------------------------------------------
  Lee el canal 0 del ADS1115 y convierte a kPa
 ---------------------------------------------------------*/
float leerPresion_kPa() {
  int16_t adc = ads.readADC_SingleEnded(0);
  float volt  = ads.computeVolts(adc);

  float P_kPa = (volt - V_OFFSET) / V_PER_kPa;  // kPa (será negativa en vacío)

  return P_kPa;
}
