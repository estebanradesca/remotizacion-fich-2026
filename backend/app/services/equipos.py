import socket

async def cambiar_estado_servicio(id_equipo, estado, socket_arduino):
    mensaje = f"ID:{id_equipo}|E:{estado.value}|PA:0|PT:0\n"
    
    await socket_arduino.enviar(mensaje.encode("utf-8"))
    return "Comando exitoso"


async def enviar_comando_arduino(comando: dict, socket_arduino):
    mensaje = comando
    await socket_arduino.enviar(mensaje.encode("utf-8"))
    ### Falta procesarlo


def procesar_lectura_arduino(mensaje: str) -> dict:
    mensaje_procesado = {}
    partes = mensaje.strip().split('|')
    
    
    for parte in partes:
        clave, valor = parte.split(':')
        mensaje_procesado[clave] = valor

    
    return mensaje_procesado



"""
async def modificar_pasos_servicio(id_equipo, pasos, tipo, socket_arduino):
    if tipo == "agua":
        mensaje = f"ID: {id_equipo}|E: {estado}|PA: {pasos}|PT: 0\n"
    elif tipo == "tinta":
        mensaje = f"ID: {id_equipo}|E: {estado}|PA: 0|PT: {pasos}\n"
    else:
        return "Comando incorrecto"    
    await socket_arduino.enviar(mensaje.encode("utf-8"))
    return "Comando exitoso"


def procesar_lectura(linea: str):
    #        ESTADO|    CAUDAL|TEMP     |    CANT.PASOS M1| CANT. PASOS M2|ESTADO DEL RELE   |
    #        "E:ERR|     C:5.42|T:22.5   |            M1:10|         M2:300|R:ON             | 
      
    # ejemplo de envío de arduino = "E:ON|C:5.42|T:22.5|M1:10|M2:300|R:ON\n"
    datos = {}
    partes = linea.strip().split('|')
    
    for  in partes:
        clave, valor = parte.split(':')
        datos[clave] = float(valor)
    
    return datos 
    # Resultado: {"C": 5.42, "T": 22.5, "N": 45.0, ...}
"""