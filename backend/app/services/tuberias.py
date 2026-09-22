import json
import asyncio


# ============================================================
# CONFIGURACIÓN DEL EQUIPO
# ============================================================

ID_EQUIPO = 1


# ============================================================
# CONFIGURACIÓN DE PURGA
# ============================================================

# Tiempo que permanecen abiertas las válvulas de cada toma
# mientras se realiza la purga.
TIEMPO_PURGA = 60


# Tiempo de estabilización del circuito después de seleccionar
# una cañería.
TIEMPO_ESTABILIZACION = 120


# Durante el cambio de cañería:
# 1. se abre la nueva cañería
# 2. se esperan 5 segundos
# 3. se cierra la cañería anterior
#
# Después de mandar el cierre se espera nuevamente 5 segundos
# antes de permitir otra operación.
#
# Esto evita que se envíen comandos mientras la cañería anterior
# todavía puede estar cerrándose.
TIEMPO_CAMBIO_CANERIA = 5


# ============================================================
# VÁLVULAS
# ============================================================

# Cañerías principales
CANERIAS = [
    19,  # Cañería 1
    20,  # Cañería 2
    21,  # Cañería 3
    22,  # Cañería 4
]


# Purgadores principales
VALVULA_PURGA_1 = 17
VALVULA_PURGA_2 = 18


# Cada toma está formada por dos válvulas.
#
# Toma 1       -> 1, 2
# Toma 2       -> 3, 4
# Diafragma    -> 5, 6
# Tobera       -> 7, 8
# Contracción  -> 9, 10
# V. Globo     -> 11, 12
# V. Esclusa   -> 13, 14
# Expansión    -> 15, 16
#
TOMAS = [
    (1, 2),
    (3, 4),
    (5, 6),
    (7, 8),
    (9, 10),
    (11, 12),
    (13, 14),
    (15, 16),
]


# A qué cañería pertenece cada toma.
#
# Cañería 1 -> Toma 1
# Cañería 2 -> Toma 2
# Cañería 3 -> Diafragma, Tobera, Contracción
# Cañería 4 -> V. Globo, V. Esclusa, Expansión
#
TOMAS_POR_CANERIA = [
    [0],
    [1],
    [2, 3, 4],
    [5, 6, 7],
]


# ============================================================
# ESTADO DE LA PURGA
# ============================================================

purga_en_curso = False
caneria_actual = None


# ============================================================
# COMANDOS DESDE EL FRONTEND
# ============================================================

async def enviar_comando_al_arduino(
    comando: str,
    socket_arduino
):
    """
    Recibe el JSON enviado por el frontend y transmite solamente
    el campo cmd al Arduino.
    """

    comando_dict = json.loads(comando)

    cmd = comando_dict.get("cmd")

    if cmd is None:
        raise ValueError(
            "El comando recibido no contiene el campo 'cmd'."
        )

    await enviar_comando(
        socket_arduino,
        cmd
    )


# ============================================================
# ENVIAR COMANDO DIRECTO AL ARDUINO
# ============================================================

async def enviar_comando(
    socket_arduino,
    comando: str
):
    """
    Envía un comando terminado en salto de línea al Arduino.
    """

    oracion = comando + "\n"

    await socket_arduino.enviar(
        oracion.encode("utf-8")
    )

    print(f"Arduino <- {comando}")


# ============================================================
# VÁLVULA
# ============================================================

async def abrir_valvula(
    socket_arduino,
    numero: int
):
    await enviar_comando(
        socket_arduino,
        f"{numero}A"
    )


async def cerrar_valvula(
    socket_arduino,
    numero: int
):
    await enviar_comando(
        socket_arduino,
        f"{numero}C"
    )


# ============================================================
# TOMAS
# ============================================================

async def abrir_toma(
    socket_arduino,
    toma
):
    """
    Abre simultáneamente las dos válvulas de una toma.
    """

    valvula1, valvula2 = toma

    await abrir_valvula(
        socket_arduino,
        valvula1
    )

    await abrir_valvula(
        socket_arduino,
        valvula2
    )


async def cerrar_toma(
    socket_arduino,
    toma
):
    """
    Cierra las dos válvulas de una toma.
    """

    valvula1, valvula2 = toma

    await cerrar_valvula(
        socket_arduino,
        valvula1
    )

    await cerrar_valvula(
        socket_arduino,
        valvula2
    )


