import serial
import csv
import datetime

# ===============================
# CONFIGURACIÓN
# ===============================

PORT = '/dev/ttyUSB0'   # Puerto en Linux
BAUD = 115200
OUT_FILE = 'datos_barrido.csv'

SERIAL_TIMEOUT = 2   # segundos

# ===============================
# ABRIR PUERTO SERIAL
# ===============================
print(f"📡 Abriendo puerto serial {PORT} a {BAUD} baud...")
try:
    ser = serial.Serial(PORT, BAUD, timeout=SERIAL_TIMEOUT)
except Exception as e:
    print("❌ Error al abrir el puerto serial:")
    print(e)
    exit()

# ===============================
# ABRIR CSV PARA ESCRITURA
# ===============================
print(f"📝 Guardando datos en: {OUT_FILE}")

try:
    csv_file = open(OUT_FILE, 'w', newline='')
except Exception as e:
    print("❌ Error al crear el archivo CSV:")
    print(e)
    ser.close()
    exit()

writer = None  # se crea al detectar el encabezado

print("⏳ Esperando datos del ESP32... (CTRL + C para detener)\n")

try:
    while True:
        line_bytes = ser.readline()
        if not line_bytes:
            continue

        line = line_bytes.decode('utf-8', errors='ignore').strip()
        if not line:
            continue

        # Mostrar en pantalla para monitoreo
        print(line)

        # Detectar encabezado CSV
        if line.startswith("t_ms"):
            header = line.split(',')
            writer = csv.writer(csv_file)
            writer.writerow(header)
            print("📌 Encabezado detectado, iniciando registro...")
            continue

        # Si aún no hay encabezado, esperar
        if writer is None:
            print("⚠️ Aún no se detecta encabezado CSV. Ignorando línea.")
            continue

        # Escribir línea de datos
        row = line.split(',')
        writer.writerow(row)

except KeyboardInterrupt:
    print("\n⛔ Registro detenido manualmente.")

finally:
    csv_file.close()
    ser.close()
    print("📁 Archivo CSV guardado y puerto cerrado.")
