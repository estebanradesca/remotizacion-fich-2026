from fastapi import WebSocket

class ControlConexion:
    def __init__(self):
        self.websocket: WebSocket | None = None
    
    async def conectar(self, websocket: WebSocket):
        await websocket.accept()
        self.websocket = websocket
    
    def desconectar(self):
        self.websocket = None
    
    # Envío los datos desde el servidor al cliente
    async def enviar(self, msg: "str"):
        if self.websocket:
            await self.websocket.send_text(msg)

# Callback para enviar mensaje desde el servidor al cliente web
# que llega desde el arduino 
import asyncio

async def recibo_mensaje(msg: str):
    # Esto envía el mensaje a través del WebSocket
    print(f"DEBUG: Intentando enviar al WS: {msg}")
    await controlador_ws.enviar(msg)

# Si tu SocketArduino llama al callback desde un hilo distinto,
# necesitas una versión que pueda ser llamada de forma segura:
def callback_desde_socket(msg: str):
    # Obtenemos el loop de la app y le pedimos que ejecute la corrutina
    loop = asyncio.get_event_loop()
    if loop.is_running():
        loop.create_task(recibo_mensaje(msg))

controlador_ws = ControlConexion()    
