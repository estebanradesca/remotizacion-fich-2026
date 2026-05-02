from pydantic import BaseModel, Field
from typing import Annotated
from enum import Enum

class Estado(str, Enum):
    APAGADO = "apagado"
    FUNCIONANDO = "funcionando"
    ERROR = "error"

class TipoComando(str, Enum):
    TINTA = "tinta"
    CAUDAL = "caudal"

# Se envían desde la web

class ModificarPasosAgua(BaseModel):
    pasos: Annotated[int, Field(ge=0, le=600)]

class ModificarPasosTinta(BaseModel):
    pasos: Annotated[int, Field(ge=0, le=50)]


class CambiarEstado(BaseModel):
    estado: Estado 

class ComandoControl(BaseModel):
    tipo: TipoComando
    valor: float = Field(ge=0, le=10)

# Se envían desde la API 

class EstadoEquipo(BaseModel):
    id_equipo: int = Field(ge=1, le=5)
    estado: Estado 

class LecturaSensores(BaseModel):
    id_equipo: int = Field(ge=0, le=5)
    temperatura: float 
    vel_caudal: float 

class Experimento(BaseModel):
    equipo: EstadoEquipo
    sensores: LecturaSensores
    

    




