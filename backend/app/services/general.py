from . import reynolds
from . import tuberias

EQUIPOS = {
    0: reynolds,
    1: tuberias
}

async def inicio_conexion_equipo(id_equipo, socket_arduino):
    equipo = EQUIPOS[id_equipo]
    await equipo.iniciar_equipo(socket_arduino)


async def enviar_comando_al_arduino(id_equipo, comando, socket_arduino):
    equipo = EQUIPOS[id_equipo]
    await equipo.enviar_comando_al_arduino(comando, socket_arduino)


async def fin_conexion_equipo(id_equipo, socket_arduino):
    equipo = EQUIPOS[id_equipo]
    await equipo.fin_conexion_equipo(socket_arduino)

def procesar_mensaje_de_arduino(mensaje, id_equipo):
    equipo = EQUIPOS[id_equipo]
    equipo.procesar_mensaje_de_arduino(mensaje)