import socket
import os
from dotenv import load_dotenv
import asyncio

load_dotenv()

class SocketArduino:
    def __init__(self):
        self.HOST_ARDUINO = os.getenv("HOST_ARDUINO")
        self.PUERTO_ARDUINO = int(os.getenv("PUERTO_ARDUINO"))
        self.socket_arduino = None
        self.buffer = b""
        self.escuchando = True
        self.callback = None

    def conectar(self):
        if self.socket_arduino is not None:
            return  
        try:
            self.socket_arduino = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.socket_arduino.connect((self.HOST_ARDUINO, self.PUERTO_ARDUINO))
            print(f"Conectado a {self.HOST_ARDUINO}:{self.PUERTO_ARDUINO}")

        except Exception as e:
            self.socket_arduino = None
            raise Exception(f"Error conectando al Arduino: {e}")

    def cerrar(self):

        if self.socket_arduino is not None:
            self.socket_arduino.close()
            self.socket_arduino = None
            self.escuchando = False
            print("Conexión cerrada")

    async def enviar(self, msg: bytes):
        if self.socket_arduino is None:
            raise Exception("Socket no conectado")
        await asyncio.to_thread(self.socket_arduino.sendall, msg)

    async def escuchar(self):
        if self.socket_arduino is None:
                raise Exception("Socket no conectado")

        while self.escuchando:
            datos = await asyncio.to_thread(self.socket_arduino.recv, 1024)
            if not datos: 
                print("Arduino desconectado")
                self.escuchando = False
                break 
            self.buffer += datos

            while b"\n" in self.buffer:
                mensaje, self.buffer = self.buffer.split(b"\n", 1)
                try: 
                    await self.callback(mensaje.decode())
                except Exception as e:
                    print("Error en callback:", e)
    
    def funcion_callback(self, callback):
        self.callback = callback