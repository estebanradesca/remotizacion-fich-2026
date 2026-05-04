from pydantic import BaseModel, Field
from typing import Annotated
from enum import Enum

class Estado(str, Enum):
    APAGADO = "apagado"
    ENCENDIDO = "encendido"
    ERROR = "error"

Id = Annotated[int, Field(ge=1, le=5)]

# Se envían desde la web
"""
class ModificarPasosAgua(BaseModel):
    id_equipo: Id
    pasos: Annotated[int, Field(ge=0, le=600)]

class ModificarPasosTinta(BaseModel):
    id_equipo: Id
    pasos: Annotated[int, Field(ge=0, le=50)]
"""

class CambiarEstado(BaseModel):
    id_equipo: Id
    estado: Estado 

# Se envían desde la API

class LecturaSensores(BaseModel):
    temperatura: float 
    vel_caudal: float 

class EstadoExperimento(BaseModel):
    id_equipo: Id
    equipo: Estado
    sensores: LecturaSensores
    mensaje: None | str = None
    

    




