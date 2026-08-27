import json
import asyncio


# ============================================================
# CONFIGURACIÓN DE PURGA
# ============================================================

TIEMPO_ENTRE_TOMAS = 5

VALVULA_PURGA_1 = 17
VALVULA_PURGA_2 = 18

# Cada toma está formada por dos válvulas
TOMAS_PURGA = [
    (1, 2),
    (3, 4),
    (5, 6),
    (7, 8),
    (9, 10),
    (11, 12),
    (13, 14),
    (15, 16),
]


# ============================================================
# COMANDOS DESDE EL FRONTEND
# ============================================================

async def enviar_comando_al_arduino(
    comando: str,
    socket_arduino
):
    comando_dict = json.loads(comando)

    oracion = comando_dict.get("cmd") + "\n"

    await socket_arduino.enviar(
        oracion.encode("utf-8")
    )


# ============================================================
# ENVIAR COMANDO DIRECTO AL ARDUINO
# ============================================================

async def enviar_comando(
    socket_arduino,
    comando: str
):
    oracion = comando + "\n"

    await socket_arduino.enviar(
        oracion.encode("utf-8")
    )

    print(f"Arduino <- {comando}")


# ============================================================
# INICIO DEL EQUIPO
# ============================================================

async def iniciar_equipo(
    socket_arduino
):
    print("Iniciando equipo de tuberías...")

    # Inicializar Arduino
    await enviar_comando(
        socket_arduino,
        "S"
    )

    # Realizar purga
    """
    await purgar_caneria(
        socket_arduino
    )
    """
    print("Equipo de tuberías listo.")


# ============================================================
# PURGA
# ============================================================

async def purgar_caneria(
    socket_arduino
):

    print("================================")
    print("INICIANDO PURGA")
    print("================================")

    # --------------------------------------------------------
    # Abrir válvulas principales de purga
    # --------------------------------------------------------

    print("Abriendo válvulas de purga 17 y 18")

    await enviar_comando(
        socket_arduino,
        "17A"
    )

    await enviar_comando(
        socket_arduino,
        "18A"
    )


    # --------------------------------------------------------
    # Recorrer todas las tomas
    # --------------------------------------------------------

    for numero, (valvula1, valvula2) in enumerate(
        TOMAS_PURGA,
        start=1
    ):

        print(
            f"Purgando toma {numero}: "
            f"válvulas {valvula1} y {valvula2}"
        )

        # Abrir válvulas de la toma

        await enviar_comando(
            socket_arduino,
            f"{valvula1}A"
        )

        await enviar_comando(
            socket_arduino,
            f"{valvula2}A"
        )

        # Esperar 3 segundos

        await asyncio.sleep(
            TIEMPO_ENTRE_TOMAS
        )

        # Cerrar válvulas de la toma

        await enviar_comando(
            socket_arduino,
            f"{valvula1}C"
        )

        await enviar_comando(
            socket_arduino,
            f"{valvula2}C"
        )


    # --------------------------------------------------------
    # Cerrar válvulas principales de purga
    # --------------------------------------------------------

    print("Cerrando válvulas de purga")

    await enviar_comando(
        socket_arduino,
        "17C"
    )

    await enviar_comando(
        socket_arduino,
        "18C"
    )

    print("================================")
    print("PURGA FINALIZADA")
    print("================================")


# ============================================================
# FIN DE CONEXIÓN
# ============================================================

async def fin_conexion_equipo(
    socket_arduino
):

    await enviar_comando(
        socket_arduino,
        "A"
    )


# ============================================================
# MENSAJES DEL ARDUINO
# ============================================================

def procesar_mensaje_de_arduino(
    mensaje
):
    print(mensaje)