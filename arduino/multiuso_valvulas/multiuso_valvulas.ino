// ============================================================
// SISTEMA UNIFICADO:
//   - ELECTROVÁLVULA + 2 SENSORES DE PRESIÓN + SENSOR HALL (RPM)
//   - CONTROL DE BOMBA MEDIANTE DOS RELÉS
// Arduino Uno (ATmega328P) - 115200 baud
// + MÓDULO ETHERNET (conversor UART-TTL a Ethernet) vía SoftwareSerial
// ============================================================

#include <SoftwareSerial.h>

// ===== DEFINICIÓN DE PINES =====
// ELECTROVÁLVULA
#define VALVULA_PIN  2

// SENSORES DE PRESIÓN
#define PRESION1_PIN  A2   // P4 (0-20 bar, 4-20 mA)
#define PRESION2_PIN  A1   // P20 (0-40 bar, 4-20 mA)

// SENSOR HALL
#define HALL_PIN      3    // Pin DIGITAL para sensor de efecto Hall

// RELÉS DE BOMBA
const int releMarcha = 4;
const int releParada = 5;

// ===== ETHERNET (SoftwareSerial) =====
// TX del conversor Ethernet -> RX del SoftwareSerial
// RX del conversor Ethernet -> TX del SoftwareSerial
#define ETH_RX_PIN  A4   // Recibe del conversor (TX del conversor)
#define ETH_TX_PIN  A5   // Envía al conversor (RX del conversor)

SoftwareSerial ethernetSerial(ETH_RX_PIN, ETH_TX_PIN);

// ===== CONSTANTES SENSORES =====
const float RANGO1 = 20.0;     // Rango P4 (bar)
const float RANGO2 = 40.0;     // Rango P20 (bar)
const float RESISTOR = 250.0;  // Resistor 250Ω para 4-20 mA

// ===== VARIABLES ELECTROVÁLVULA =====
bool valvulaActiva = false;

// ===== VARIABLES SENSORES PRESIÓN =====
float presionP4_actual = 0;
float presionP4_maxima = 0;
float presionP4_minima = 999.9;

float presionP20_actual = 0;
float presionP20_maxima = 0;
float presionP20_minima = 999.9;

// ===== VARIABLES SENSOR HALL (RPM) =====
volatile unsigned long pulsos = 0;
unsigned long tiempoInicioRPM = 0;
float rpm = 0;
unsigned long rpmUltimaActualizacion = 0;
const unsigned long INTERVALO_ACTUALIZACION_RPM = 1000;
const unsigned long TIEMPO_MEDICION_RPM = 60000;
bool midiendoRPM = false;
bool rpmCompletado = false;
unsigned long rpmFinal = 0;

// ===== VARIABLES DE HABILITACIÓN INDIVIDUAL =====
bool habilitarP4 = true;
bool habilitarP20 = true;
bool habilitarRPM = true;

// ===== VARIABLES DE CONTROL =====
bool diagnosticando = false;
unsigned long tiempoDiagnostico = 0;
unsigned long tiempoMonitoreo = 0;
const unsigned long INTERVALO_MONITOREO = 200;
bool monitoreoActivo = true;

// ===== VARIABLES BOMBA =====
const unsigned long tiempoPulso = 500; // ms
bool bombaEncendida = false;           // Estado lógico de la bomba

// ===== VARIABLES ETHERNET (NUEVO) =====
char comandoBufferEthernet[64];
uint8_t indiceComandoEthernet = 0;

// ============================================================
// FUNCIONES DE ENVÍO POR ETHERNET (NUEVO)
// ============================================================

void enviarPorEthernet(String mensaje) {
  // Enviar por SoftwareSerial (conversor UART-TTL a Ethernet)
  ethernetSerial.println(mensaje);
  // También enviar por USB para depuración
  Serial.println(mensaje);
}

