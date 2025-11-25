// ===============================================================
//  micro_ros_bomba_dual_PI.ino
//  Control de bomba neumática con micro-ROS en ESP32
//
//  - Lee presión con ADS1115
//  - Control PI para:
//        * Presión positiva  (Kp=5.6845,   Ki=113.4116)
//        * Presión negativa  (Kp=-200.33,  Ki=-1670.3)
//  - Selección de modo vía tópico:
//        /pressure_mode : std_msgs/Int8  (-1 = vacío, 0 = apagado, +1 = presión)
//  - Setpoint de presión vía tópico:
//        /pressure_setpoint : std_msgs/Float32 (kPa)
//  - Publica presión medida en:
//        /pressure_feedback : std_msgs/Float32 (kPa)
//
//  Notas:
//  * Periodo de muestreo: 10 ms (100 Hz), consistente con tus TS_CONTROLLER.
//  * Cada vez que cambia de modo, se resetean los integradores.
// ===============================================================

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>

// micro-ROS
#include <micro_ros_arduino.h>
#include <rcl/rcl.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>

#include <std_msgs/msg/float32.h>
#include <std_msgs/msg/int8.h>

// ---------- I2C / ADC ----------
Adafruit_ADS1115 ads;

const int SDA_PIN = 21;
const int SCL_PIN = 22;

// ---------- CONFIGURACIÓN HARDWARE ----------
const int PIN_PWM_BOMBA   = 18;  // PWM de la bomba (ESP32)
const int PIN_VALVE_POS   = 25;  // válvula hacia salida de presión
const int PIN_VALVE_NEG   = 26;  // válvula hacia salida de vacío

// ---------- CONFIG PWM ESP32 ----------
const int PWM_CHANNEL     = 0;
const int PWM_FREQ        = 1000;      // Hz (igual que en tus controladores)
const int PWM_RESOLUTION  = 8;         // bits → 0–255
const int PWM_MAX         = 255;
const int PWM_MIN         = 0;

int currentPWM = 0; // Se actualiza con la salida del controlador

// ===========================================================
//  CONTROLADOR PI POSITIVO (TU CÓDIGO ADAPTADO)
// ===========================================================
// Kp: 5.6845, Ki: 113.4116
const float KP_POS_CONTROLLER = 5.6845f;
const float KI_POS_CONTROLLER = 113.4116f;

// Tiempo de muestreo (10 ms = 0.01 s).
const float TS_POS_CONTROLLER = 0.010f; 

float integral_pos_sum = 0.0f; 

// ===========================================================
//  CONTROLADOR PI NEGATIVO (TU CÓDIGO EXACTO ADAPTADO)
// ===========================================================
// Kp: -200.33, Ki: -1670.3
// NOTA: ganancias negativas para compensar la ganancia de planta negativa.
const float KP_NEG_CONTROLLER = -200.33f;
const float KI_NEG_CONTROLLER = -1670.3f;

// Tiempo de muestreo (10 ms = 0.01 s).
const float TS_NEG_CONTROLLER = 0.010f; 

float integral_neg_sum = 0.0f; 

// ===========================================================
//  MODELO DEL SENSOR DE PRESIÓN (igual que tus códigos)
// ===========================================================
const float V_OFFSET  = 2.5f;   // Voltaje a 0 kPa
const float V_PER_kPa = 0.02f;  // Volt/ kPa

// ===========================================================
//  PARÁMETROS DE CONTROL GLOBAL
// ===========================================================
const float Ts_ms = 10.0f;                 // 10 ms → 100 Hz
unsigned long last_control_ms = 0;

// Setpoint y modo (desde ROS2)
float  setpoint_kPa = 0.0f;   // Se actualiza por /pressure_setpoint
int8_t mode_control = 0;      // -1: vacío, 0: apagado, +1: presión

float last_pressure_kPa = 0.0f;

// ===========================================================
//  micro-ROS: estructuras básicas
// ===========================================================
rcl_allocator_t allocator;
rclc_support_t  support;
rcl_node_t      node;
rclc_executor_t executor;

rcl_subscription_t sub_setpoint;
rcl_subscription_t sub_mode;
rcl_publisher_t    pub_feedback;

std_msgs__msg__Float32 msg_setpoint;
std_msgs__msg__Int8    msg_mode;
std_msgs__msg__Float32 msg_feedback;

