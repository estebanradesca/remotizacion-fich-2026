import socket
import json

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
        else:
            mensaje = "$RELE,ON"
        checksum = calcular_checksum(mensaje[1:])
        mensaje_procesado = mensaje + "*" + checksum + "\n"
        await socket_arduino.enviar(mensaje_procesado.encode("utf-8"))
    
    if comando_dict.get("motores") is not None:
        await habilitar_motores(socket_arduino)
    if comando_dict.get("datos") is not None:
        await obtener_datos(socket_arduino)
    
    


async def habilitar_motores(socket_arduino):
    mensajes = ["$M1,ENABLE,1", "$M2,ENABLE,1"]
    for mensaje in mensajes:
        checksum = calcular_checksum(mensaje[1:])
        mensaje_procesado = mensaje + "*" + checksum + "\n"
        await socket_arduino.enviar(mensaje_procesado.encode("utf-8"))
        
        
        

# Acá envío los motores a 0 pasos y luego los deshabilito
async def deshabilitar_motores(socket_arduino):
    mensajes = ["$M1,HOME", "$M2,HOME", "$M1,ENABLE,0", "$M2,ENABLE,0"]
    for mensaje in mensajes:
        checksum = calcular_checksum(mensaje[1:])
        mensaje_procesado = mensaje + "*" + checksum + "\n"
        await socket_arduino.enviar(mensaje_procesado.encode("utf-8"))
        print("mensaje ", mensaje_procesado)

async def obtener_datos(socket_arduino):
    mensaje = "$GET"
    checksum = calcular_checksum(mensaje[1:])
    mensaje_procesado = mensaje + "*" + checksum + "\n"
    await socket_arduino.enviar(mensaje_procesado.encode("utf-8"))



def procesar_mensaje_de_arduino(mensaje: str) -> dict:
    print(mensaje)
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
        mensaje_procesado["pasos_tinta"] = int(partes[4])
        mensaje_procesado["pasos_agua"] = int(partes[5])
        print(mensaje_procesado)
        return mensaje_procesado

    elif tipo == "ERR":
        comando, motivo = contenido.split(",", 1)
        mensaje_procesado["mensaje"] = f"**ERROR** Comando:{comando}. Motivo: {motivo}."
        return mensaje_procesado

    elif tipo == "ACK":
        comando, estado = contenido.split(",", 1)
        mensaje_procesado["mensaje"] = f"**RECIBIDO** Comando: {comando}. Estado: {estado}."
        return mensaje_procesado

    else:
        return 
        
def calcular_checksum(oracion):
    checksum = 0
    for letra in oracion:
        checksum ^= ord(letra)
    return f"{checksum:02X}"