void enviarRespuestaEthernet(String comando, String respuesta) {
  String mensaje = "CMD:" + comando + "|RES:" + respuesta;
  enviarPorEthernet(mensaje);
}

// ============================================================
// PROCESAMIENTO DE COMANDOS POR ETHERNET (NUEVO)
// ============================================================

void procesarComandoEthernet(String cmd) {
  cmd.trim();
  if (cmd.length() == 0) return;

  Serial.print(F("Comando recibido por ETHERNET: "));
  Serial.println(cmd);

  // Ejecuta el mismo procesador de comandos que usa el USB
  procesarComando(cmd);

  // Envía acuse de recibo por Ethernet
  enviarRespuestaEthernet(cmd, "OK");
}

// ===== FUNCIÓN DE INTERRUPCIÓN PARA RPM =====
void contarPulso() {
  if (midiendoRPM) {
    pulsos++;
  }
}

void setup() {
  // Electroválvula
  pinMode(VALVULA_PIN, OUTPUT);
  digitalWrite(VALVULA_PIN, HIGH);  // Relé activo por LOW

  // Sensores de presión
  pinMode(PRESION1_PIN, INPUT);
  pinMode(PRESION2_PIN, INPUT);

  // Sensor Hall
  pinMode(HALL_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(HALL_PIN), contarPulso, FALLING);

  // Relés de bomba
  pinMode(releMarcha, OUTPUT);
  pinMode(releParada, OUTPUT);
  digitalWrite(releMarcha, HIGH);  // Relés desactivados (módulo activo en LOW)
  digitalWrite(releParada, HIGH);

  Serial.begin(115200);
  delay(1000);

  // ===== INICIO SOFTWARESERIAL PARA ETHERNET (NUEVO) =====
  ethernetSerial.begin(115200);

  Serial.println(F("=== SISTEMA UNIFICADO: ELECTROVÁLVULA + PRESIÓN + RPM + BOMBA ==="));
  Serial.println(F(""));
  Serial.println(F("  📊 SENSORES (HABILITACIÓN INDIVIDUAL):"));
  Serial.println(F("    P4   - Pin A2 (0-20 bar)  [HABILITADO]"));
  Serial.println(F("    P20  - Pin A1 (0-40 bar)  [HABILITADO]"));
  Serial.println(F("    RPM  - Pin 3  (Sensor Hall) [HABILITADO]"));
  Serial.println(F(""));
  Serial.println(F("  ELECTROVÁLVULA:"));
  Serial.println(F("    V ON      - Activar"));
  Serial.println(F("    V OFF     - Desactivar"));
  Serial.println(F("    V TOGGLE  - Cambiar estado"));
  Serial.println(F(""));
  Serial.println(F("  BOMBA:"));
  Serial.println(F("    B ON      - Arrancar bomba (pulso marcha)"));
  Serial.println(F("    B OFF     - Apagar bomba (pulso parada)"));
  Serial.println(F("    B TOGGLE  - Conmutar bomba"));
  Serial.println(F("    (También: 'P'/'p' = Prender, 'A'/'a' = Apagar)"));
  Serial.println(F(""));
  Serial.println(F("  SENSORES PRESIÓN:"));
  Serial.println(F("    P4        - Leer P4"));
  Serial.println(F("    P20       - Leer P20"));
  Serial.println(F("    P4 MAX    - Máxima P4"));
  Serial.println(F("    P4 MIN    - Mínima P4"));
  Serial.println(F("    P4 RESET  - Resetear P4"));
  Serial.println(F("    P20 MAX   - Máxima P20"));
  Serial.println(F("    P20 MIN   - Mínima P20"));
  Serial.println(F("    P20 RESET - Resetear P20"));
  Serial.println(F(""));
  Serial.println(F("  HABILITAR/DESHABILITAR SENSORES:"));
  Serial.println(F("    P4 ON     - Habilitar P4"));
  Serial.println(F("    P4 OFF    - Deshabilitar P4"));
  Serial.println(F("    P20 ON    - Habilitar P20"));
  Serial.println(F("    P20 OFF   - Deshabilitar P20"));
  Serial.println(F("    RPM ON    - Habilitar RPM"));
  Serial.println(F("    RPM OFF   - Deshabilitar RPM"));
  Serial.println(F(""));
  Serial.println(F("  RPM:"));
  Serial.println(F("    RPM       - Iniciar medición de RPM (1 minuto)"));
  Serial.println(F("    RPM STOP  - Detener medición de RPM"));
  Serial.println(F("    RPM RESET - Resetear contador de RPM"));
  Serial.println(F(""));
  Serial.println(F("  MONITOREO:"));
  Serial.println(F("    M         - Pausar/Reanudar monitoreo"));
  Serial.println(F(""));
  Serial.println(F("  DIAGNÓSTICO:"));
  Serial.println(F("    DIAG      - Activar/Desactivar diagnóstico"));
  Serial.println(F("    ESTADO    - Ver estado completo"));
  Serial.println(F("================================"));
  Serial.println(F(""));

  // ===== MENSAJES DE INICIO POR ETHERNET (NUEVO) =====
  enviarPorEthernet("SYS:STARTED|INFO:Sistema unificado (valvula+presion+RPM+bomba) listo");
  enviarPorEthernet("INFO:Use los mismos comandos del monitor serie");
}

