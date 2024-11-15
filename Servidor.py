"""
Desarrollado por:
Arturo López García y Diego Gerardo Sánchez Moreno
Descripción:
Este programa implementa un servidor que controla un dispositivo Arduino
a través de comunicación serial.
"""

import socket
import threading
import serial
import serial.tools.list_ports
import time

# ================================ CONFIGURACIÓN DEL SERVIDOR ====================================

HEADER = 64  # Tamaño fijo del encabezado para mensajes
PORT = 65432  # Puerto donde el servidor escucha
HOST = '0.0.0.0'  # Escucha en todas las interfaces de la máquina
ADDR = (HOST, PORT)  # Dirección completa del servidor
FORMAT = 'utf-8'  # Codificación de los mensajes
DISCONNECT_MESSAGE = "!DISCONNECT"  # Mensaje para desconectar clientes

# =============================== FUNCIONES DEL SERVIDOR ========================================

def find_serial_ports():
    """
    Encuentra y lista los puertos seriales disponibles en la máquina.
    :return: Lista de nombres de los puertos seriales disponibles.
    """
    ports = serial.tools.list_ports.comports()
    return [port.device for port in ports]

def setup_serial_connection():
    """
    Configura una conexión serial con el primer puerto disponible.
    :return: Objeto de conexión serial si tiene éxito, None en caso contrario.
    """
    ports = find_serial_ports()
    if ports:
        selected_port = ports[0]
        print("Conectando al puerto:", selected_port)
        try:
            arduino = serial.Serial(selected_port, 9600)
            time.sleep(2)  # Esperar a que el puerto esté listo
            print("Conexión establecida con el Arduino en", selected_port)
            return arduino
        except serial.SerialException as e:
            print("Error al conectar al puerto:", e)
            return None
    else:
        print("No se encontraron puertos seriales disponibles.")
        return None

# Inicialización de la conexión serial
arduino = setup_serial_connection()

def handle_client(conn, addr):
    """
    Maneja la comunicación con un cliente en un hilo separado.
    :param conn: Objeto de conexión del cliente.
    :param addr: Dirección del cliente.
    """
    print("Conectado a:", addr)
    connected = True
    while connected:
        try:
            # Recibir el tamaño del mensaje
            msg_length = conn.recv(HEADER).decode(FORMAT)
            if msg_length:
                msg_length = int(msg_length)
                data = conn.recv(msg_length).decode(FORMAT)

                if data == DISCONNECT_MESSAGE:
                    connected = False
                    print(f"Cliente {addr} desconectado.")
                    break

                command = data.strip()
                print("Recibido de {}: {}".format(addr, command))

                # Procesar comandos y enviarlos al Arduino
                if arduino:
                    if command == "Encender":
                        arduino.write(b'ON')
                        print("Comando enviado al Arduino: ENCENDER")
                    elif command == "Apagar":
                        arduino.write(b'OFF')
                        print("Comando enviado al Arduino: APAGAR")
                    else:
                        print("Comando no reconocido.")
            else:
                break
        except Exception as e:
            print(f"Error en la conexión con {addr}: {e}")
            break

    conn.close()
    print("Conexión cerrada con", addr)

def start_server():
    """
    Inicia el servidor y acepta conexiones de clientes.
    """
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.bind(ADDR)
    server_socket.listen()
    print("Servidor escuchando en {}:{}".format(HOST, PORT))

    while True:
        try:
            conn, addr = server_socket.accept()
            client_thread = threading.Thread(target=handle_client, args=(conn, addr))
            client_thread.start()
        except KeyboardInterrupt:
            break

    server_socket.close()
    print("Servidor apagado.")

# =============================== EJECUCIÓN DEL SERVIDOR ========================================

if __name__ == "__main__":
    start_server()
