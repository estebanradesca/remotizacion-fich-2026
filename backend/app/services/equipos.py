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
    mensaje = f"Id: {msg.id_equipo}, Estado: {msg.estado}"

    data = (mensaje + "\n").encode("utf-8")
    await arduino.enviar(mensaje)

    return "El mensaje se envió correctamente"


def procesar_lectura_arduino(linea: str):
    #        ESTADO|    CAUDAL|TEMP     |    CANT.PASOS M1| CANT. PASOS M2|ESTADO DEL RELE   |
    #        "E:ERR|     C:5.42|T:22.5   |            M1:10|         M2:300|R:ON             | 
      
    # ejemplo de envío de arduino = "E:ON|C:5.42|T:22.5|M1:10|M2:300|R:ON\n"
    datos = {}
    partes = linea.strip().split('|')
    
    for parte in partes:
        clave, valor = parte.split(':')
        datos[clave] = float(valor)
    
    return datos 
    # Resultado: {"C": 5.42, "T": 22.5, "N": 45.0, ...}