# ============================================================
# PURGADORES
# ============================================================

async def abrir_purgadores(
    socket_arduino
):
    print("Abriendo purgadores 17 y 18.")

    await abrir_valvula(
        socket_arduino,
        VALVULA_PURGA_1
    )

    await abrir_valvula(
        socket_arduino,
        VALVULA_PURGA_2
    )


async def cerrar_purgadores(
    socket_arduino
):
    print("Cerrando purgadores 17 y 18.")

    await cerrar_valvula(
        socket_arduino,
        VALVULA_PURGA_1
    )

    await cerrar_valvula(
        socket_arduino,
        VALVULA_PURGA_2
    )


# ============================================================
# BOMBA
# ============================================================

async def encender_bomba(
    socket_arduino
):
    """
    P -> bomba encendida
    """

    await enviar_comando(
        socket_arduino,
        "P"
    )


async def apagar_bomba(
    socket_arduino
):
    """
    A -> bomba apagada
    """

    await enviar_comando(
        socket_arduino,
        "A"
    )


# ============================================================
# PURGAR UNA TOMA
# ============================================================

async def purgar_toma(
    socket_arduino,
    numero_toma
):
    """
    Secuencia:

        abrir las dos válvulas de la toma
        abrir purgadores
        esperar 1 minuto
        cerrar purgadores
        cerrar las dos válvulas de la toma
    """

    toma = TOMAS[numero_toma]

    print(
        "--------------------------------"
    )

    print(
        f"Purgando toma {numero_toma + 1}: "
        f"válvulas {toma[0]} y {toma[1]}"
    )

    # --------------------------------------------------------
    # Abrir la toma
    # --------------------------------------------------------

    await abrir_toma(
        socket_arduino,
        toma
    )

    # --------------------------------------------------------
    # Abrir purgadores
    # --------------------------------------------------------

    await abrir_purgadores(
        socket_arduino
    )

    # --------------------------------------------------------
    # Mantener purgadores abiertos durante 1 minuto
    # --------------------------------------------------------

    print(
        f"Purgando durante {TIEMPO_PURGA} segundos."
    )

    await asyncio.sleep(
        TIEMPO_PURGA
    )

    # --------------------------------------------------------
    # Cerrar purgadores
    # --------------------------------------------------------

    await cerrar_purgadores(
        socket_arduino
    )

    # --------------------------------------------------------
    # Cerrar toma
    # --------------------------------------------------------

    await cerrar_toma(
        socket_arduino,
        toma
    )

    print(
        f"Toma {numero_toma + 1} purgada."
    )


# ============================================================
# PURGAR UNA CAÑERÍA
# ============================================================
async def purgar_caneria(
    socket_arduino,
    numero_caneria
):
    """
    Purga todas las tomas pertenecientes a una cañería.

    Cada toma se purga individualmente durante
    TIEMPO_PURGA segundos.
    """

    tomas = TOMAS_POR_CANERIA[numero_caneria]

    print(
        "================================"
    )

    print(
        f"PURGA CAÑERÍA {numero_caneria + 1}"
    )

    print(
        "Tomas:",
        [numero + 1 for numero in tomas]
    )

    print(
        "================================"
    )

    # --------------------------------------------------------
    # Procesar cada toma individualmente
    # --------------------------------------------------------

    for indice_toma in tomas:

        print(
            f"Purgando toma {indice_toma + 1}"
        )

        # ----------------------------------------------------
        # Abrir toma
        # ----------------------------------------------------

        await abrir_toma(
            socket_arduino,
            TOMAS[indice_toma]
        )

        # ----------------------------------------------------
        # Abrir purgadores
        # ----------------------------------------------------

        await abrir_purgadores(
            socket_arduino
        )

        # ----------------------------------------------------
        # Esperar el tiempo de purga
        # ----------------------------------------------------

        print(
            f"Purgando toma {indice_toma + 1} "
            f"durante {TIEMPO_PURGA} segundos."
        )

        await asyncio.sleep(
            TIEMPO_PURGA
        )

        # ----------------------------------------------------
        # Cerrar purgadores
        # ----------------------------------------------------

        await cerrar_purgadores(
            socket_arduino
        )

        # ----------------------------------------------------
        # Cerrar toma
        # ----------------------------------------------------

        await cerrar_toma(
            socket_arduino,
            TOMAS[indice_toma]
        )

        print(
            f"Toma {indice_toma + 1} purgada."
        )

    print(
        f"Purga de cañería {numero_caneria + 1} terminada."
    )
    
