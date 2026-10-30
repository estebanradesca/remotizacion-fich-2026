import json
import re


ID_EQUIPO = 2

CAUDAL_MIN = 0
CAUDAL_MAX = 1300

ANGULO_MIN = 0.0
ANGULO_MAX = 11.5


# ============================================================
# ESTADO LOCAL
# ============================================================

# Últimas posiciones ordenadas a los motores.
POSICION_MOTOR1 = 0
POSICION_MOTOR2 = 0

# Último ángulo ordenado al servo.
ANGULO_SERVO = 0.0


# ============================================================
# COMANDOS PARA EL ARDUINO DE VÁLVULAS
# ============================================================

COMANDOS_VALVULAS = {
    # Bomba
    "B ON",
    "B OFF",
    "B TOGGLE",

    # Electroválvula
    "V ON",
    "V OFF",
    "V TOGGLE",

    # Comandos generales
    "P",
    "A",

    # RPM
    "RPM",
    "RPM STOP",
    "RPM RESET",
    "RPM ON",
    "RPM OFF",

    # Presiones
    "M",
    "P4",
    "P20",

    "P4 ON",
    "P4 OFF",
    "P4 MAX",
    "P4 MIN",
    "P4 RESET",

    "P20 ON",
    "P20 OFF",
    "P20 MAX",
    "P20 MIN",
    "P20 RESET",

    # Otros
    "MAX",
    "MIN",
    "RESET",
    "ESTADO",
    "DIAG",
}


# ============================================================
# PATRONES DE COMANDOS PARA MOTORES
# ============================================================

# Comandos directos:
# 1+500, 1-500, 2+500, 2-500.
PATRON_MOTOR_1_2 = re.compile(r"[12][+-]\d+")

# Motor 3:
# 3A
# 3C
# 3A 1500
# 3C 1500
PATRON_MOTOR_3 = re.compile(
    r"3[AC](?:\s+\d+)?",
    re.IGNORECASE
)

# Servo:
# S0
# S5.5
# S11.5
PATRON_SERVO = re.compile(
    r"S\d+(?:\.\d+)?",
    re.IGNORECASE
)

# Posiciones absolutas recibidas desde JavaScript.
# M1:500
# M2:800
PATRON_POSICION_MOTOR1 = re.compile(r"M1:(\d+)")
PATRON_POSICION_MOTOR2 = re.compile(r"M2:(\d+)")


# Comandos de estado y control de motores.
COMANDOS_MOTORES = {
    "INIT",

    "+",
    "-",
    "++",
    "--",

    "1S",
    "1P",
    "1H",
    "1D",

    "2S",
    "2P",
    "2H",
    "2D",

    "3S",
    "3P",
    "3H",
    "3D",

    "V3+",
    "V3-",

    "SS",
    "DIAG",
    "ESTADO",
}


# ============================================================
# ENVÍO TCP
# ============================================================

async def enviar_a_socket(socket_destino, comando):

    await socket_destino.enviar(
        (comando + "\n").encode("utf-8")
    )


# ============================================================
# ENVÍO DE COMANDOS A LOS ARDUINOS
# ============================================================

