
import json


ID_EQUIPO = 3  # Ajustar al ID usado en tu backend


# Envío de comandos desde el servidor al Arduino
async def enviar_comando_al_arduino(comando: str, socket_arduino):
    comando_dict = json.loads(comando)

    # Comando de texto definido por el Arduino:
    # P, A, V ON, V OFF, RPM, RPM STOP, etc.
    cmd = comando_dict.get("cmd")

    if cmd is not None:
        cmd = str(cmd).strip()

        if not cmd or "\n" in cmd or "\r" in cmd:
            return

        await socket_arduino.enviar((cmd + "\n").encode("utf-8"))


# Acciones al conectar un cliente.
# Consultamos el estado, sin encender la bomba automáticamente.
async def iniciar_equipo(socket_arduino):
    await socket_arduino.enviar(b"ESTADO\n")


# Acciones al desconectar un cliente.
# Detenemos la bomba y la medición de RPM.
async def fin_conexion_equipo(socket_arduino):
    await socket_arduino.enviar(b"A\n")
    await socket_arduino.enviar(b"RPM STOP\n")


# Procesamiento de mensajes recibidos desde el Arduino
def procesar_mensaje_de_arduino(mensaje):
    mensaje = mensaje.strip()

    if not mensaje:
        return None

    # Mediciones periódicas:
    # PRESION,tiempo_ms,presion_P4,presion_P20
    if mensaje.startswith("PRESION,"):
        try:
            partes = mensaje.split(",")

            if len(partes) != 4:
                return {
                    "id_equipo": ID_EQUIPO,
                    "mensaje": f"Formato de presión inválido: {mensaje}",
                }

            tiempo_ms = int(partes[1])
            presion_p4 = float(partes[2])
            presion_p20 = float(partes[3])

            return {
                "id_equipo": ID_EQUIPO,
                "tipo": "presion",
                "tiempo_ms": tiempo_ms,
                "presion_p4": (
                    presion_p4 if presion_p4 >= 0 else None
                ),
                "presion_p20": (
                    presion_p20 if presion_p20 >= 0 else None
                ),
            }

        except (ValueError, IndexError):
            return {
                "id_equipo": ID_EQUIPO,
                "mensaje": f"No se pudieron interpretar las presiones: {mensaje}",
            }

    # Respuestas que envía el Arduino con el formato:
    # CMD:comando|RES:respuesta
    if mensaje.startswith("CMD:") and "|RES:" in mensaje:
        comando, respuesta = mensaje.split("|RES:", 1)

        return {
            "id_equipo": ID_EQUIPO,
            "tipo": "respuesta",
            "comando": comando.removeprefix("CMD:"),
            "respuesta": respuesta,
        }

    # Mensajes de inicio, información, errores o estado
    return {
        "id_equipo": ID_EQUIPO,
        "tipo": "mensaje",
        "mensaje": mensaje,
    }
