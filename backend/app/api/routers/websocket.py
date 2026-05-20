from fastapi import APIRouter, WebSocket, Depends
from app.api.dependencies import obtener_socket, obtener_controlador
from app.infra.tcp_arduino import SocketArduino
from app.api.ws.ws_control import ControlConexion
from app.services.equipos import enviar_comando_al_arduino

router = APIRouter(
    prefix="/ws",
    tags=["websocket"]
)

@router.websocket("/")
async def websocket_equipo(
    websocket: WebSocket,
    socket_arduino: SocketArduino = Depends(obtener_socket),
    controlador: ControlConexion = Depends(obtener_controlador)):

    # Conecto a la instancia global del controlador del websocket
    await controlador.conectar(websocket)

    # Recibo un comando desde el cliente y lo envío al arduino
    try: 
        while True:
            comando = await websocket.receive_text()
            print(comando)
            await enviar_comando_al_arduino(comando, socket_arduino)
    # Si algo falla desconecto al cliente
    except: 
        controlador.desconectar(websocket)