
import json
import re

ID_EQUIPO = 2
CAUDAL_MIN = 0
CAUDAL_MAX = 1300

# Comandos destinados al Arduino de válvulas.
COMANDOS_VALVULAS = {
    "P", "A",
    "RPM", "RPM STOP", "RPM RESET", "RPM ON", "RPM OFF",
    "M", "P4", "P20",
    "P4 ON", "P4 OFF", "P4 MAX", "P4 MIN", "P4 RESET",
    "P20 ON", "P20 OFF", "P20 MAX", "P20 MIN", "P20 RESET",
    "MAX", "MIN", "RESET", "ESTADO", "DIAG",
}

# Comandos destinados al Arduino de motores.
COMANDOS_MOTORES = {
    "B ON", "B OFF", "B TOGGLE",
    "V ON", "V OFF", "V TOGGLE",
}

# Comando de cierre del motor paso a paso 3:
# 3C <velocidad>
PATRON_CIERRE_LENTO = re.compile(r"3C\s+(\d+)")


async def enviar_comando_al_arduino(
    comando: str,
    socket_valvulas,
    socket_motores,
):
    """Valida el mensaje WebSocket y lo envía al Arduino correspondiente."""

    try:
        datos = json.loads(comando)
    except (json.JSONDecodeError, TypeError):
        print("Mensaje WebSocket no es un JSON válido")
        return

    if not isinstance(datos, dict):
        print("El mensaje JSON debe ser un objeto")
        return

    if datos.get("id_equipo") != ID_EQUIPO:
        print(f"ID de equipo incorrecto: {datos.get('id_equipo')}")
        return

    cmd = datos.get("cmd")

    if not isinstance(cmd, str):
        print("El comando debe ser una cadena")
        return

    cmd = cmd.strip()

    if not cmd:
        print("Comando vacío")
        return

    if "\n" in cmd or "\r" in cmd:
        print("Comando rechazado: contiene saltos de línea")
        return

    socket_destino = None

    # CAUDAL: número entero de 0 a 1300.
    # Lo regula el motor 1, por eso va al Arduino de motores.
    if re.fullmatch(r"\d+", cmd):
        caudal = int(cmd)

        if not CAUDAL_MIN <= caudal <= CAUDAL_MAX:
            print(f"Caudal fuera de rango: {caudal}")
            return

        socket_destino = socket_motores

    # CIERRE LENTO: motor paso a paso 3.
    # Formato enviado al Arduino: 3C <velocidad>
    else:
        coincidencia = PATRON_CIERRE_LENTO.fullmatch(cmd)

        if coincidencia:
            velocidad = int(coincidencia.group(1))

            if velocidad <= 0:
                print("La velocidad de cierre debe ser mayor que cero")
                return

            socket_destino = socket_motores

        elif cmd in COMANDOS_MOTORES:
            socket_destino = socket_motores

        elif cmd in COMANDOS_VALVULAS:
            socket_destino = socket_valvulas

        else:
            print(f"Comando de Golpe de Ariete no reconocido: {cmd!r}")
            return

    conexion = getattr(socket_destino, "conexion", None)
    nombre_socket = conexion or "Arduino"

    print(f"Enviando a [{nombre_socket}]: {cmd!r}")

    try:
        await socket_destino.enviar(
            (cmd + "\n").encode("utf-8")
        )
    except (ConnectionError, OSError) as error:
        print(f"Error enviando {cmd!r} a [{nombre_socket}]: {error}")
        raise


async def iniciar_equipo(socket_valvulas, socket_motores):
    """Consulta el estado inicial del Arduino de válvulas."""
    await socket_valvulas.enviar(b"ESTADO\n")
    


async def fin_conexion_equipo(socket_valvulas, socket_motores):
    """Envía las órdenes de finalización al Arduino de válvulas."""

    await socket_valvulas.enviar(b"A\n")
    await socket_valvulas.enviar(b"RPM STOP\n")


def procesar_mensaje_de_arduino(
    mensaje: str | bytes,
    conexion: str | None = None,
):
    """Interpreta los mensajes recibidos por TCP para el WebSocket."""

    if isinstance(mensaje, bytes):
        mensaje = mensaje.decode("utf-8", errors="replace")

    mensaje = mensaje.strip()

    if not mensaje:
        return None

    origen = conexion or "valvulas"

    # Los mensajes del Arduino de motores se reenvían como texto.
    if origen != "valvulas":
        return {
            "id_equipo": ID_EQUIPO,
            "tipo": "mensaje",
            "origen": origen,
            "mensaje": mensaje,
        }

    # Formato: PRESION,tiempo_ms,p4,p20
    if mensaje.startswith("PRESION,"):
        try:
            partes = mensaje.split(",")

            if len(partes) != 4:
                print(f"Formato PRESION incorrecto: {mensaje!r}")
                return None

            tiempo_ms = int(partes[1])
            p4 = float(partes[2])
            p20 = float(partes[3])

            return {
                "id_equipo": ID_EQUIPO,
                "tipo": "presion",
                "tiempo_ms": tiempo_ms,
                "presion_p4": p4 if p4 >= 0 else None,
                "presion_p20": p20 if p20 >= 0 else None,
            }

        except ValueError:
            print(f"No se pudo interpretar la presión: {mensaje!r}")
            return None

    # Formato de monitoreo: P4:12.35|P20:23.50|RPM:120|...
    if "P4:" in mensaje and "P20:" in mensaje:
        try:
            campos = {}

            for parte in mensaje.split("|"):
                if ":" not in parte:
                    continue

                clave, valor = parte.split(":", 1)
                campos[clave.strip().upper()] = valor.strip()

            p4 = float(campos["P4"])
            p20 = float(campos["P20"])

            return {
                "id_equipo": ID_EQUIPO,
                "tipo": "presion",
                "tiempo_ms": 0,
                "presion_p4": p4 if p4 >= 0 else None,
                "presion_p20": p20 if p20 >= 0 else None,
            }

        except (ValueError, KeyError):
            print(f"Formato de monitoreo incorrecto: {mensaje!r}")
            return None

    # Mensajes generales, diagnósticos y confirmaciones.
    return {
        "id_equipo": ID_EQUIPO,
        "tipo": "mensaje",
        "origen": origen,
        "mensaje": mensaje,
    }
