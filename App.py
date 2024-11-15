"""
Desarrollado por:
Arturo López García y Diego Gerardo Sánchez Moreno
Descripción:
Este programa implementa una interfaz gráfica para interactuar con un servidor
que controla un dispositivo Arduino.
"""

import socket
import customtkinter

# ================================ CONFIGURACIÓN DE LA GUI ======================================

GUI_HOST = 'localhost'  # Dirección del servidor
GUI_PORT = 65432  # Puerto del servidor

# Configuración de la apariencia de la interfaz gráfica
customtkinter.set_appearance_mode("dark")
customtkinter.set_default_color_theme("dark-blue")

# Inicialización de la ventana principal
root = customtkinter.CTk()
root.title('Locomotion for soft crawling robots')
root.geometry('1000x700')  # Tamaño de la ventana

# =============================== FUNCIONES DE LA GUI ===========================================

def Encender():
    """
    Envía el comando 'Encender' al servidor.
    """
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as client_socket:
        client_socket.connect((GUI_HOST, GUI_PORT))
        message = "Encender"
        client_socket.sendall(message.encode())

def Apagar():
    """
    Envía el comando 'Apagar' al servidor.
    """
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as client_socket:
        client_socket.connect((GUI_HOST, GUI_PORT))
        message = "Apagar"
        client_socket.sendall(message.encode())

# =============================== COMPONENTES DE LA GUI =========================================

Titulo = customtkinter.CTkLabel(root, text="Locomotion for soft crawling robots", font=("Nunito Bold", 40))
Titulo.pack(pady=30)

boton_encender = customtkinter.CTkButton(root, text="Encender", command=Encender)
boton_encender.pack(pady=20)

boton_apagar = customtkinter.CTkButton(root, text="Apagar", command=Apagar)
boton_apagar.pack(pady=20)

# =============================== EJECUCIÓN DE LA GUI ===========================================

if __name__ == "__main__":
    root.mainloop()