# ============================================================
# CAMBIO DE CAÑERÍA
# ============================================================

async def cambiar_caneria(
    socket_arduino,
    caneria_anterior,
    caneria_nueva
):
    """
    Realiza el cambio de cañería.

    Secuencia:

        1. abrir nueva cañería
        2. esperar 5 segundos
        3. cerrar cañería anterior
        4. esperar 5 segundos antes de permitir
           cualquier otra operación

    Durante toda esta función la purga permanece bloqueada.
    """

    global caneria_actual

    valvula_anterior = CANERIAS[caneria_anterior]

    valvula_nueva = CANERIAS[caneria_nueva]

    print(
        "================================"
    )

    print(
        f"CAMBIO DE CAÑERÍA "
        f"{caneria_anterior + 1} -> "
        f"{caneria_nueva + 1}"
    )

    print(
        "================================"
    )

    # --------------------------------------------------------
    # Abrir nueva cañería
    # --------------------------------------------------------

    print(
        f"Abriendo cañería {caneria_nueva + 1} "
        f"(válvula {valvula_nueva})"
    )

    await abrir_valvula(
        socket_arduino,
        valvula_nueva
    )

    # --------------------------------------------------------
    # Mantener ambas cañerías abiertas durante 5 segundos
    # --------------------------------------------------------

    print(
        f"Esperando {TIEMPO_CAMBIO_CANERIA} segundos..."
    )

    await asyncio.sleep(
        TIEMPO_CAMBIO_CANERIA
    )

    # --------------------------------------------------------
    # Cerrar cañería anterior
    # --------------------------------------------------------

    print(
        f"Cerrando cañería {caneria_anterior + 1} "
        f"(válvula {valvula_anterior})"
    )

    await cerrar_valvula(
        socket_arduino,
        valvula_anterior
    )

    # --------------------------------------------------------
    # Esperar antes de permitir otra operación
    # --------------------------------------------------------

    print(
        f"Esperando {TIEMPO_CAMBIO_CANERIA} segundos "
        "para asegurar el cierre."
    )

    await asyncio.sleep(
        TIEMPO_CAMBIO_CANERIA
    )

    caneria_actual = caneria_nueva

    print(
        "Cambio de cañería terminado."
    )


# ============================================================
# PURGA COMPLETA
# ============================================================