async def enviar_comando_al_arduino(
    comando: str,
    socket_valvulas,
    socket_motores,
):
    """
    Recibe un JSON por WebSocket:

        {
            "id_equipo": 2,
            "cmd": "B ON"
        }

    Distribuye los comandos entre el Arduino de válvulas
    y el Arduino de motores.

    Para los motores 1 y 2 admite posiciones absolutas:

        M1:500
        M2:800

    Estas posiciones se convierten en movimientos relativos:

        1+500
        2-200

    según la posición anterior de cada motor.
    """

    global POSICION_MOTOR1
    global POSICION_MOTOR2
    global ANGULO_SERVO

    # --------------------------------------------------------
    # VALIDACIÓN DEL JSON
    # --------------------------------------------------------

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
    nombre_destino = None
    comando_arduino = cmd

    nueva_posicion_motor1 = None
    nueva_posicion_motor2 = None
    nuevo_angulo_servo = None

    # --------------------------------------------------------
    # MOTOR 1: POSICIÓN ABSOLUTA
    # --------------------------------------------------------

    coincidencia = PATRON_POSICION_MOTOR1.fullmatch(cmd)

    if coincidencia:

        posicion = int(coincidencia.group(1))

        if not CAUDAL_MIN <= posicion <= CAUDAL_MAX:
            print(f"Posición de motor 1 fuera de rango: {posicion}")
            return

        diferencia = posicion - POSICION_MOTOR1

        if diferencia == 0:
            print(f"Motor 1 ya está en la posición {posicion}")
            return

        if diferencia > 0:
            comando_arduino = f"1+{diferencia}"
        else:
            comando_arduino = f"1{diferencia}"

        nueva_posicion_motor1 = posicion

        socket_destino = socket_motores
        nombre_destino = "motores"

    # --------------------------------------------------------
    # MOTOR 2: POSICIÓN ABSOLUTA
    # --------------------------------------------------------

    elif PATRON_POSICION_MOTOR2.fullmatch(cmd):

        coincidencia = PATRON_POSICION_MOTOR2.fullmatch(cmd)
        posicion = int(coincidencia.group(1))

        if not CAUDAL_MIN <= posicion <= CAUDAL_MAX:
            print(f"Posición de motor 2 fuera de rango: {posicion}")
            return

        diferencia = posicion - POSICION_MOTOR2

        if diferencia == 0:
            print(f"Motor 2 ya está en la posición {posicion}")
            return

        if diferencia > 0:
            comando_arduino = f"2+{diferencia}"
        else:
            comando_arduino = f"2{diferencia}"

        nueva_posicion_motor2 = posicion

        socket_destino = socket_motores
        nombre_destino = "motores"

    # --------------------------------------------------------
    # COMANDOS PARA EL SERVO
    # --------------------------------------------------------

    elif cmd.upper() == "INIT":

        comando_arduino = "INIT"

        socket_destino = socket_motores
        nombre_destino = "motores"

        nuevo_angulo_servo = 0.0

    elif PATRON_SERVO.fullmatch(cmd):

        angulo = float(cmd[1:])

        if not ANGULO_MIN <= angulo <= ANGULO_MAX:
            print(f"Ángulo del servo fuera de rango: {angulo}")
            return

        comando_arduino = f"S{angulo:.1f}"

        socket_destino = socket_motores
        nombre_destino = "motores"

        nuevo_angulo_servo = angulo

    # --------------------------------------------------------
    # COMANDOS DIRECTOS DE MOTORES
    # --------------------------------------------------------

    elif (
        PATRON_MOTOR_1_2.fullmatch(cmd)
        or PATRON_MOTOR_3.fullmatch(cmd)
        or cmd.upper() in COMANDOS_MOTORES
    ):

        socket_destino = socket_motores
        nombre_destino = "motores"

        comando_arduino = cmd

    # --------------------------------------------------------
    # COMANDOS DEL ARDUINO DE VÁLVULAS
    # --------------------------------------------------------

    elif cmd.upper() in COMANDOS_VALVULAS:

        socket_destino = socket_valvulas
        nombre_destino = "valvulas"

        comando_arduino = cmd.upper()

    # --------------------------------------------------------
    # COMANDO DESCONOCIDO
    # --------------------------------------------------------

    else:

        print(f"Comando no reconocido: {cmd!r}")
        return

    # --------------------------------------------------------
    # ENVÍO
    # --------------------------------------------------------

    conexion = getattr(socket_destino, "conexion", None)

    print(
        f"Enviando a [{nombre_destino}] "
        f"({conexion or 'sin conexión identificada'}): "
        f"{comando_arduino!r}"
    )

    try:

        await enviar_a_socket(
            socket_destino,
            comando_arduino
        )

        # Actualizamos las posiciones si el envío TCP
        # no produjo un error.

        if nueva_posicion_motor1 is not None:

            POSICION_MOTOR1 = nueva_posicion_motor1

            print(
                f"Posición objetivo motor 1: "
                f"{POSICION_MOTOR1}"
            )

        if nueva_posicion_motor2 is not None:

            POSICION_MOTOR2 = nueva_posicion_motor2

            print(
                f"Posición objetivo motor 2: "
                f"{POSICION_MOTOR2}"
            )

        if nuevo_angulo_servo is not None:

            ANGULO_SERVO = nuevo_angulo_servo

            print(
                f"Ángulo objetivo del servo: "
                f"{ANGULO_SERVO}"
            )

    except (ConnectionError, OSError) as error:

        print(
            f"Error enviando {comando_arduino!r} "
            f"a [{nombre_destino}]: {error}"
        )

        raise


# ============================================================
# INICIO DEL EQUIPO
# ============================================================

async def iniciar_equipo(socket_valvulas, socket_motores):

    global POSICION_MOTOR1
    global POSICION_MOTOR2
    global ANGULO_SERVO

    POSICION_MOTOR1 = 0
    POSICION_MOTOR2 = 0
    ANGULO_SERVO = 0.0

    await enviar_a_socket(socket_motores, "INIT")

    await enviar_a_socket(socket_valvulas, "ESTADO")


# ============================================================
# FINALIZACIÓN DEL EQUIPO
# ============================================================

async def fin_conexion_equipo(socket_valvulas, socket_motores):

    await enviar_a_socket(socket_motores, "SS")

    await enviar_a_socket(socket_valvulas, "A")

    await enviar_a_socket(socket_valvulas, "RPM STOP")


# ============================================================
# PROCESAMIENTO DE MENSAJES RECIBIDOS
# ============================================================

def procesar_mensaje_de_arduino(mensaje, conexion=None):
    if isinstance(mensaje, bytes):
        mensaje = mensaje.decode("utf-8", errors="ignore")

    mensaje = mensaje.strip()

    # Procesar presión
    if "P4:" in mensaje and "P20:" in mensaje:
        try:
            datos = {}

            for campo in mensaje.split("|"):
                if ":" in campo:
                    clave, valor = campo.split(":", 1)
                    datos[clave.strip()] = valor.strip()

            p4 = float(datos["P4"])
            p20 = float(datos["P20"])
            rpm = float(datos.get("RPM", 0))

            return {
                "id_equipo": ID_EQUIPO,
                "tipo": "presion",
                "tiempo_ms": 0,
                "presion_p4": p4 if p4 >= 0 else None,
                "presion_p20": p20 if p20 >= 0 else None,
                "rpm": rpm,
            }

        except (ValueError, KeyError) as e:
            print(f"Error procesando presión: {e}")
            return None

    # Otros mensajes
    return {
        "id_equipo": ID_EQUIPO,
        "tipo": "respuesta_arduino",
        "origen": conexion,
        "mensaje": mensaje,
    }