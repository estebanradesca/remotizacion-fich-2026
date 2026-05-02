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
        self.buffer = b"" # Buffer de recepción
        self.escuchando = False
        self.callback = lambda x: asyncio.sleep(0)
        self.lock = asyncio.Lock()

    # Conexión con el módulo del Arduino
    def conectar(self):
        # Si el socket ya está conectado, no hago nada
        if self.socket_arduino is not None:
            return  
        try:
            # Creo el socket
            self.socket_arduino = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            # Conecto con el socket del módulo del Arduino
            self.socket_arduino.connect((self.HOST_ARDUINO, self.PUERTO_ARDUINO))
            print(f"Conectado al módulo del Arduino con dirección: {self.HOST_ARDUINO}:{self.PUERTO_ARDUINO}")
            self.escuchando = True

        except Exception as error:
            self.socket_arduino = None
            raise Exception(f"Error conectando al módulo del Arduino: {error}")

    # Cierre de la conexión con el módulo
    def cerrar(self):
        if self.socket_arduino is not None:
            self.socket_arduino.close()
            self.socket_arduino = None
            self.escuchando = False
            print("Conexión cerrada con el módulo del Arduino")
    
    # Envío datos desde el servidor al módulo Arduino a través del socket
    async def enviar(self, datos: bytes):
        if self.socket_arduino is None:
            raise Exception("Socket no conectado")
        try:
            # Bloqueo el acceso al socket con el arduino mientras envío todos los datos
            async with self.lock:
                await asyncio.to_thread(self.socket_arduino.sendall, datos)
        except Exception as error:
            raise Exception(f"Error enviando los datos: {error}")
    
    # Recibo todo lo que llega desde el módulo
    async def recibir(self):
        if self.socket_arduino is None:
                raise Exception("Socket no conectado")

        while self.escuchando:
            datos = await asyncio.to_thread(self.socket_arduino.recv, 1024)

            # Cuando se desconecta el módulo se recibe b"" a través de recv,
            # por eso utilizo el if de esta forma
            if not datos:  
                print("Módulo del Arduino desconectado, no se están recibiendo datos desde el Arduino")
                break 
            
            self.buffer += datos

            while b"\n" in self.buffer:
                texto, self.buffer = self.buffer.split(b"\n", 1)
                # El mensaje viene en bytes y lo mando para que se procese
                # y se envíe por el websocket a través de la función callback 
                await self.callback(texto)
    
    # La funcion callback me va a servir para procesar el mensaje 
    # y enviar por el websocket lo que viene desde el módulo del 
    # Arduino

    ### Podría poner el tipo función

    def funcion_callback(self, fun):
        self.callback = fun