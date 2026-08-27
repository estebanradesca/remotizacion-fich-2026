import json


# Envío de comandos desde el servidor al arduino
async def enviar_comando_al_arduino(comando: str, socket_arduino):

    # Convierto a diccionario el comando que llega desde el websocket
    comando_dict = json.loads(comando) 

    # Pasos de tinta para el primer motor
    if comando_dict.get("pasos_tinta") is not None:
        pasos = comando_dict["pasos_tinta"]
        oracion = f"$M1,POS,{pasos}"
        checksum = calcular_checksum(oracion[1:])
        oracion += "*" + checksum + "\n"
        await socket_arduino.enviar(oracion.encode("utf-8"))

    # Pasos de agua para el segundo motor
    if comando_dict.get("pasos_agua") is not None:
        pasos = comando_dict["pasos_agua"]
        oracion = f"$M2,POS,{pasos}"
        checksum = calcular_checksum(oracion[1:])
        oracion += "*" + checksum + "\n"
        await socket_arduino.enviar(oracion.encode("utf-8"))
    
    # Activación o desactivación del relé
    if comando_dict.get("rele") is not None:
        estado = comando_dict["rele"]
        if estado == 0:
            oracion = "$RELE,OFF"
        else:   # estado == 1:
            oracion = "$RELE,ON"
        checksum = calcular_checksum(oracion[1:])
        oracion += "*" + checksum + "\n"
        await socket_arduino.enviar(oracion.encode("utf-8"))

    if comando_dict.get("datos") is not None:
        await obtener_datos(socket_arduino)
    
    

# Habilitación de los motores cuando se conecta un cliente
async def iniciar_equipo(socket_arduino):
    oraciones = ["$M1,ENABLE,1", "$M2,ENABLE,1"]
    for oracion in oraciones:
        checksum = calcular_checksum(oracion[1:])
        oracion += "*" + checksum + "\n"
        await socket_arduino.enviar(oracion.encode("utf-8"))
        
        
        
# Deshabilitación de los motores cuando se desconecta el cliente
# Acá envío los motores a 0 pasos y luego los deshabilito
async def fin_conexion_equipo(socket_arduino):
    oraciones = ["$M1,HOME", "$M2,HOME", "$M1,ENABLE,0", "$M2,ENABLE,0"]
    for oracion in oraciones:
        checksum = calcular_checksum(oracion[1:])
        oracion += "*" + checksum + "\n"
        await socket_arduino.enviar(oracion.encode("utf-8"))



async def obtener_datos(socket_arduino):
    oracion = "$GET"
    checksum = calcular_checksum(oracion[1:])
    oracion += "*" + checksum + "\n"
    await socket_arduino.enviar(oracion.encode("utf-8"))


# Proceso comandos que llegan desde el arduino
def procesar_mensaje_de_arduino(mensaje):
    print(mensaje)
    
    
    oracion = {}

    contenido, checksum = mensaje.split("*", 1)    
    checksum_calculado = calcular_checksum(contenido[1:])

    if checksum != checksum_calculado:
        oracion["mensaje"] = "El mensaje no se envió correctamente desde el Arduino, error de checksum" 
        return oracion

    tipo, contenido = contenido[1:].split(",", 1)
    
    # Proceso los mensajes de datos
    if tipo == "SD":
        partes = contenido.split(",", 5)   
        oracion["id_equipo"] = 0 # El equipo de Reynolds es el 0
        oracion["caudal_agua"] = float(partes[0])
        oracion["temp"] = float(partes[1])
        oracion["nivel"] = float(partes[2])
        oracion["rele"] = int(partes[3])
        oracion["pasos_tinta"] = int(partes[4])
        oracion["pasos_agua"] = int(partes[5])
        return oracion

    

    elif tipo == "ERR":
        comando, motivo = contenido.split(",", 1)
        oracion["mensaje"] = f"**ERROR** Comando:{comando}. Motivo: {motivo}."
        return oracion

    elif tipo == "ACK":
        comando, estado = contenido.split(",", 1)
        oracion["mensaje"] = f"**RECIBIDO** Comando: {comando}. Estado: {estado}."
        return oracion

    else:
        return 
        
def calcular_checksum(oracion):
    checksum = 0
    for letra in oracion:
        checksum ^= ord(letra)
    return f"{checksum:02X}"
