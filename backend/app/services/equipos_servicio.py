import socket
from dotenv import load_dotenv

async def enviar_mensaje(cmd, arduino):
    msg = f"Id: {cmd.id_equipo}, Estado: {cmd.estado}\n"
    
    await arduino.enviar(msg.encode("utf-8"))
    
    # FORZAR LECTURA AQUÍ PARA TESTEAR
    # Si esto imprime algo, el Arduino está respondiendo.
    # respuesta = await arduino.recibir(1024) 
    # print(f"DEBUG: El Arduino respondió directamente al POST: {respuesta}")

    return "Mensaje enviado"

async def enviar_mensaje_ws(msg, arduino):
    mensaje = (msg + "\n").encode("utf-8")
    await arduino.enviar(mensaje)

    return "El mensaje se envió correctamente"


