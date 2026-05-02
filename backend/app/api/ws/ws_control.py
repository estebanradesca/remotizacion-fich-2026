from fastapi import WebSocket
import asyncio

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
    async def difundir(self, mensaje: str):
        for cliente in self.websocket_clientes:
            try:
                await cliente.send_text(mensaje)
            except:
                print(f"No se puedo enviar el mensaje al cliente {cliente}")

# Instancia global del controlador del websocket 
controlador_ws = ControlConexion()    

# Callback para enviar mensaje a los clientes web que llega al servidor desde el Arduino  
async def recibo_mensaje_de_arduino(datos: bytes):

    mensaje = datos.decode("utf-8")

    await controlador_ws.difundir(mensaje)