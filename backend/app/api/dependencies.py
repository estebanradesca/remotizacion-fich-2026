from starlette.requests import HTTPConnection
from app.infra.tcp_arduino import SocketArduino

def obtener_arduino(conexion: HTTPConnection):
    return conexion.app.state.arduino_socket
