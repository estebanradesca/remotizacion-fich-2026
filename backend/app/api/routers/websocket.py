from fastapi import APIRouter, WebSocket, Depends, WebSocketDisconnect
from fastapi.responses import JSONResponse, Response
from app.api.dependencies import obtener_socket, obtener_controlador
from app.infra.tcp_arduino import SocketArduino
from app.api.ws.ws_control import ControlConexion
from app.services.reynolds import enviar_comando_al_arduino, deshabilitar_motores

router = APIRouter(
    prefix="/ws",
    tags=["websocket"]
)

@router.websocket("/{id_equipo}")
async def websocket_equipo(
    websocket: WebSocket,
    id_equipo: int,
    socket_arduino: SocketArduino = Depends(obtener_socket),
    controlador: ControlConexion = Depends(obtener_controlador)):

    # Si no se inicio la conexión con el arduino no lo abro
    # Si ya hay un administrador no permite conectarse
    if (socket_arduino.escuchando == False) or (controlador.websocket_admin[id_equipo] is not None):
        await websocket.send_denial_response(Response(status_code=401, content="Unauthorized"))
        return 

    
    await controlador.conectar(id_equipo, websocket)    

    # Recibo un comando desde el cliente y lo envío al arduino
    try:
        while True:
            comando = await websocket.receive_text()
            await enviar_comando_al_arduino(comando, socket_arduino)
                    
    # Si algo falla desconecto al cliente
    except WebSocketDisconnect:
        print(f"Se desconectó el administrador del equipo.")
        controlador.desconectar(id_equipo, websocket)
        await deshabilitar_motores(socket_arduino)
        