async def ejecutar_purga(
    socket_arduino,
    caneria_inicial=0
):
    """
    Ejecuta la purga completa del panel.

    Secuencia general:

        1. Seleccionar cañería inicial.
        2. Verificar regulador cerrado.
        3. Encender bomba.
        4. Regular caudal.
        5. Esperar 2 minutos.
        6. Purgar las tomas de la cañería.
        7. Cambiar de cañería.
        8. Esperar 2 minutos.
        9. Repetir.
       10. Finalizar purga.
    """

    global purga_en_curso
    global caneria_actual

    if purga_en_curso:

        print(
            "Ya hay una purga en curso."
        )

        return

    purga_en_curso = True

    try:

        print(
            "################################"
        )

        print(
            "INICIANDO PURGA COMPLETA"
        )

        print(
            "################################"
        )

        # ----------------------------------------------------
        # Seguridad: cerrar todas las cañerías
        # ----------------------------------------------------

        print(
            "Cerrando todas las cañerías antes de comenzar."
        )

        for valvula in CANERIAS:

            await cerrar_valvula(
                socket_arduino,
                valvula
            )

        # ----------------------------------------------------
        # Seleccionar cañería inicial
        # ----------------------------------------------------

        if (
            caneria_inicial < 0 or
            caneria_inicial >= len(CANERIAS)
        ):

            raise ValueError(
                "Número de cañería inicial inválido."
            )

        caneria_actual = caneria_inicial

        valvula_inicial = CANERIAS[caneria_inicial]

        print(
            f"Abriendo cañería inicial "
            f"{caneria_inicial + 1}."
        )

        await abrir_valvula(
            socket_arduino,
            valvula_inicial
        )

        # ----------------------------------------------------
        # Paso 2:
        # el regulador debe estar cerrado.
        #
        # No se manda aquí ningún movimiento del regulador
        # porque el valor mecánico inicial depende del equipo.
        # El Arduino debe estar inicializado con el regulador
        # cerrado.
        # ----------------------------------------------------

        print(
            "Regulador de caudal: debe encontrarse cerrado."
        )

        # ----------------------------------------------------
        # Paso 3:
        # encender bomba
        # ----------------------------------------------------

        print(
            "Encendiendo bomba."
        )

        await encender_bomba(
            socket_arduino
        )

        # ----------------------------------------------------
        # Paso 4:
        # regular caudal
        #
        # No se fija un valor porque no fue indicado.
        # El operador/equipo debe establecer el caudal
        # correspondiente antes de continuar.
        #
        # Si el valor debe ser automático, puede agregarse aquí.
        # ----------------------------------------------------

        print(
            "Caudal establecido. "
            "Comenzando estabilización."
        )

        # ----------------------------------------------------
        # Paso 5:
        # esperar 2 minutos
        # ----------------------------------------------------

        print(
            f"Esperando {TIEMPO_ESTABILIZACION} segundos "
            "para estabilizar el circuito."
        )

        await asyncio.sleep(
            TIEMPO_ESTABILIZACION
        )

        # ----------------------------------------------------
        # Purga de cada cañería
        # ----------------------------------------------------

        for numero_caneria in range(
            caneria_inicial,
            len(CANERIAS)
        ):

            if numero_caneria != caneria_actual:

                await cambiar_caneria(
                    socket_arduino,
                    caneria_actual,
                    numero_caneria
                )

                # --------------------------------------------
                # Nueva estabilización
                # --------------------------------------------
                """    
                print(
                    f"Esperando "
                    f"{TIEMPO_ESTABILIZACION} segundos "
                    f"para estabilizar cañería "
                    f"{numero_caneria + 1}."
                )

                await asyncio.sleep(
                    TIEMPO_ESTABILIZACION
                )
                """
            # ------------------------------------------------
            # Purga de la cañería actual
            # ------------------------------------------------

            await purgar_caneria(
                socket_arduino,
                numero_caneria
            )

        # ----------------------------------------------------
        # Purga finalizada
        # ----------------------------------------------------

        print(
            "################################"
        )

        print(
            "PURGA COMPLETA FINALIZADA"
        )

        print(
            "################################"
        )

        caneria_actual = None

    except asyncio.CancelledError:

        print(
            "Purga cancelada."
        )

        raise

    except Exception as error:

        print(
            f"ERROR DURANTE LA PURGA: {error}"
        )

        # Ante un error, cerrar las cañerías y apagar la bomba.
        await apagar_bomba(
            socket_arduino
        )

        for valvula in CANERIAS:

            await cerrar_valvula(
                socket_arduino,
                valvula
            )

        caneria_actual = None

        raise

    finally:

        purga_en_curso = False


# ============================================================
# INICIO DEL EQUIPO
# ============================================================

async def iniciar_equipo(
    socket_arduino
):
    print(
        "Iniciando equipo de tuberías..."
    )

    # Inicializar Arduino
    await enviar_comando(
        socket_arduino,
        "S"
    )

    print(
        "Arduino inicializado."
    )

    # --------------------------------------------------------
    # PURGA
    # --------------------------------------------------------
    #
    # Descomentar cuando se quiera realizar automáticamente
    # la purga al iniciar el equipo.
    #
    await ejecutar_purga(
         socket_arduino,
         caneria_inicial=0
     )

    print(
        "Equipo de tuberías listo."
    )


# ============================================================
# FIN DE CONEXIÓN
# ============================================================

async def fin_conexion_equipo(
    socket_arduino
):
    """
    Deja el equipo en un estado seguro.
    """

    print(
        "Finalizando conexión del equipo..."
    )

    await apagar_bomba(
        socket_arduino
    )

    # Cerrar cañerías principales
    for valvula in CANERIAS:

        await cerrar_valvula(
            socket_arduino,
            valvula
        )

    # Cerrar purgadores
    await cerrar_purgadores(
        socket_arduino
    )

    print(
        "Equipo detenido."
    )


# ============================================================
# MENSAJES DEL ARDUINO
# ============================================================

def procesar_mensaje_de_arduino(
    mensaje
):
    print(mensaje)
