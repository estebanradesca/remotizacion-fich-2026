
from . import reynolds
from . import tuberias
from . import golpe
from . import turbina


EQUIPOS = {
    0: reynolds,
    1: tuberias,
    2: golpe,
    3: turbina,
}


async def inicio_conexion_equipo(
    id_equipo: int,
    socket_arduino,
):
    equipo = EQUIPOS[id_equipo]

    if id_equipo == 2:
        socket_valvulas, socket_motores = socket_arduino

        await equipo.iniciar_equipo(
            socket_valvulas,
            socket_motores,
        )
    else:
        await equipo.iniciar_equipo(socket_arduino)


async def enviar_comando_al_arduino(
    id_equipo: int,
    comando,
    socket_arduino,
):
    equipo = EQUIPOS[id_equipo]

    if id_equipo == 2:
        socket_valvulas, socket_motores = socket_arduino

        await equipo.enviar_comando_al_arduino(
            comando,
            socket_valvulas,
            socket_motores,
        )
    else:
        await equipo.enviar_comando_al_arduino(
            comando,
            socket_arduino,
        )


async def fin_conexion_equipo(
    id_equipo: int,
    socket_arduino,
):
    equipo = EQUIPOS[id_equipo]

    if id_equipo == 2:
        socket_valvulas, socket_motores = socket_arduino

        await equipo.fin_conexion_equipo(
            socket_valvulas,
            socket_motores,
        )
    else:
        await equipo.fin_conexion_equipo(socket_arduino)


def procesar_mensaje_de_arduino(
    mensaje: str,
    id_equipo: int,
    conexion: str | None = None,
):
    equipo = EQUIPOS.get(id_equipo)

    if equipo is None:
        return None

    if id_equipo == 2:
        return equipo.procesar_mensaje_de_arduino(
            mensaje,
            conexion=conexion,
        )

    return equipo.procesar_mensaje_de_arduino(mensaje)
