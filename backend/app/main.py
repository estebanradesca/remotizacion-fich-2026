from fastapi import FastAPI, WebSocket  
from fastapi.middleware.cors import CORSMiddleware 
from contextlib import asynccontextmanager
import asyncio


from app.infra.tcp_arduino import SocketArduino
from app.api.ws.ws_control import recibo_mensaje_de_arduino, controlador_ws
from app.api.routers import websocket


# Inicializo recursos que voy a necesitar durante todo el tiempo
# que dure el servidor. Cuando el servidor finaliza se cierran
@asynccontextmanager
async def lifespan(app: FastAPI):
    # Este código se ejecuta antes de iniciar el servidor

    try:
        await iniciar_socket(app, id_equipo = 0)
        
        # Controlador del websocket para usarlo como dependencia
        app.state.controlador = controlador_ws

    except Exception as error:
        print("No se pudo iniciar")
        app.state.controlador = None

    
    yield
    # Esta parte del código se ejecuta justo antes de cerrar el servidor    
    
    if app.state.controlador is not None:
        # Cierro la conexión con el módulo NT1-B del Arduino 
        await cerrar_socket(app)
    


app = FastAPI(lifespan = lifespan)

async def iniciar_socket(app, id_equipo: int):
    # Inicio la conexión con el módulo NT1-B del Arduino    
    app.state.arduino_socket = SocketArduino(id_equipo)
    
    
    await asyncio.to_thread(app.state.arduino_socket.conectar)
    
    # Le paso a mi socket la función que utilizo para reenviar el mensaje
    # que me llega desde el Arduino al websocket.
    app.state.arduino_socket.funcion_callback(recibo_mensaje_de_arduino)
    
    # El socket se queda escuchando en segundo plano sin interrumpir
    
    app.state.escucha_task = asyncio.create_task(app.state.arduino_socket.recibir())
    

async def cerrar_socket(app):
    await asyncio.to_thread(app.state.arduino_socket.cerrar)
    app.state.escucha_task.cancel()




### Cambiar después para solo permitir acceso seguro
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=False,
    allow_methods=["*"],
    allow_headers=["*"],
)

#app.include_router(equipos.router)
app.include_router(websocket.router)

@app.get("/")
async def root():
    return {"mensaje": "Hola"}


