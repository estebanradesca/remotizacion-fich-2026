
from starlette.requests import HTTPConnection


def obtener_sockets(conexion: HTTPConnection):
    return conexion.app.state.sockets


def obtener_controlador(conexion: HTTPConnection):
    return conexion.app.state.controlador