void loop() {
  // ===== PROCESAR COMANDOS POR SoftwareSerial (ETHERNET) (NUEVO) =====
  while (ethernetSerial.available()) {
    char c = ethernetSerial.read();

    if (c == '\n' || c == '\r') {
      if (indiceComandoEthernet > 0) {
        comandoBufferEthernet[indiceComandoEthernet] = '\0';
        procesarComandoEthernet(String(comandoBufferEthernet));
        indiceComandoEthernet = 0;
      }
    } else if (c != '\0') {
      // Limitar el comando para evitar desbordar la RAM del Uno.
      if (indiceComandoEthernet < sizeof(comandoBufferEthernet) - 1) {
        comandoBufferEthernet[indiceComandoEthernet++] = c;
      } else {
        indiceComandoEthernet = 0; // descartar comando demasiado largo
      }
    }
  }

  // ===== PROCESAR COMANDOS POR USB (MONITOR SERIE) =====
  if (Serial.available() > 0) {
    String comando = Serial.readStringUntil('\n');
    comando.trim();
    procesarComando(comando);
  }

  // ===== MEDICIÓN DE RPM EN TIEMPO REAL =====
  if (habilitarRPM && midiendoRPM) {
    unsigned long tiempoActual = millis();

    if (tiempoActual - rpmUltimaActualizacion >= INTERVALO_ACTUALIZACION_RPM) {
      rpmUltimaActualizacion = tiempoActual;

      unsigned long tiempoTranscurrido = tiempoActual - tiempoInicioRPM;

      if (tiempoTranscurrido > 0) {
        noInterrupts();
        unsigned long pulsosActuales = pulsos;
        interrupts();

        rpm = (float)pulsosActuales * 60000.0 / tiempoTranscurrido;

        unsigned long tiempoRestante = (TIEMPO_MEDICION_RPM - tiempoTranscurrido) / 1000;
        Serial.print(F("📊 RPM: "));
        Serial.print(rpm, 0);
        Serial.print(F(" rev/min | Tiempo: "));
        Serial.print(tiempoTranscurrido / 1000);
        Serial.print(F("s/"));
        Serial.print(tiempoRestante);
        Serial.print(F("s rest | Pulsos: "));
        Serial.println(pulsosActuales);
      }
    }

    if (tiempoActual - tiempoInicioRPM >= TIEMPO_MEDICION_RPM) {
      noInterrupts();
      unsigned long pulsosTotales = pulsos;
      interrupts();

      rpmFinal = pulsosTotales;
      rpm = rpmFinal;

      Serial.println(F("========================================"));
      Serial.print(F("✅ RPM FINAL (1 minuto): "));
      Serial.print(rpmFinal);
      Serial.println(F(" rev/min"));
      Serial.print(F("   Total de pulsos en 1 minuto: "));
      Serial.println(pulsosTotales);
      Serial.println(F("========================================"));

      midiendoRPM = false;
      rpmCompletado = true;
    }
  }

  // ===== LECTURA DE SENSORES EN TIEMPO REAL =====
if (monitoreoActivo &&
    (millis() - tiempoMonitoreo >= INTERVALO_MONITOREO)) {

  tiempoMonitoreo = millis();

  String linea = "";

  if (habilitarP4) {
    presionP4_actual = leerPresion(PRESION1_PIN, RANGO1);

    if (presionP4_actual > presionP4_maxima)
      presionP4_maxima = presionP4_actual;

    if (presionP4_actual < presionP4_minima)
      presionP4_minima = presionP4_actual;
  } else {
    presionP4_actual = 0;
  }

  if (habilitarP20) {
    presionP20_actual = leerPresion(PRESION2_PIN, RANGO2);

    if (presionP20_actual > presionP20_maxima)
      presionP20_maxima = presionP20_actual;

    if (presionP20_actual < presionP20_minima)
      presionP20_minima = presionP20_actual;
  } else {
    presionP20_actual = 0;
  }

  linea += "P4:" + String(presionP4_actual, 2);
  linea += "|P20:" + String(presionP20_actual, 2);

  linea += "|RPM:";

  if (habilitarRPM) {
    linea += String(rpm, 0);
  } else {
    linea += "OFF";
  }

  linea += "|BOMBA:";
  linea += bombaEncendida ? "ENCENDIDA" : "APAGADA";

  linea += "|VALVULA:";
  linea += valvulaActiva ? "ON" : "OFF";

  //ethernetSerial.println(linea);
  Serial.println(linea);
}

  if (diagnosticando && (millis() - tiempoDiagnostico >= 5000)) {
    tiempoDiagnostico = millis();
    mostrarDiagnostico();
  }
}

