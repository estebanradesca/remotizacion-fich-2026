from fastapi import APIRouter, Depends


from app.api.dependencies import obtener_arduino
from app.infra.tcp_arduino import SocketArduino
from app.api.schemas.equipos_schema import CambiarEstado, ComandoControl
from app.services.equipos_servicio import enviar_mensaje

router = APIRouter()

@router.post("/equipo/estado")
async def cambiar_estado(
    cmd: CambiarEstado, 
    arduino: SocketArduino = Depends(obtener_arduino)
    ):

    mensaje = await enviar_mensaje(cmd, arduino)

    return {"mensaje": mensaje}

"""
@router.post("/equipo/comando")
async def enviar_comando(
    cmd: ComandoControl, 
    arduino: SocketArduino = Depends(obtener_arduino)
    ):
    
    mensaje = await enviar_mensaje(arduino, cmd)
    
    return {"mensaje": mensaje}
"""

