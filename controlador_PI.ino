#include <Wire.h>
#include <Adafruit_ADS1X15.h>

// I2C
Adafruit_ADS1115 ads;

/*---------- PINES ESP32 ----------*/
const int SDA_PIN = 21;
const int SCL_PIN = 22;
const int pinPWM = 18;

/*---------- CONFIG PWM ESP32 ----------*/
const int PWM_CHANNEL    = 0;
const int PWM_FREQ       = 1000;      // Hz
const int PWM_RESOLUTION = 8;         // bits → 0–255
const int PWM_MAX        = 255;

int currentPWM = 0; // Se actualiza con la salida del controlador

/*---------- PARÁMETROS DEL CONTROLADOR PI (ACTUALIZADOS) ----------*/
// Kp: 5.6845, Ki: 113.4116
const float KP_CONTROLLER = 5.6845f;
const float KI_CONTROLLER = 113.4116f;

// Tiempo de muestreo (10 ms = 0.01 s).
const float TS_CONTROLLER = 0.010f; 

// REFERENCIA DENTRO DEL RANGO LINEAL (47 kPa a 50 kPa)
const float pressure_reference = 60.0f; // KPa a la que queremos llegar

/*---------- VARIABLES DE CONTROL ----------*/
float integral_sum = 0.0f; 
float error_prev = 0.0f; // Para control PD o anti-windup (no usado aquí, pero buena práctica)

/*---------- MODELO DEL SENSOR DE PRESIÓN ----------*/
const float V_OFFSET  = 2.5f;   // Voltaje a 0 kPa
const float V_PER_kPa = 0.02f;  // Volt/ kPa

/*---------- PARÁMETROS DEL EXPERIMENTO ----------*/
const int PWM_BASE = 0; 
const unsigned long T_BASELINE_MS   = 5000;   // 5 s de espera inicial
const unsigned long T_CONTROL_MS    = 30000;  // 30 s de control activo
const unsigned long SAMPLE_PERIOD_MS = 10;    // 10 ms → 100 Hz

/*---------- MÁQUINA DE ESTADOS ----------*/
enum State {
  STATE_BASELINE,  // PWM_BASE
  STATE_CONTROL,   // Ejecutar lazo PI
  STATE_FINISHED   // Experimento terminó
};

State state = STATE_BASELINE;

unsigned long tExperiment0 = 0;   
unsigned long tLastSample  = 0;   
unsigned long tControlStart = 0; 

/*---------- FUNCIONES AUXILIARES ----------*/

float readPressurekPa() {
  int16_t adc = ads.readADC_SingleEnded(0);  
  float volts = ads.computeVolts(adc);
  float p_kPa = (volts - V_OFFSET) / V_PER_kPa;
  return p_kPa;
}

void setPWM(int pwmValue) {
  if (pwmValue < 0)   pwmValue = 0;
  if (pwmValue > PWM_MAX) pwmValue = PWM_MAX;
  currentPWM = pwmValue;
  ledcWrite(PWM_CHANNEL, currentPWM);
}

/*---------- ALGORITMO DE CONTROL PI EN TIEMPO DISCRETO ----------*/
void runPIControl(float currentPressure) {
  // 1. Cálculo del Error
  float error = pressure_reference - currentPressure;

  // 2. Cálculo de la Acción Integral
  integral_sum += error * TS_CONTROLLER;

  // --- Anti-windup Básico ---
  // Si el controlador está pidiendo más del máximo (255) y el error es positivo,
  // detenemos la integración para evitar saturación excesiva.
  float predicted_pwm = (KP_CONTROLLER * error) + (KI_CONTROLLER * integral_sum);
  if ((predicted_pwm > PWM_MAX && error > 0.0f) || (predicted_pwm < 0 && error < 0.0f)) {
      // Revertir el último paso de integración
      integral_sum -= error * TS_CONTROLLER; 
  }
  
  // 3. Cálculo de la Salida de Control (PWM)
  float pwm_output_float = (KP_CONTROLLER * error) + (KI_CONTROLLER * integral_sum);

  // 4. Saturación y Aplicación
  int pwm_output_int = (int)pwm_output_float;
  setPWM(pwm_output_int);
}

/*------------------- SETUP -------------------*/
void setup() {
  Serial.begin(115200);
  delay(8000); 

  // I2C y ADS1115
  Wire.begin(SDA_PIN, SCL_PIN);
  if (!ads.begin()) {
    Serial.println("ERROR: No se encontró el ADS1115");
    while (1) { delay(1000); }
  }
  ads.setGain(GAIN_ONE); 

  // PWM ESP32
  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(pinPWM, PWM_CHANNEL);

  setPWM(PWM_BASE);

  // Tiempos iniciales y reseteo del integrador
  tExperiment0 = millis();
  tLastSample  = tExperiment0;
  tControlStart = 0;
  integral_sum = 0.0f; 

  Serial.println("t_ms,PWM,Presion_kPa");
}

/*------------------- LOOP -------------------*/
void loop() {
  unsigned long now = millis();

  // 1) Lazo de muestreo y control (se ejecuta cada SAMPLE_PERIOD_MS)
  if (now - tLastSample >= SAMPLE_PERIOD_MS) {
    tLastSample = now;

    float pres_kPa   = readPressurekPa();
    unsigned long t_ms = now - tExperiment0;

    // 2) Lógica de Control
    if (state == STATE_CONTROL) {
      runPIControl(pres_kPa);
    }
    
    // 3) Registro de Salida (I/O)
    Serial.print(t_ms);
    Serial.print(",");
    Serial.print(currentPWM);
    Serial.print(",");
    Serial.println(pres_kPa, 3);
  }

  // 4) Lógica del experimento (máquina de estados)
  switch (state) {
    case STATE_BASELINE:
      if (now - tExperiment0 >= T_BASELINE_MS) {
        tControlStart = now;
        state = STATE_CONTROL; // Activar el controlador
      }
      break;

    case STATE_CONTROL:
      if (now - tControlStart >= T_CONTROL_MS) {
        setPWM(PWM_BASE); 
        state = STATE_FINISHED; // Terminar
      }
      break;

    case STATE_FINISHED:
      // Mantener el registro de datos
      break;
  }
}