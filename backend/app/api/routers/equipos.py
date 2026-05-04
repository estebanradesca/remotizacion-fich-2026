from fastapi import APIRouter, Depends
from typing import Annotated

from app.api.dependencies import obtener_socket
from app.infra.tcp_arduino import SocketArduino

from app.api.schemas.equipos import CambiarEstado
from app.services.equipos import cambiar_estado_servicio

router = APIRouter(
    prefix="/equipos",
    tags=["equipos"]
)

SocketArduinoDep = Annotated[SocketArduino, Depends(obtener_socket)]

@router.post("/estado")
async def cambiar_estado(
    comando: CambiarEstado,
    socket_arduino: SocketArduinoDep
    ):
    respuesta = await cambiar_estado_servicio(comando.id_equipo, comando.estado, socket_arduino)

    return {"Respuesta": respuesta}

"""
@router.post("/pasos_agua")
async def modificar_pasos_agua(
    comando: ModificarPasosAgua,
    socket_arduino: SocketArduinoDep
    ):

    respuesta = await modificar_pasos_servicio(
        comando.id_equipo, 
        comando.pasos, 
        tipo = "agua", 
        socket_arduino = socket_arduino)

    return {"Respuesta": respuesta}

@router.post("/pasos_tinta")
async def modificar_pasos_tinta(
    comando: ModificarPasosTinta,
    socket_arduino: SocketArduinoDep
    ):

    respuesta = await modificar_pasos_servicio(
        comando.id_equipo, 
        comando.pasos, 
        tipo = "tinta", 
        socket_arduino = socket_arduino)

    return {"Respuesta": respuesta}
"""