// ===========================================================
// LECTURA DE PRESIÓN (ADS1115)
// ===========================================================
float readPressure_kPa()
{
  int16_t adc = ads.readADC_SingleEnded(0);  
  float volts = ads.computeVolts(adc);
  float p_kPa = (volts - V_OFFSET) / V_PER_kPa;
  return p_kPa;
}

// ===========================================================
// SALIDA PWM A LA BOMBA (setPWM de tus códigos)
// ===========================================================
void writePumpPWM(int pwmValue)
{
  if (pwmValue < PWM_MIN)   pwmValue = PWM_MIN;
  if (pwmValue > PWM_MAX)   pwmValue = PWM_MAX;

  currentPWM = pwmValue;
  ledcWrite(PWM_CHANNEL, currentPWM);
}

// ===========================================================
// LÓGICA DE VÁLVULAS SEGÚN MODO
// ===========================================================
void updateValves(int8_t mode)
{
  if (mode > 0) {
    // Presión positiva: abre válvula de presión, cierra la de vacío
    digitalWrite(PIN_VALVE_POS, HIGH);
    digitalWrite(PIN_VALVE_NEG, LOW);
  } else if (mode < 0) {
    // Presión negativa: abre válvula de vacío, cierra la de presión
    digitalWrite(PIN_VALVE_POS, LOW);
    digitalWrite(PIN_VALVE_NEG, HIGH);
  } else {
    // Apagado: ambas cerradas
    digitalWrite(PIN_VALVE_POS, LOW);
    digitalWrite(PIN_VALVE_NEG, LOW);
  }
}

// ===========================================================
// CONTROLADOR PI POSITIVO (runPIControl adaptado)
// ===========================================================
float PI_pos_step(float setpoint, float currentPressure)
{
  // 1. Cálculo del Error
  float error = setpoint - currentPressure;

  // 2. Cálculo de la Acción Integral
  integral_pos_sum += error * TS_POS_CONTROLLER;

  // --- Anti-windup Básico (igual que tu versión positiva) ---
  float predicted_pwm = (KP_POS_CONTROLLER * error) + (KI_POS_CONTROLLER * integral_pos_sum);
  if ((predicted_pwm > PWM_MAX && error > 0.0f) || 
      (predicted_pwm < 0       && error < 0.0f)) {
    // Revertir el último paso de integración
    integral_pos_sum -= error * TS_POS_CONTROLLER; 
  }

  // 3. Cálculo de la Salida de Control (PWM)
  float pwm_output_float = (KP_POS_CONTROLLER * error) + (KI_POS_CONTROLLER * integral_pos_sum);

  return pwm_output_float;  // la saturación final la hace writePumpPWM()
}

// ===========================================================
// CONTROLADOR PI NEGATIVO (runPIControl negativo adaptado)
// ===========================================================
float PI_neg_step(float setpoint, float currentPressure)
{
  // 1. Cálculo del Error
  float error = setpoint - currentPressure; 

  // 2. Cálculo de la Acción Integral
  integral_neg_sum += error * TS_NEG_CONTROLLER;

  // --- Anti-windup Básico EXACTO de tu código negativo ---
  float predicted_pwm = (KP_NEG_CONTROLLER * error) + (KI_NEG_CONTROLLER * integral_neg_sum);
  if ((predicted_pwm > PWM_MAX && error * KP_NEG_CONTROLLER > 0) || 
      (predicted_pwm < 0       && error * KP_NEG_CONTROLLER < 0)) {
    // Revertir el último paso de integración
    integral_neg_sum -= error * TS_NEG_CONTROLLER; 
  }
  
  // 3. Cálculo de la Salida de Control (PWM)
  float pwm_output_float = (KP_NEG_CONTROLLER * error) + (KI_NEG_CONTROLLER * integral_neg_sum);

  return pwm_output_float;  // saturación en writePumpPWM()
}

// ===========================================================
// CALLBACKS DE micro-ROS
// ===========================================================
void setpoint_callback(const void * msgin)
{
  const std_msgs__msg__Float32 * msg = (const std_msgs__msg__Float32 *)msgin;
  setpoint_kPa = msg->data;
}

