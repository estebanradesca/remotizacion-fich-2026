from fastapi import WebSocket
import asyncio
from app.services.reynolds import procesar_mensaje_de_arduino

class ControlConexion:
    def __init__(self):
        # Es una lista de los adminstradores de los 3 equipos
        self.websocket_admin: list[WebSocket | None] = [None] * 3 

    # Conecta el administrador del equipo 
    async def conectar(self, id_equipo: int, websocket: WebSocket):
        await websocket.accept()
        self.websocket_admin[id_equipo] = websocket 
        print(f"Administrador conectado al equipo número {id_equipo}")
    
    # Desconecta al administrador del equipo
    def desconectar(self, id_equipo: int, websocket: WebSocket):
        self.websocket_admin[id_equipo] = None
        print(f"Administrador desconectado del equipo número {id_equipo}")
    
    # Envío los datos desde el servidor al administrador
    async def difundir(self, mensaje: dict, id_equipo: int):
        cliente = self.websocket_admin[id_equipo] 
        if cliente is None: 
            return
        try:
            await cliente.send_json(mensaje) 
        except Exception as error:
            print(f"No se pudo enviar el mensaje al administrador {cliente} del equipo número {id_equipo}, removiendo... Error: {error}")
            self.desconectar(id_equipo, cliente)

# Instancia global del controlador del websocket 
controlador_ws = ControlConexion()    

# Callback para enviar mensaje a los clientes web que llega al servidor desde el Arduino  
async def recibo_mensaje_de_arduino(datos: bytes, id_equipo):
    datos = datos.strip()
    mensaje = datos.decode("utf-8")
    mensaje_procesado = procesar_mensaje_de_arduino(mensaje)
    if mensaje_procesado is None:
        return
    await controlador_ws.difundir(mensaje_procesado, id_equipo)
    