// ===== FUNCIÓN PARA INICIAR MEDICIÓN DE RPM =====
void iniciarMedicionRPM() {
  if (!habilitarRPM) {
    Serial.println(F("⚠ RPM está DESHABILITADO. Usa 'RPM ON' para habilitar"));
    return;
  }

  noInterrupts();
  pulsos = 0;
  interrupts();
  tiempoInicioRPM = millis();
  rpmUltimaActualizacion = millis();
  rpm = 0;
  rpmFinal = 0;
  rpmCompletado = false;
  midiendoRPM = true;

  Serial.println(F("========================================"));
  Serial.println(F("🔄 INICIANDO MEDICIÓN DE RPM (1 minuto)"));
  Serial.println(F("   Esperando pulsos del sensor Hall..."));
  Serial.println(F("========================================"));
}

// ===== FUNCIÓN PARA DETENER MEDICIÓN DE RPM =====
void detenerMedicionRPM() {
  if (midiendoRPM) {
    midiendoRPM = false;
    noInterrupts();
    unsigned long pulsosDetenidos = pulsos;
    interrupts();
    Serial.println(F("========================================"));
    Serial.println(F("⏹ MEDICIÓN DE RPM DETENIDA"));
    Serial.print(F("   Pulsos contados hasta ahora: "));
    Serial.println(pulsosDetenidos);
    Serial.println(F("========================================"));
  } else {
    Serial.println(F("⚠ No hay una medición de RPM activa"));
  }
}

// ===== FUNCIÓN PARA ARRANCAR BOMBA =====
void arrancarBomba() {
  Serial.println(F("🔧 Arrancando bomba..."));
  digitalWrite(releMarcha, LOW);
  delay(tiempoPulso);
  digitalWrite(releMarcha, HIGH);
  bombaEncendida = true;
  Serial.println(F("✅ Bomba ENCENDIDA"));
}

