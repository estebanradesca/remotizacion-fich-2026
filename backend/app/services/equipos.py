import socket
import json

async def cambiar_estado_servicio(id_equipo, estado, socket_arduino):
    mensaje = "{" + f"\"id_equipo\":{id_equipo}, \"pasos_agua\":0, \"pasos_tinta\":0" + "}\n"
    
    await socket_arduino.enviar(mensaje.encode("utf-8"))
    return "Comando exitoso"


async def enviar_comando_al_arduino(comando: str, socket_arduino):
    comando_dict = json.loads(comando)
    if comando_dict.get("pasos_agua") is not None:
        pasos = comando_dict["pasos_agua"]
        mensaje = f"$M2,POS,{pasos}"
        checksum = calcular_checksum(mensaje[1:])
        mensaje_procesado = mensaje + "*" + checksum + "\n"
        await socket_arduino.enviar(mensaje_procesado.encode("utf-8"))
    if comando_dict.get("pasos_tinta") is not None:
        pasos = comando_dict["pasos_tinta"]
        mensaje = f"$M1,POS,{pasos}"
        checksum = calcular_checksum(mensaje[1:])
        mensaje_procesado = mensaje + "*" + checksum + "\n"
        await socket_arduino.enviar(mensaje_procesado.encode("utf-8"))
    if comando_dict.get("rele") is not None:

        estado = comando_dict["rele"]

        if estado == 0:
            mensaje = "$RELE,OFF"
            print("llegue hasta aca")
        else:
            mensaje = "$RELE,ON"
        checksum = calcular_checksum(mensaje[1:])
        mensaje_procesado = mensaje + "*" + checksum + "\n"
        await socket_arduino.enviar(mensaje_procesado.encode("utf-8"))



def procesar_mensaje_de_arduino(mensaje: str) -> dict:
    mensaje_procesado = {}
    

    oracion, checksum = mensaje.split("*", 1)

    
    

    checksum_calculado = calcular_checksum(oracion[1:])


    if checksum != checksum_calculado:
        mensaje_procesado["mensaje"] = "El mensaje no se envió correctamente desde el Arduino, error de checksum" 
        return mensaje_procesado

    tipo, contenido = oracion[1:].split(",", 1)
    if tipo == "SD":
        partes = contenido.split(",", 5)
        mensaje_procesado["id_equipo"] = 1
        mensaje_procesado["caudal_agua"] = float(partes[0])
        mensaje_procesado["temp"] = float(partes[1])
        mensaje_procesado["nivel"] = float(partes[2])
        mensaje_procesado["rele"] = 1 if (str(partes[3]) == "ON") else 0
        mensaje_procesado["pasos_agua"] = int(partes[4])
        mensaje_procesado["pasos_tinta"] = int(partes[5])
        return mensaje_procesado

    elif tipo == "ERR":
        comando, motivo = contenido.split(",", 1)
        mensaje_procesado["mensaje"] = f"**ERROR** Código:{comando}. Motivo: {motivo}."
        print(mensaje_procesado)
        return mensaje_procesado

    elif tipo == "ACK":
        comando, estado = contenido.split(",", 1)
        mensaje_procesado["mensaje"] = f"El {comando} se envió correctamente. Estado: {estado}"
        print(mensaje_procesado)
        return mensaje_procesado
        
    else:
        mensaje_procesado["mensaje"] = f"{mensaje_procesado}"
        return mensaje_procesado


def calcular_checksum(oracion):
    checksum = 0
    for letra in oracion:
        checksum ^= ord(letra)
    return f"{checksum:02X}"
