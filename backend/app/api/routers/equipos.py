from fastapi import APIRouter, Depends
from typing import Annotated
from app.api.dependencies import obtener_socket
from app.infra.tcp_arduino import SocketArduino
from app.api.schemas.equipos import CambiarEstado, ComandoControl
from app.services.equipos import enviar_mensaje

router = APIRouter(
    prefix="/equipos"
    tags="equipos"
)

SocketArduinoDep = Annotated[SocketArduino, Depends(obtener_socket)]
@router.post("/estado")
async def cambiar_estado(
    cmd: CambiarEstado, 
    socket_arduino: SocketArduinoDep,
    ):

    mensaje = await enviar_mensaje(cmd, arduino)

    return {"mensaje": mensaje}

"""
@router.post("/equipo/comando")
async def enviar_comando(
    cmd: ComandoControl, 
    arduino: SocketArduino = Depends(obtener_socket)
    ):
    
    mensaje = await enviar_mensaje(arduino, cmd)
    
    return {"mensaje": mensaje}
"""

