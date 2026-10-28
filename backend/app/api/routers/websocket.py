
from fastapi import APIRouter, WebSocket, Depends, WebSocketDisconnect
from fastapi.responses import Response

from app.api.dependencies import obtener_sockets, obtener_controlador
from app.api.ws.ws_control import ControlConexion
from app.services.general import (
    enviar_comando_al_arduino,
    inicio_conexion_equipo,
    fin_conexion_equipo,
)

router = APIRouter(
    prefix="/ws",
    tags=["websocket"],
)


@router.websocket("/{id_equipo}")
async def websocket_equipo(
    websocket: WebSocket,
    id_equipo: int,
    sockets: dict = Depends(obtener_sockets),
    controlador: ControlConexion = Depends(obtener_controlador),
):
    # Seleccionar las conexiones correspondientes al equipo.
    if id_equipo == 1:
        socket_arduino = sockets.get((1, None))
        sockets_equipo = socket_arduino

    elif id_equipo == 2:
        socket_valvulas = sockets.get((2, "valvulas"))
        socket_motores = sockets.get((2, "motores"))

        sockets_equipo = (socket_valvulas, socket_motores)

    else:
        await websocket.close(code=1008, reason="Equipo no válido")
        return

    # Comprobar que las conexiones necesarias existan y estén activas.
    if id_equipo == 1:
        conexiones_validas = (
            socket_arduino is not None
            and socket_arduino.escuchando
        )
    else:
        conexiones_validas = (
            socket_valvulas is not None
            and socket_valvulas.escuchando
            and socket_motores is not None
            and socket_motores.escuchando
        )

    if not conexiones_validas:
        await websocket.close(code=1013, reason="Arduino no conectado")
        return

    # Solo un administrador por equipo.
    if controlador.websocket_admin.get(id_equipo) is not None:
        await websocket.close(code=1008, reason="Equipo ocupado")
        return

    await controlador.conectar(id_equipo, websocket)

    try:
        await inicio_conexion_equipo(id_equipo, sockets_equipo)

        while True:
            comando = await websocket.receive_text()

            await enviar_comando_al_arduino(
                id_equipo,
                comando,
                sockets_equipo,
            )

    except WebSocketDisconnect:
        print(f"Se desconectó el administrador del equipo {id_equipo}.")

    finally:
        controlador.desconectar(id_equipo, websocket)

        try:
            await fin_conexion_equipo(id_equipo, sockets_equipo)
        except Exception as error:
            print(
                f"Error al finalizar la conexión del equipo "
                f"{id_equipo}: {error}"
            )