// ===== FUNCIÓN PARA APAGAR BOMBA =====
void apagarBomba() {
  Serial.println(F("🔧 Apagando bomba..."));
  digitalWrite(releParada, LOW);
  delay(tiempoPulso);
  digitalWrite(releParada, HIGH);
  bombaEncendida = false;
  Serial.println(F("✅ Bomba APAGADA"));
}

void procesarComando(String cmd) {
  String comando = cmd;
  comando.toUpperCase();

  // ===== COMANDOS CORTOS BOMBA (P / A) =====
  if (cmd == "P" || cmd == "p") {
    arrancarBomba();
    return;
  }
  if (cmd == "A" || cmd == "a") {
    apagarBomba();
    return;
  }

  // ===== ELECTROVÁLVULA =====
  if (comando == "V ON") {
    digitalWrite(VALVULA_PIN, LOW);
    valvulaActiva = true;
    Serial.println(F("✅ Electroválvula ACTIVADA"));
  }
  else if (comando == "V OFF") {
    digitalWrite(VALVULA_PIN, HIGH);
    valvulaActiva = false;
    Serial.println(F("❌ Electroválvula DESACTIVADA"));
  }
  else if (comando == "V TOGGLE") {
    valvulaActiva = !valvulaActiva;
    digitalWrite(VALVULA_PIN, valvulaActiva ? LOW : HIGH);
    Serial.print(F("🔄 Electroválvula: "));
    Serial.println(valvulaActiva ? "ACTIVADA" : "DESACTIVADA");
  }

  // ===== BOMBA =====
  else if (comando == "B ON") {
    arrancarBomba();
  }
  else if (comando == "B OFF") {
    apagarBomba();
  }
  else if (comando == "B TOGGLE") {
    if (bombaEncendida) {
      apagarBomba();
    } else {
      arrancarBomba();
    }
  }

  // ===== SENSOR P4 =====
  else if (comando == "P4") {
    if (!habilitarP4) {
      Serial.println(F("⚠ P4 está DESHABILITADO. Usa 'P4 ON' para habilitar"));
      return;
    }
    float p = leerPresion(PRESION1_PIN, RANGO1);
    Serial.print(F("📊 P4: "));
    Serial.print(p, 2);
    Serial.println(F(" bar"));
    mostrarCorriente(PRESION1_PIN);
  }
  else if (comando == "P4 MAX") {
    Serial.print(F("📊 P4 - Máxima: "));
    Serial.print(presionP4_maxima, 2);
    Serial.println(F(" bar"));
  }
  else if (comando == "P4 MIN") {
    Serial.print(F("📊 P4 - Mínima: "));
    Serial.print(presionP4_minima, 2);
    Serial.println(F(" bar"));
  }
  else if (comando == "P4 RESET") {
    presionP4_maxima = 0;
    presionP4_minima = 999.9;
    Serial.println(F("🔄 P4 reseteados"));
  }
  else if (comando == "P4 ON") {
    habilitarP4 = true;
    presionP4_actual = leerPresion(PRESION1_PIN, RANGO1);
    Serial.println(F("✅ P4 HABILITADO"));
  }
  else if (comando == "P4 OFF") {
    habilitarP4 = false;
    presionP4_actual = 0;
    presionP4_maxima = 0;
    presionP4_minima = 999.9;
    Serial.println(F("❌ P4 DESHABILITADO (deja de leer)"));
  }

  // ===== SENSOR P20 =====
  else if (comando == "P20") {
    if (!habilitarP20) {
      Serial.println(F("⚠ P20 está DESHABILITADO. Usa 'P20 ON' para habilitar"));
      return;
    }
    float p = leerPresion(PRESION2_PIN, RANGO2);
    Serial.print(F("📊 P20: "));
    Serial.print(p, 2);
    Serial.println(F(" bar"));
    mostrarCorriente(PRESION2_PIN);
  }
  else if (comando == "P20 MAX") {
    Serial.print(F("📊 P20 - Máxima: "));
    Serial.print(presionP20_maxima, 2);
    Serial.println(F(" bar"));
  }
  else if (comando == "P20 MIN") {
    Serial.print(F("📊 P20 - Mínima: "));
    Serial.print(presionP20_minima, 2);
    Serial.println(F(" bar"));
  }
  else if (comando == "P20 RESET") {
    presionP20_maxima = 0;
    presionP20_minima = 999.9;
    Serial.println(F("🔄 P20 reseteados"));
  }
  else if (comando == "P20 ON") {
    habilitarP20 = true;
    presionP20_actual = leerPresion(PRESION2_PIN, RANGO2);
    Serial.println(F("✅ P20 HABILITADO"));
  }
  else if (comando == "P20 OFF") {
    habilitarP20 = false;
    presionP20_actual = 0;
    presionP20_maxima = 0;
    presionP20_minima = 999.9;
    Serial.println(F("❌ P20 DESHABILITADO (deja de leer)"));
  }

  // ===== SENSOR HALL (RPM) =====
  else if (comando == "RPM") {
    iniciarMedicionRPM();
  }
  else if (comando == "RPM STOP") {
    detenerMedicionRPM();
  }
  else if (comando == "RPM RESET") {
    noInterrupts();
    pulsos = 0;
    interrupts();
    rpm = 0;
    rpmFinal = 0;
    midiendoRPM = false;
    rpmCompletado = false;
    Serial.println(F("🔄 Contador de RPM reseteados"));
  }
  else if (comando == "RPM ON") {
    habilitarRPM = true;
    pulsos = 0;
    rpm = 0;
    rpmFinal = 0;
    midiendoRPM = false;
    rpmCompletado = false;
    attachInterrupt(digitalPinToInterrupt(HALL_PIN), contarPulso, FALLING);
    Serial.println(F("✅ RPM HABILITADO (interrupción activada)"));
    Serial.println(F("   Usa 'RPM' para iniciar una medición"));
  }
  else if (comando == "RPM OFF") {
    habilitarRPM = false;
    detachInterrupt(digitalPinToInterrupt(HALL_PIN));
    rpm = 0;
    pulsos = 0;
    rpmFinal = 0;
    midiendoRPM = false;
    rpmCompletado = false;
    Serial.println(F("❌ RPM DESHABILITADO (interrupción desactivada)"));
  }

  // ===== CONTROL DE MONITOREO =====
  else if (comando == "M") {
    monitoreoActivo = !monitoreoActivo;
    Serial.print(F("🔄 Monitoreo en tiempo real: "));
    Serial.println(monitoreoActivo ? "REANUDADO" : "PAUSADO");
    if (monitoreoActivo) {
      tiempoMonitoreo = millis();
    }
  }

  // ===== DIAGNÓSTICO =====
  else if (comando == "DIAG") {
    diagnosticando = !diagnosticando;
    Serial.println(diagnosticando ? "🔍 DIAGNÓSTICO ACTIVADO" : "🔍 DIAGNÓSTICO DESACTIVADO");
  }
  else if (comando == "ESTADO") {
    mostrarDiagnostico();
  }
  else if (cmd != "") {
    Serial.print(F("❌ Comando no válido: "));
    Serial.println(cmd);
    Serial.println(F("Ejemplos:"));
    Serial.println(F("  V ON/OFF      - Controlar electroválvula"));
    Serial.println(F("  B ON/OFF      - Controlar bomba (o 'P'/'A')"));
    Serial.println(F("  P4 ON/OFF     - Habilitar/Deshabilitar P4"));
    Serial.println(F("  P20 ON/OFF    - Habilitar/Deshabilitar P20"));
    Serial.println(F("  RPM ON/OFF    - Habilitar/Deshabilitar RPM"));
    Serial.println(F("  P4            - Leer P4"));
    Serial.println(F("  P20           - Leer P20"));
    Serial.println(F("  RPM           - INICIAR medición de RPM (1 minuto)"));
    Serial.println(F("  RPM STOP      - Detener medición de RPM"));
    Serial.println(F("  RPM RESET     - Resetear contador de RPM"));
    Serial.println(F("  M             - Pausar/Reanudar monitoreo"));
    Serial.println(F("  ESTADO        - Ver estado completo"));
  }
}

