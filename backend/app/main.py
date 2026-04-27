from fastapi import FastAPI, WebSocket  
from fastapi.middleware.cors import CORSMiddleware 
import asyncio
from contextlib import asynccontextmanager

from app.api.ws.ws_control import recibo_mensaje
from app.api.routers import equipos, websocket
from app.infra.tcp_arduino import SocketArduino

# Este código se ejecuta antes de iniciar la app
# Inicio el socket TCP con el módulo NT1-B del Arduino
# y también la conexión con la base de datos

### Falta la conexión a la base de datos
@asynccontextmanager
async def lifespan(app: FastAPI):
    # Se ejecuta antes de iniciar la app
    app.state.arduino_socket = SocketArduino()

    await asyncio.to_thread(app.state.arduino_socket.conectar)

    app.state.arduino_socket.funcion_callback(recibo_mensaje)

    app.state.arduino_task = asyncio.create_task(
        app.state.arduino_socket.escuchar()
    )
    yield
    # Se ejecuta antes de cerrar la app
    app.state.arduino_task.cancel()
    await asyncio.to_thread(app.state.arduino_socket.cerrar)


app = FastAPI(lifespan = lifespan)

app.include_router(equipos.router)
app.include_router(websocket.router)

@app.get("/")
async def root():
    return {"mensaje": "Hola"}


