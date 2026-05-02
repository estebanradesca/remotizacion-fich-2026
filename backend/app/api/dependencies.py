from starlette.requests import HTTPConnection
from app.infra.tcp_arduino import SocketArduino

def obtener_socket(conexion: HTTPConnection):
    return conexion.app.state.arduino_socket

def obtener_controlador(conexion: HTTPConnection):
    return conexion.app.state.controlador
