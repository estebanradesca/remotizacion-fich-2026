
import asyncio
from typing import Optional


class ServicioPelton:
    """
    Servicio de control de la turbina Pelton.

    Arduino de válvulas:
        - Bomba
        - Electroválvula
        - Sensor de presión P4
        - Sensor de presión P20
        - Sensor Hall / RPM

    Arduino de motores:
        - Motor 2: regulación de caudal
        - Servo: apertura y cierre de aguja
    """

    MOTOR2_POS_MIN = 0
    MOTOR2_POS_MAX = 1300

    SERVO_ANGULO_MIN = 0.0
    SERVO_ANGULO_MAX = 11.5

    def __init__(
        self,
        socket_valvulas,
        socket_motores,
        angulo_aguja_abierta=11.5,
        angulo_aguja_cerrada=0.0,
    ):
        self.socket_valvulas = socket_valvulas
        self.socket_motores = socket_motores

        # =====================================================
        # ESTADO DE LOS DISPOSITIVOS
        # =====================================================

        self.bomba_encendida = False
        self.valvula_activa = False

        # =====================================================
        # SENSORES DE PRESIÓN
        # =====================================================

        self.presion_p4: Optional[float] = None
        self.presion_p20: Optional[float] = None

        # =====================================================
        # SENSOR HALL / RPM
        # =====================================================

        self.rpm: Optional[float] = None
        self.rpm_midiendo = False
        self.rpm_habilitado = True
        self.rpm_completado = False

        # =====================================================
        # MOTOR 2 - CAUDAL
        # =====================================================

        # Posición objetivo estimada.
        # No es una medición física de la posición del motor.
        self.posicion_objetivo_motor2 = 0
        self.motor2_habilitado = True

        # =====================================================
        # SERVO - AGUJA
        # =====================================================

        self.servo_inicializado = False

        self.angulo_aguja_abierta = float(angulo_aguja_abierta)
        self.angulo_aguja_cerrada = float(angulo_aguja_cerrada)

        self.angulo_aguja_actual: Optional[float] = None

        # =====================================================
        # SINCRONIZACIÓN
        # =====================================================

        self.lock_valvulas = asyncio.Lock()
        self.lock_motores = asyncio.Lock()

        self.lock_motor2 = asyncio.Lock()

        # =====================================================
        # VALIDACIÓN DE CONFIGURACIÓN DEL SERVO
        # =====================================================

        self._validar_angulo(self.angulo_aguja_abierta)
        self._validar_angulo(self.angulo_aguja_cerrada)

    # =========================================================
    # ENVÍO DE COMANDOS
    # =========================================================

    async def _enviar_valvulas(self, comando: str):
        """
        Envía un comando al Arduino de válvulas.
        """

        async with self.lock_valvulas:
            await self.socket_valvulas.enviar(comando + "\n")

    async def _enviar_motores(self, comando: str):
        """
        Envía un comando al Arduino de motores.
        """

        async with self.lock_motores:
            await self.socket_motores.enviar(comando + "\n")

    # =========================================================
    # BOMBA
    # =========================================================

    async def encender_bomba(self):
        await self._enviar_valvulas("B ON")

    async def apagar_bomba(self):
        await self._enviar_valvulas("B OFF")

    async def alternar_bomba(self):
        if self.bomba_encendida:
            await self.apagar_bomba()
        else:
            await self.encender_bomba()

    # =========================================================
    # ELECTROVÁLVULA
    # =========================================================

    async def abrir_electrovalvula(self):
        await self._enviar_valvulas("V ON")

    async def cerrar_electrovalvula(self):
        await self._enviar_valvulas("V OFF")

    async def alternar_electrovalvula(self):
        if self.valvula_activa:
            await self.cerrar_electrovalvula()
        else:
            await self.abrir_electrovalvula()

    # =========================================================
    # SENSOR P4
    # =========================================================

    async def habilitar_p4(self):
        await self._enviar_valvulas("P4 ON")

    async def deshabilitar_p4(self):
        await self._enviar_valvulas("P4 OFF")

    async def leer_p4(self):
        await self._enviar_valvulas("P4")

    async def resetear_p4(self):
        await self._enviar_valvulas("P4 RESET")

    # =========================================================
    # SENSOR P20
    # =========================================================

    async def habilitar_p20(self):
        await self._enviar_valvulas("P20 ON")

    async def deshabilitar_p20(self):
        await self._enviar_valvulas("P20 OFF")

    async def leer_p20(self):
        await self._enviar_valvulas("P20")

    async def resetear_p20(self):
        await self._enviar_valvulas("P20 RESET")

    # =========================================================
    # SENSOR HALL / RPM
    # =========================================================

    async def iniciar_medicion_rpm(self):
        if not self.rpm_habilitado:
            raise RuntimeError("El sensor Hall está deshabilitado.")

        await self._enviar_valvulas("RPM")

        self.rpm_midiendo = True
        self.rpm_completado = False

    async def detener_medicion_rpm(self):
        await self._enviar_valvulas("RPM STOP")

        self.rpm_midiendo = False

    async def resetear_rpm(self):
        await self._enviar_valvulas("RPM RESET")

        self.rpm = 0
        self.rpm_midiendo = False
        self.rpm_completado = False

    async def habilitar_rpm(self):
        await self._enviar_valvulas("RPM ON")

        self.rpm_habilitado = True
        self.rpm_midiendo = False
        self.rpm_completado = False

    async def deshabilitar_rpm(self):
        await self._enviar_valvulas("RPM OFF")

        self.rpm_habilitado = False
        self.rpm_midiendo = False
        self.rpm_completado = False
        self.rpm = None

    # =========================================================
    # MOTOR 2 - REGULACIÓN DE CAUDAL
    # =========================================================

    async def mover_motor2_a(self, posicion: int):
        """
        Mueve el motor 2 a una posición objetivo.

        La posición admitida está entre 0 y 1300.

        El movimiento se calcula respecto de la última posición
        objetivo enviada por el backend.
        """

        if isinstance(posicion, bool) or not isinstance(posicion, int):
            raise TypeError("La posición del motor 2 debe ser un entero.")

        if not self.MOTOR2_POS_MIN <= posicion <= self.MOTOR2_POS_MAX:
            raise ValueError(
                f"La posición debe estar entre "
                f"{self.MOTOR2_POS_MIN} y {self.MOTOR2_POS_MAX}."
            )

        async with self.lock_motor2:

            if not self.motor2_habilitado:
                raise RuntimeError("El motor 2 está deshabilitado.")

            actual = self.posicion_objetivo_motor2
            diferencia = posicion - actual

            if diferencia == 0:
                return

            if diferencia > 0:
                comando = f"2+{diferencia}"
            else:
                comando = f"2-{abs(diferencia)}"

            await self._enviar_motores(comando)

            self.posicion_objetivo_motor2 = posicion

    async def abrir_motor2(self, pasos: int):
        """
        Incrementa la posición objetivo del motor 2.
        """

        if pasos <= 0:
            raise ValueError("Los pasos deben ser positivos.")

        async with self.lock_motor2:
            nueva_posicion = self.posicion_objetivo_motor2 + pasos

            if nueva_posicion > self.MOTOR2_POS_MAX:
                raise ValueError("Se superó el límite máximo del motor 2.")

            if not self.motor2_habilitado:
                raise RuntimeError("El motor 2 está deshabilitado.")

            comando = f"2+{pasos}"

            await self._enviar_motores(comando)

            self.posicion_objetivo_motor2 = nueva_posicion

    async def cerrar_motor2(self, pasos: int):
        """
        Decrementa la posición objetivo del motor 2.
        """

        if pasos <= 0:
            raise ValueError("Los pasos deben ser positivos.")

        async with self.lock_motor2:
            nueva_posicion = self.posicion_objetivo_motor2 - pasos

            if nueva_posicion < self.MOTOR2_POS_MIN:
                raise ValueError("Se superó el límite mínimo del motor 2.")

            if not self.motor2_habilitado:
                raise RuntimeError("El motor 2 está deshabilitado.")

            comando = f"2-{pasos}"

            await self._enviar_motores(comando)

            self.posicion_objetivo_motor2 = nueva_posicion

    async def detener_motor2(self):
        await self._enviar_motores("2S")

    async def habilitar_motor2(self):
        await self._enviar_motores("2H")
        self.motor2_habilitado = True

    async def deshabilitar_motor2(self):
        await self._enviar_motores("2S")
        await self._enviar_motores("2D")

        self.motor2_habilitado = False

    def resetear_posicion_objetivo_motor2(self):
        """
        Reinicia únicamente la referencia lógica del backend.

        No mueve físicamente el motor ni realiza homing.
        """

        self.posicion_objetivo_motor2 = 0

    # =========================================================
    # SERVO - AGUJA
    # =========================================================

    def _validar_angulo(self, angulo: float):
        if not self.SERVO_ANGULO_MIN <= angulo <= self.SERVO_ANGULO_MAX:
            raise ValueError(
                f"El ángulo debe estar entre "
                f"{self.SERVO_ANGULO_MIN} y {self.SERVO_ANGULO_MAX}."
            )

    async def inicializar_servo(self):
        """
        Inicializa el servo mediante INIT.
        """

        async with self.lock_motores:
            if not self.servo_inicializado:
                await self.socket_motores.enviar("INIT\n")
                self.servo_inicializado = True

    async def mover_servo(self, angulo: float):
        """
        Mueve el servo a un ángulo absoluto.
        """

        angulo = float(angulo)

        self._validar_angulo(angulo)

        async with self.lock_motores:

            if not self.servo_inicializado:
                await self.socket_motores.enviar("INIT\n")
                self.servo_inicializado = True

            await self.socket_motores.enviar(f"S{angulo:.1f}\n")

            self.angulo_aguja_actual = angulo

    async def abrir_aguja(self):
        await self.mover_servo(self.angulo_aguja_abierta)

    async def cerrar_aguja(self):
        await self.mover_servo(self.angulo_aguja_cerrada)

    def configurar_angulos_aguja(
        self,
        angulo_abierta: float,
        angulo_cerrada: float,
    ):
        """
        Configura los ángulos físicos de apertura y cierre.
        """

        angulo_abierta = float(angulo_abierta)
        angulo_cerrada = float(angulo_cerrada)

        self._validar_angulo(angulo_abierta)
        self._validar_angulo(angulo_cerrada)

        self.angulo_aguja_abierta = angulo_abierta
        self.angulo_aguja_cerrada = angulo_cerrada

    # =========================================================
    # RECEPCIÓN DE TELEMETRÍA
    # =========================================================

    def procesar_telemetria(self, mensaje: str):
        """
        Procesa los mensajes periódicos del Arduino de válvulas.

        Formato:

        P4:12.34|P20:23.45|RPM:150|BOMBA:ENCENDIDA|VALVULA:ON
        """

        if not mensaje:
            return

        mensaje = mensaje.strip()

        if not mensaje:
            return

        # Ignorar respuestas de comandos e información de inicio.
        if mensaje.startswith(("CMD:", "SYS:", "INFO:")):
            return

        datos = {}

        for campo in mensaje.split("|"):
            if ":" not in campo:
                continue

            clave, valor = campo.split(":", 1)

            datos[clave.strip().upper()] = valor.strip()

        # -----------------------------------------------------
        # PRESIÓN P4
        # -----------------------------------------------------

        if "P4" in datos:
            try:
                self.presion_p4 = float(datos["P4"])
            except ValueError:
                pass

        # -----------------------------------------------------
        # PRESIÓN P20
        # -----------------------------------------------------

        if "P20" in datos:
            try:
                self.presion_p20 = float(datos["P20"])
            except ValueError:
                pass

        # -----------------------------------------------------
        # RPM
        # -----------------------------------------------------

        if "RPM" in datos:
            valor_rpm = datos["RPM"].upper()

            if valor_rpm == "OFF":
                self.rpm = None
            else:
                try:
                    self.rpm = float(valor_rpm)
                except ValueError:
                    pass

        # -----------------------------------------------------
        # BOMBA
        # -----------------------------------------------------

        if "BOMBA" in datos:
            self.bomba_encendida = (
                datos["BOMBA"].upper() == "ENCENDIDA"
            )

        # -----------------------------------------------------
        # ELECTROVÁLVULA
        # -----------------------------------------------------

        if "VALVULA" in datos:
            self.valvula_activa = (
                datos["VALVULA"].upper() == "ON"
            )

    # =========================================================
    # ESTADO GENERAL
    # =========================================================

    def obtener_estado(self):
        """
        Devuelve el estado conocido de la turbina Pelton.
        """

        return {
            "bomba": {
                "encendida": self.bomba_encendida,
            },

            "electrovalvula": {
                "activa": self.valvula_activa,
            },

            "presiones": {
                "p4": self.presion_p4,
                "p20": self.presion_p20,
            },

            "rpm": {
                "valor": self.rpm,
                "midiendo": self.rpm_midiendo,
                "habilitado": self.rpm_habilitado,
                "completado": self.rpm_completado,
            },

            "motor2": {
                "posicion_objetivo": self.posicion_objetivo_motor2,
                "habilitado": self.motor2_habilitado,
            },

            "servo": {
                "inicializado": self.servo_inicializado,
                "angulo_actual": self.angulo_aguja_actual,
                "angulo_abierta": self.angulo_aguja_abierta,
                "angulo_cerrada": self.angulo_aguja_cerrada,
            },
        }

    # =========================================================
    # DETENCIÓN GENERAL
    # =========================================================

    async def detener_experimento(self):
        """
        Detiene el motor 2 y la medición de RPM.

        No apaga la bomba ni modifica la posición de la aguja.
        Esas acciones deben decidirse según el procedimiento
        experimental y las condiciones de seguridad.
        """

        await asyncio.gather(
            self.detener_motor2(),
            self.detener_medicion_rpm(),
        )
