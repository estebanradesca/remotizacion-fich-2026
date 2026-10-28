
from contextlib import asynccontextmanager
import asyncio

from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware

from app.infra.tcp_arduino import SocketArduino
from app.api.ws.ws_control import (
    recibo_mensaje_de_arduino,
    controlador_ws,
)
from app.api.routers import websocket


@asynccontextmanager
async def lifespan(app: FastAPI):
    app.state.sockets = {}
    app.state.tareas_sockets = {}
    app.state.controlador = controlador_ws

    # Cada entrada representa una conexión TCP independiente.
    conexiones = [
        (2, "valvulas"),    # Golpe de ariete: válvulas y sensores
        (2, "motores"),     # Golpe de ariete: motores
    ]

    try:
        for id_equipo, conexion in conexiones:
            await iniciar_socket(app, id_equipo, conexion)

        yield

    finally:
        await cerrar_sockets(app)


app = FastAPI(lifespan=lifespan)


async def iniciar_socket(
    app: FastAPI,
    id_equipo: int,
    conexion: str | None = None,
):
    socket_arduino = SocketArduino(id_equipo, conexion)

    # La clave distingue los dos sockets del equipo 2.
    clave = (id_equipo, conexion)

    await asyncio.to_thread(socket_arduino.conectar)

    # El callback debe conocer qué conexión recibió el mensaje.
    async def callback(mensaje, id_recibido):
        await recibo_mensaje_de_arduino(
            mensaje,
            id_recibido,
            conexion=conexion,
        )

    socket_arduino.funcion_callback(callback)

    app.state.sockets[clave] = socket_arduino
    app.state.tareas_sockets[clave] = asyncio.create_task(
        socket_arduino.recibir()
    )


async def cerrar_sockets(app: FastAPI):
    sockets = getattr(app.state, "sockets", {})
    tareas = getattr(app.state, "tareas_sockets", {})

    # Primero se cierran los sockets para interrumpir recv().
    for socket_arduino in sockets.values():
        await asyncio.to_thread(socket_arduino.cerrar)

    # Después se espera la finalización de las tareas.
    for tarea in tareas.values():
        if not tarea.done():
            tarea.cancel()

    if tareas:
        await asyncio.gather(*tareas.values(), return_exceptions=True)


app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=False,
    allow_methods=["*"],
    allow_headers=["*"],
)

app.include_router(websocket.router)


@app.get("/")
async def root():
    return {"mensaje": "Hola"}