// ===== FUNCIÓN LEER PRESIÓN =====
float leerPresion(int pin, float rangoMax) {
  int valor = analogRead(pin);
  float voltaje = valor * (5.0 / 1023.0);
  float corriente = voltaje / RESISTOR;

  if (corriente < 0.003) {
    return 0;
  }

  float presion = (corriente - 0.004) * (rangoMax / 0.016);

  if (presion < 0) presion = 0;
  if (presion > rangoMax) presion = rangoMax;

  return presion;
}

// ===== MOSTRAR CORRIENTE =====
void mostrarCorriente(int pin) {
  float voltaje = analogRead(pin) * (5.0 / 1023.0);
  float corriente = voltaje / RESISTOR;
  Serial.print(F("  Señal: "));
  Serial.print(corriente * 1000, 1);
  Serial.println(F(" mA"));
}

// ===== MOSTRAR DIAGNÓSTICO =====
void mostrarDiagnostico() {
  Serial.println(F("===== 📊 ESTADO COMPLETO ====="));

  Serial.print(F("Electroválvula: "));
  Serial.println(valvulaActiva ? "✅ ACTIVADA" : "❌ DESACTIVADA");

  Serial.print(F("Bomba: "));
  Serial.println(bombaEncendida ? "✅ ENCENDIDA" : "❌ APAGADA");

  Serial.print(F("Monitoreo: "));
  Serial.println(monitoreoActivo ? "🟢 ACTIVADO" : "🔴 PAUSADO");

  Serial.print(F("P4: "));
  Serial.print(habilitarP4 ? "✅ HABILITADO" : "❌ DESHABILITADO");
  if (habilitarP4) {
    Serial.print(F(" | "));
    Serial.print(presionP4_actual, 2);
    Serial.print(F(" bar | Máx: "));
    Serial.print(presionP4_maxima, 2);
    Serial.print(F(" | Mín: "));
    Serial.println(presionP4_minima, 2);
  } else {
    Serial.println(F(" (sin lectura)"));
  }

  Serial.print(F("P20: "));
  Serial.print(habilitarP20 ? "✅ HABILITADO" : "❌ DESHABILITADO");
  if (habilitarP20) {
    Serial.print(F(" | "));
    Serial.print(presionP20_actual, 2);
    Serial.print(F(" bar | Máx: "));
    Serial.print(presionP20_maxima, 2);
    Serial.print(F(" | Mín: "));
    Serial.println(presionP20_minima, 2);
  } else {
    Serial.println(F(" (sin lectura)"));
  }

  Serial.print(F("RPM: "));
  Serial.print(habilitarRPM ? "✅ HABILITADO" : "❌ DESHABILITADO");
  if (habilitarRPM) {
    Serial.print(F(" | "));
    if (midiendoRPM) {
      Serial.print(rpm, 0);
      Serial.print(F(" rev/min (MIDIENDO)"));
    } else if (rpmCompletado) {
      Serial.print(rpmFinal);
      Serial.print(F(" rev/min (FINAL)"));
    } else {
      Serial.print(F("0 rev/min (esperando 'RPM')"));
    }
    Serial.println();
  } else {
    Serial.println(F(" (interrupción desactivada)"));
  }

  Serial.println(F("=============================="));
}