
import socket
import os
from dotenv import load_dotenv
import asyncio

load_dotenv()


class SocketArduino:
    def __init__(self, id_equipo: int, conexion: str | None = None):
        self.id_equipo = id_equipo
        self.conexion = conexion

        # Configuración habitual para los equipos existentes.
        if conexion is None:
            self.HOST_ARDUINO = os.getenv("HOST_ARDUINO")
            puerto = os.getenv("PUERTO_ARDUINO")

        # Configuración independiente para golpe de ariete.
        elif id_equipo == 2 and conexion == "valvulas":
            self.HOST_ARDUINO = os.getenv("HOST_GOLPE_VALVULAS")
            puerto = os.getenv("PUERTO_GOLPE_VALVULAS")

        elif id_equipo == 2 and conexion == "motores":
            self.HOST_ARDUINO = os.getenv("HOST_GOLPE_MOTORES")
            puerto = os.getenv("PUERTO_GOLPE_MOTORES")

        else:
            raise ValueError(
                f"Configuración de conexión inválida: "
                f"id_equipo={id_equipo}, conexion={conexion}"
            )

        if not self.HOST_ARDUINO or not puerto:
            raise ValueError(
                f"Falta configurar HOST o PUERTO para {conexion or id_equipo}"
            )

        self.PUERTO_ARDUINO = int(puerto)
        self.socket_arduino = None
        self.buffer = b""
        self.escuchando = False
        self.callback = lambda x, y: asyncio.sleep(0)
        self.lock = asyncio.Lock()

    def conectar(self):
        if self.socket_arduino is not None:
            return

        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

        try:
            sock.connect((self.HOST_ARDUINO, self.PUERTO_ARDUINO))
            self.socket_arduino = sock
            self.escuchando = True

            print(
                f"Conectado al Arduino {self.conexion or self.id_equipo}: "
                f"{self.HOST_ARDUINO}:{self.PUERTO_ARDUINO}"
            )

        except Exception as error:
            sock.close()
            raise ConnectionError(
                f"Error conectando al Arduino "
                f"{self.conexion or self.id_equipo}: {error}"
            ) from error

    def cerrar(self):
        self.escuchando = False

        if self.socket_arduino is not None:
            try:
                self.socket_arduino.shutdown(socket.SHUT_RDWR)
            except OSError:
                pass

            self.socket_arduino.close()
            self.socket_arduino = None

            print(
                f"Conexión cerrada: {self.conexion or self.id_equipo}"
            )

    async def enviar(self, datos: bytes):
        if self.socket_arduino is None:
            raise ConnectionError("Socket no conectado")

        try:
            async with self.lock:
                await asyncio.to_thread(
                    self.socket_arduino.sendall, datos
                )
                print(self.socket_arduino)
                print("se envioo")
                print(datos)
        except OSError as error:
            raise ConnectionError(
                f"Error enviando datos al Arduino: {error}"
            ) from error


    async def recibir(self):
        if self.socket_arduino is None:
            raise ConnectionError("Socket no conectado")

        while self.escuchando:
            try:
                datos = await asyncio.to_thread(
                    self.socket_arduino.recv, 1024
                )
            except OSError:
                if self.escuchando:
                    raise
                break

            if not datos:
                print(
                    f"Arduino desconectado: "
                    f"{self.conexion or self.id_equipo}"
                )
                break

            print(f"[{self.conexion}] Bytes recibidos: {datos!r}")
            self.buffer += datos

            while b"\n" in self.buffer:
                linea, self.buffer = self.buffer.split(b"\n", 1)
                linea = linea.rstrip(b"\r")

                if not linea:
                    continue

                print(f"[{self.conexion}] Mensaje completo: {linea!r}")

                try:
                    await self.callback(linea, self.id_equipo)
                except Exception as error:
                    print(f"Error en callback: {error!r}")

        self.escuchando = False

    def funcion_callback(self, funcion):
        self.callback = funcion
