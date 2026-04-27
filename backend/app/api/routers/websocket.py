from fastapi import APIRouter, WebSocket, Depends
from app.api.dependencies import obtener_arduino
from app.infra.tcp_arduino import SocketArduino
from app.api.ws.ws_control import controlador_ws
from app.services.equipos_servicio import enviar_mensaje, enviar_mensaje_ws

router = APIRouter()

@router.websocket("/ws")
async def websocket_equipo(
    websocket: WebSocket, 
    arduino: SocketArduino = Depends(obtener_arduino)
    ):
    await controlador_ws.conectar(websocket)
    # Recibo un comando desde el cliente y lo envío al arduino
    try: 
        while True:
            data = await websocket.receive_text()
            await enviar_mensaje_ws(data, arduino)
    except: 
        controlador_ws.desconectar()