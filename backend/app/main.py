from fastapi import FastAPI, WebSocket  
from fastapi.middleware.cors import CORSMiddleware 
from contextlib import asynccontextmanager
import asyncio

from app.infra.tcp_arduino import SocketArduino
from app.api.ws.ws_control import recibo_mensaje_de_arduino, controlador_ws
from app.api.routers import equipos, websocket


# Inicializo recursos que voy a necesitar durante todo el tiempo
# que dure el servidor. Cuando el servidor finaliza se cierran
@asynccontextmanager
async def lifespan(app: FastAPI):
    # Este código se ejecuta antes de iniciar el servidor

    # Inicio la conexión con el módulo NT1-B del Arduino    
    app.state.arduino_socket = SocketArduino()
    await asyncio.to_thread(app.state.arduino_socket.conectar)

    # Inicio la conexión a la base de datos
    ### Falta la conexión a la base de datos.

    # Controlador del websocket para usarlo como dependencia
    app.state.controlador = controlador_ws


    # Le paso a mi socket la función que utilizo para reenviar el mensaje
    # que me llega desde el Arduino al websocket.
    app.state.arduino_socket.funcion_callback(recibo_mensaje_de_arduino)
    
    # El socket se queda escuchando en segundo plano sin interrumpir
    escucha_task = asyncio.create_task(app.state.arduino_socket.recibir())
    
    yield
    # Esta parte del código se ejecuta justo antes de cerrar el servidor

    escucha_task.cancel()

    # Cierro la conexión con el módulo NT1-B del Arduino 
    await asyncio.to_thread(app.state.arduino_socket.cerrar)
    


app = FastAPI(lifespan = lifespan)

### Cambiar después para solo permitir acceso seguro
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=False,
    allow_methods=["*"],
    allow_headers=["*"],
)

app.include_router(equipos.router)
app.include_router(websocket.router)

@app.get("/")
async def root():
    return {"mensaje": "Hola"}


