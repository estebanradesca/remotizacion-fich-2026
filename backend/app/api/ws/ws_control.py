from fastapi import WebSocket
import asyncio
from app.services.equipos import procesar_mensaje_de_arduino

class ControlConexion:
    def __init__(self):
        self.websocket_clientes: list[WebSocket] = [] # Es una lista ya que pueden ser varios clientes
    
    # Conecta el cliente 
    async def conectar(self, websocket: WebSocket):
        await websocket.accept()
        print(websocket)
        self.websocket_clientes.append(websocket)
        print(f"Cliente conectado. Total: {len(self.websocket_clientes)}")
    
    # Desconecta al cliente
    def desconectar(self, websocket: WebSocket):
        if websocket in self.websocket_clientes:
            self.websocket_clientes.remove(websocket)
            print(f"Cliente eliminado de la lista de clientes. Total: {len(self.websocket_clientes)}")
    
    # Envío los datos desde el servidor a los clientes
    async def difundir(self, mensaje: dict):      
        for cliente in self.websocket_clientes:
            try:
                await cliente.send_json(mensaje)
                print("Se envió el mensaje :", mensaje)
            except:
                print(f"No se pudo enviar el mensaje al cliente {cliente}, eliminando...")
                self.desconectar(cliente)

# Instancia global del controlador del websocket 
controlador_ws = ControlConexion()    

# Callback para enviar mensaje a los clientes web que llega al servidor desde el Arduino  
async def recibo_mensaje_de_arduino(datos: bytes):
    datos = datos.strip()
    mensaje = datos.decode("utf-8")
    mensaje_procesado = procesar_mensaje_de_arduino(mensaje) 
    await controlador_ws.difundir(mensaje_procesado)