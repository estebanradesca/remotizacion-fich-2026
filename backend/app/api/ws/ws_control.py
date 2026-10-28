
from fastapi import WebSocket

from app.services.general import procesar_mensaje_de_arduino


class ControlConexion:
    def __init__(self):
        # Un administrador WebSocket por equipo.
        # Los equipos pueden tener varias conexiones TCP.
        self.websocket_admin: dict[int, WebSocket] = {}

    async def conectar(self, id_equipo: int, websocket: WebSocket):
        await websocket.accept()

        self.websocket_admin[id_equipo] = websocket

        print(
            f"Administrador conectado al equipo número {id_equipo}"
        )

    def desconectar(self, id_equipo: int, websocket: WebSocket):
        # Solo desconecta si sigue siendo el mismo WebSocket.
        # Evita borrar una conexión nueva por el cierre de una anterior.
        if self.websocket_admin.get(id_equipo) is websocket:
            self.websocket_admin.pop(id_equipo, None)

            print(
                f"Administrador desconectado del equipo número "
                f"{id_equipo}"
            )

    async def difundir(self, mensaje: dict, id_equipo: int):
        cliente = self.websocket_admin.get(id_equipo)

        if cliente is None:
            return

        try:
            await cliente.send_json(mensaje)

        except Exception as error:
            print(
                f"No se pudo enviar el mensaje al administrador "
                f"del equipo {id_equipo}. Error: {error}"
            )

            self.desconectar(id_equipo, cliente)


# Instancia global del controlador WebSocket.
controlador_ws = ControlConexion()


# Callback para recibir mensajes de los Arduinos y reenviarlos
# al administrador WebSocket correspondiente.
async def recibo_mensaje_de_arduino(
    datos: bytes,
    id_equipo: int,
    conexion: str | None = None,
):
    try:
        mensaje = datos.decode("utf-8").strip()

    except UnicodeDecodeError as error:
        print(
            f"No se pudo decodificar un mensaje del equipo "
            f"{id_equipo}: {error}"
        )
        return

    if not mensaje:
        return

    try:
        mensaje_procesado = procesar_mensaje_de_arduino(
            mensaje,
            id_equipo,
            conexion=conexion,
        )

    except Exception as error:
        print(
            f"Error procesando mensaje del equipo {id_equipo}, "
            f"conexión {conexion}: {error}"
        )
        return

    if mensaje_procesado is None:
        return

    await controlador_ws.difundir(
        mensaje_procesado,
        id_equipo,
    )
