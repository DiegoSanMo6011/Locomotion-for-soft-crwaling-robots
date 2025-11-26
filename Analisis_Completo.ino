#include <Wire.h>
#include <Adafruit_ADS1X15.h>

Adafruit_ADS1115 ads;

/* ---------- PINES ESP32 ---------- */
const int SDA_PIN   = 21;
const int SCL_PIN   = 22;
const int pinPWM    = 18;   // pin PWM hacia la bomba

// NUEVO: pin de la válvula del sistema
const int VALVE_PIN = 25;   // <-- CAMBIA esto al pin real de tu válvula

/* ---------- CONFIG PWM ESP32 ---------- */
const int PWM_CHANNEL    = 0;
const int PWM_FREQ       = 1000;   // Hz
const int PWM_RESOLUTION = 8;      // 0–255

int currentPWM = 0;

/* ---------- SENSOR DE PRESIÓN ---------- */
const float V_OFFSET  = 2.5f;   // Voltaje a 0 kPa
const float V_PER_kPa = 0.02f;  // Volts por kPa

/* ---------- PARÁMETROS DEL EXPERIMENTO ---------- */

// Muestreo
const unsigned long SAMPLE_PERIOD_MS = 10;    // 10 ms

// Barrido de PWM
const int PWM_START = 0;       // PWM inicial
const int PWM_END   = 255;     // PWM final
const int PWM_STEP  = 5;       // tamaño del paso

// Duración de cada escalón
const unsigned long STEP_HOLD_MS = 5000;       // 5 s por escalón

// Tiempo en reposo antes y después del barrido
const unsigned long BASELINE_BEFORE_MS = 2000;  // 2 s en PWM=0 antes del barrido
const unsigned long BASELINE_AFTER_MS  = 2000;  // 2 s en PWM=0 después del barrido

/* ---------- MÁQUINA DE ESTADOS ---------- */
enum State {
  STATE_BASELINE_BEFORE,
  STATE_SWEEP_UP,
  STATE_BASELINE_AFTER,
  STATE_FINISHED
};

State state = STATE_BASELINE_BEFORE;

unsigned long tExperiment0   = 0;
unsigned long tLastSample    = 0;
unsigned long tStepChange    = 0;

int pwmValue = PWM_START;

/* ---------- FUNCIONES AUXILIARES ---------- */

// Convertir lectura ADS1115 a presión en kPa
float readPressurekPa() {
  int16_t adc = ads.readADC_SingleEnded(0);    // canal A0
  float volts = ads.computeVolts(adc);
  float p_kPa = (volts - V_OFFSET) / V_PER_kPa;  // puede ser negativa (vacío)
  return p_kPa;
}

// Escribir PWM en el canal del ESP32
void setPWM(int value) {
  if (value < 0)   value = 0;
  if (value > 255) value = 255;
  currentPWM = value;
  ledcWrite(PWM_CHANNEL, currentPWM);
}

/* ========================= SETUP ========================= */

void setup() {
  Serial.begin(115200);
  delay(2000);

  // I2C
  Wire.begin(SDA_PIN, SCL_PIN);

  // ADS1115
  if (!ads.begin()) {
    Serial.println("ERROR: No se encontró el ADS1115");
    while (1) { delay(1000); }
  }
  ads.setGain(GAIN_ONE);

  // PWM ESP32
  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(pinPWM, PWM_CHANNEL);

  // NUEVO: configurar válvula
  pinMode(VALVE_PIN, OUTPUT);
  digitalWrite(VALVE_PIN, LOW);  // cerrada al inicio

  // Estado inicial
  setPWM(0);
  tExperiment0 = millis();
  tLastSample  = tExperiment0;
  tStepChange  = tExperiment0;

  // Encabezado CSV
  Serial.println("t_ms,PWM,Presion_kPa");
}

/* ========================= LOOP ========================= */

void loop() {
  unsigned long now = millis();

  // 1) Enviar muestra periódicamente
  if (now - tLastSample >= SAMPLE_PERIOD_MS) {
    tLastSample = now;

    float pres_kPa    = readPressurekPa();
    unsigned long t_ms = now - tExperiment0;

    Serial.print(t_ms);
    Serial.print(",");
    Serial.print(currentPWM);
    Serial.print(",");
    Serial.println(pres_kPa, 3);
  }

  // 2) Lógica de la máquina de estados
  switch (state) {

    case STATE_BASELINE_BEFORE:
      // Baseline con bomba apagada y válvula cerrada
      setPWM(0);
      digitalWrite(VALVE_PIN, LOW);

      if (now - tExperiment0 >= BASELINE_BEFORE_MS) {
        // Pasar al primer escalón del barrido
        pwmValue   = PWM_START;
        setPWM(pwmValue);
        tStepChange = now;

        // NUEVO: abrir la válvula durante el barrido
        digitalWrite(VALVE_PIN, HIGH);

        state = STATE_SWEEP_UP;
      }
      break;

    case STATE_SWEEP_UP:
      // Barrido con válvula ABIERTA (sistema completo con pérdidas)
      if (now - tStepChange >= STEP_HOLD_MS) {
        pwmValue += PWM_STEP;
        if (pwmValue > PWM_END) {
          // Termina barrido → baseline final con válvula aún abierta
          setPWM(0);
          tStepChange = now;
          state = STATE_BASELINE_AFTER;
        } else {
          setPWM(pwmValue);
          tStepChange = now;
        }
      }
      break;

    case STATE_BASELINE_AFTER:
      // Baseline final con bomba apagada y válvula ABIERTA
      setPWM(0);
      digitalWrite(VALVE_PIN, HIGH);

      if (now - tStepChange >= BASELINE_AFTER_MS) {
        state = STATE_FINISHED;
      }
      break;

    case STATE_FINISHED:
      // Experimento terminado: bomba apagada y válvula CERRADA
      setPWM(0);
      digitalWrite(VALVE_PIN, LOW);
      while (1) { delay(1000); }
      break;
  }
}