void mode_callback(const void * msgin)
{
  const std_msgs__msg__Int8 * msg = (const std_msgs__msg__Int8 *)msgin;
  int8_t new_mode = msg->data;

  if (new_mode != mode_control) {
    // Reset integradores al cambiar de modo para evitar "golpes"
    integral_pos_sum = 0.0f;
    integral_neg_sum = 0.0f;
  }

  mode_control = new_mode;
}

// ===========================================================
// SETUP micro-ROS
// ===========================================================
void setup_microros()
{
  // Transportes de micro-ROS (Serial, WiFi, etc.)
//set_microros_wifi_transports("SSID", "PASS", "AGENT_IP", 8888);
  set_microros_transports();   // usa el que tengas configurado (por ej. Serial)

  allocator = rcl_get_default_allocator();

  // support
  rclc_support_init(&support, 0, NULL, &allocator);

  // node
  rclc_node_init_default(
    &node,
    "bomba_dual_pi_node",
    "",
    &support
  );

  // subscriber setpoint
  rclc_subscription_init_default(
    &sub_setpoint,
    &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Float32),
    "pressure_setpoint"
  );

  // subscriber mode
  rclc_subscription_init_default(
    &sub_mode,
    &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int8),
    "pressure_mode"
  );

  // publisher feedback
  rclc_publisher_init_default(
    &pub_feedback,
    &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Float32),
    "pressure_feedback"
  );

  // executor
  rclc_executor_init(&executor, &support.context, 2, &allocator);
  rclc_executor_add_subscription(
    &executor,
    &sub_setpoint,
    &msg_setpoint,
    &setpoint_callback,
    ON_NEW_DATA
  );
  rclc_executor_add_subscription(
    &executor,
    &sub_mode,
    &msg_mode,
    &mode_callback,
    ON_NEW_DATA
  );
}

// ===========================================================
// SETUP ARDUINO
// ===========================================================
void setup()
{
  Serial.begin(115200);
  delay(2000);

  // I2C y ADS1115
  Wire.begin(SDA_PIN, SCL_PIN);
  if (!ads.begin()) {
    Serial.println("ERROR: No se encontró el ADS1115");
    while (1) { delay(1000); }
  }
  ads.setGain(GAIN_ONE); 

  // Pines de válvulas
  pinMode(PIN_VALVE_POS, OUTPUT);
  pinMode(PIN_VALVE_NEG, OUTPUT);
  digitalWrite(PIN_VALVE_POS, LOW);
  digitalWrite(PIN_VALVE_NEG, LOW);

  // PWM ESP32
  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(PIN_PWM_BOMBA, PWM_CHANNEL);
  writePumpPWM(0);  // PWM en reposo

  // micro-ROS
  setup_microros();

  last_control_ms = millis();

  Serial.println("micro-ROS bomba dual PI listo (modo positivo y negativo integrados).");
}

// ===========================================================
// LOOP PRINCIPAL
// ===========================================================
void loop()
{
  // Ejecutar micro-ROS (callbacks de tópicos)
  rclc_executor_spin_some(&executor, RCL_MS_TO_NS(5));

  // Control con periodo fijo Ts_ms (10 ms)
  unsigned long now = millis();
  if (now - last_control_ms >= (unsigned long)Ts_ms) {
    last_control_ms += (unsigned long)Ts_ms;

    // 1) Leer presión
    float p_meas = readPressure_kPa();
    last_pressure_kPa = p_meas;

    // 2) Actualizar válvulas según modo
    updateValves(mode_control);

    // 3) Calcular PWM según modo
    int pwm_cmd = 0;

    if (mode_control > 0) {
      // ----- CONTROL PI POSITIVO -----
      float pwm_f = PI_pos_step(setpoint_kPa, p_meas);
      pwm_cmd = (int)pwm_f;

    } else if (mode_control < 0) {
      // ----- CONTROL PI NEGATIVO -----
      float pwm_f = PI_neg_step(setpoint_kPa, p_meas);
      pwm_cmd = (int)pwm_f;

    } else {
      // ----- APAGADO -----
      pwm_cmd = 0;
    }

    // 4) Escribir PWM con saturación
    writePumpPWM(pwm_cmd);

    // 5) Publicar presión medida
    msg_feedback.data = p_meas;
    rcl_publish(&pub_feedback, &msg_feedback, NULL);
  }
}
