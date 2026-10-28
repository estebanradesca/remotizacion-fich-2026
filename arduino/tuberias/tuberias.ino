/*
 * CONTROL DE 22 ELECTROVÁLVULAS + MOTOR PASO A PASO + BOMBA
 * Arduino Mega 2560
 * Comandos por Monitor Serie a 115200 baudios Y POR ETHERNET
 * 
 * Comandos válvulas: 1A, 1C, 2A, 2C... 22A, 22C
 * Comandos motor: M+30, M-20, A, B, H, E, P
 * Comandos bomba: P (arrancar), A (apagar)
 * 
 * PIN 22 VALVULA 1  
 * PIN 23 VALVULA 2  
 * PIN 24 VALVULA 3  
 * PIN 25 VALVULA 4    
 * PIN 26 VALVULA 5   
 * PIN 27 VALVULA 6  
 * PIN 28 VALVULA 7     
 * PIN 29 VALVULA 8
 * PIN 30 VALVULA 9
 * PIN 31 VALVULA 10
 * PIN 32 VALVULA 11
 * PIN 33 VALVULA 12
 * PIN 34 VALVULA 13
 * PIN 35 VALVULA 14
 * PIN 36 VALVULA 15
 * PIN 37 VALVULA 16
 * PIN 38 VALVULA 17
 * PIN 39 VALVULA 18
 * PIN 40 VALVULA 19
 * PIN 41 VALVULA 20
 * PIN 42 VALVULA 21
 * PIN 43 VALVULA 22
 * 
 * PIN 44 BOMBA MARCHA
 * PIN 45 BOMBA PARADA
 * 
 * //Ethernet (Conversor UART-TTL a Ethernet)
 * RX Conversor    --> PIN 18 (TX del Arduino Mega)
 * TX Conversor    --> PIN 19 (RX del Arduino Mega)
 * GND Conversor   --> GND ProtoBoard
 * VCC Conversor   --> 3.3V o 5V (según especificaciones del módulo)
 * 
 * //Comunicación USB para depuración
 * USB Arduino     --> PC
 */

// ============================================================
// DEFINICIÓN DE PINES
// ============================================================

// ===== PINES DE LOS RELÉS (22 electroválvulas) =====
const int pinesRele[22] = {
  22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43
};

// Estados de las válvulas (false = cerrada, true = abierta)
bool estadoValvula[22] = {false};

// ===== PINES DEL MOTOR PASO A PASO =====
#define STEP_PIN_1 3
#define DIR_PIN_1 2
#define ENABLE_PIN_1 4

// ===== PINES DE LA BOMBA =====
const int releMarcha = 44;
const int releParada = 45;
const unsigned long tiempoPulsoBomba = 500; // ms

// ===== VARIABLES DEL MOTOR =====
unsigned int velocidad = 5;      // Velocidad del motor
const int LIMITE_PASOS = 1250;     // Límite de pasos
int pos1 = 0;                    // Posición actual
bool habilitado1 = true;         // Motor habilitado
bool motor1Moviendo = false;     // Moviendo?
bool direccionMotor1 = true;     // Dirección
int pasosRestantesMotor1 = 0;    // Pasos restantes
unsigned long tiempoUltimoPasoMotor1 = 0;

// Buffer para comandos recibidos por Ethernet
String comandoBuffer = "";

// ============================================================
// FUNCIONES DE ENVÍO POR ETHERNET
// ============================================================

void enviarPorEthernet(String mensaje) {
  // Enviar por Serial1 (pines 18=TX, 19=RX) para conversor UART-Ethernet
  Serial1.println(mensaje);
  // También enviar por USB para depuración
  Serial.println(mensaje);
}

void enviarRespuestaEthernet(String comando, String respuesta) {
  String mensaje = "CMD:" + comando + "|RES:" + respuesta;
  enviarPorEthernet(mensaje);
}

// ============================================================
// PROCESAMIENTO DE COMANDOS POR ETHERNET
// ============================================================

void procesarComandoEthernet(String cmd) {
  cmd.trim();
  cmd.toUpperCase();
  
  if (cmd.length() == 0) return;
  
  // Procesar el comando y enviar respuesta
  String respuesta = ejecutarComando(cmd);
  enviarRespuestaEthernet(cmd, respuesta);
}

// ============================================================
// EJECUTAR COMANDO Y DEVOLVER RESPUESTA
// ============================================================

String ejecutarComando(String cmd) {
  String respuesta = "";
  
  // ===== COMANDOS DE VÁLVULAS (Ej: 1A, 5C, 22A) =====
  if ((cmd[0] >= '0' && cmd[0] <= '9') && cmd.length() >= 2) {
    // Extraer el número de válvula (1-22)
    String numStr = "";
    int i = 0;
    while (i < cmd.length() && cmd[i] >= '0' && cmd[i] <= '9') {
      numStr += cmd[i];
      i++;
    }
    
    int numValvula = numStr.toInt();
    
    // Extraer la acción (A o C)
    if (i < cmd.length() && (cmd[i] == 'A' || cmd[i] == 'C')) {
      char accion = cmd[i];
      
      if (numValvula >= 1 && numValvula <= 22) {
        if (accion == 'A') {
          abrirValvula(numValvula);
          respuesta = "OK:Valvula " + String(numValvula) + " ABIERTA";
        } else if (accion == 'C') {
          cerrarValvula(numValvula);
          respuesta = "OK:Valvula " + String(numValvula) + " CERRADA";
        }
      } else {
        respuesta = "ERROR:Valvula debe ser 1 a 22";
      }
    } else {
      respuesta = "ERROR:Formato debe ser NUMERO+A o NUMERO+C (ej: 1A, 5C)";
    }
  }
  
  // ===== COMANDOS DEL MOTOR M+30 o M-20 =====
  else if (cmd[0] == 'M' && cmd.length() >= 3) {
    char direccion = cmd[1];  // + o -
    String numPasos = "";
    
    for (int i = 2; i < cmd.length(); i++) {
      if (cmd[i] >= '0' && cmd[i] <= '9') {
        numPasos += cmd[i];
      }
    }
    
    if (numPasos.length() > 0 && (direccion == '+' || direccion == '-')) {
      int pasos = numPasos.toInt();
      if (pasos > 0) {
        moverMotor(direccion == '+', pasos);
        respuesta = "OK:Motor moviendo " + String(pasos) + " pasos " + (direccion == '+' ? "ADELANTE" : "ATRAS");
      } else {
        respuesta = "ERROR:Numero de pasos invalido";
      }
    } else {
      respuesta = "ERROR:Formato correcto: M+30 o M-20";
    }
  }
  
  // ===== COMANDOS DE LA BOMBA =====
  else if (cmd == "P") {
    arrancarBomba();
    respuesta = "OK:Bomba ARRANCADA";
  }
  else if (cmd == "A" && cmd.length() == 1) {
    // Solo ejecuta si es el comando "A" solitario (no confundir con "A" de válvulas)
    apagarBomba();
    respuesta = "OK:Bomba APAGADA";
  }
  
  // ===== COMANDOS SIMPLES DEL MOTOR =====
  else if (cmd == "A" && cmd.length() == 1) {
    // Comando A del motor (avanzar 1 paso)
    moverMotor(true, 1);
    respuesta = "OK:Motor avanza 1 paso";
  }
  else if (cmd == "B") {
    moverMotor(false, 1);
    respuesta = "OK:Motor retrocede 1 paso";
  }
  else if (cmd == "H") {
    habilitarMotor();
    respuesta = "OK:Motor HABILITADO";
  }
  else if (cmd == "E") {
    deshabilitarMotor();
    respuesta = "OK:Motor DESHABILITADO";
  }
  else if (cmd == "P" && cmd.length() == 1) {
    // Este caso ya está cubierto arriba (bomba)
  }
  
  // ===== COMANDOS DE VÁLVULAS MÚLTIPLES =====
  else if (cmd == "T") {
    abrirTodas();
    respuesta = "OK:TODAS las válvulas ABIERTAS";
  }
  else if (cmd == "X") {
    cerrarTodas();
    respuesta = "OK:TODAS las válvulas CERRADAS";
  }
  else if (cmd == "S") {
    respuesta = obtenerEstado();
  }
  else if (cmd == "?") {
    respuesta = obtenerAyuda();
  }
  else {
    respuesta = "ERROR:Comando no reconocido. Escriba '?' para ayuda";
  }
  
  return respuesta;
}

// ============================================================
// OBTENER ESTADO ACTUAL (para Ethernet)
// ============================================================

String obtenerEstado() {
  String estado = "=== ESTADO DE VÁLVULAS ===\n";
  for (int i = 0; i < 22; i++) {
    estado += "V";
    estado += String(i+1);
    estado += ": ";
    estado += estadoValvula[i] ? "ABIERTA" : "CERRADA";
    if ((i+1) % 5 == 0) estado += "\n";
    else estado += " | ";
  }
  estado += "\n===========================\n";
  estado += "Motor: ";
  estado += habilitado1 ? "HABILITADO" : "DESHABILITADO";
  estado += " | Posicion: ";
  estado += String(pos1);
  estado += "\n=== BOMBA ===\n";
  estado += "Comandos: P = Arrancar, A = Apagar\n";
  return estado;
}

String obtenerAyuda() {
  String ayuda = "";
  ayuda += "=== COMANDOS DISPONIBLES ===\n";
  ayuda += "\n";
  ayuda += "【VALVULAS 1 a 22】\n";
  ayuda += "  1A  → Abrir valvula 1\n";
  ayuda += "  1C  → Cerrar valvula 1\n";
  ayuda += "  2A  → Abrir valvula 2\n";
  ayuda += "  2C  → Cerrar valvula 2\n";
  ayuda += "  ... hasta 22A / 22C\n";
  ayuda += "  T   → Abrir TODAS las valvulas\n";
  ayuda += "  X   → Cerrar TODAS las valvulas\n";
  ayuda += "  S   → Mostrar estado de todo\n";
  ayuda += "\n";
  ayuda += "【MOTOR PASO A PASO】\n";
  ayuda += "  M+30 → Avanzar 30 pasos\n";
  ayuda += "  M-20 → Retroceder 20 pasos\n";
  ayuda += "  A    → Avanzar 1 paso\n";
  ayuda += "  B    → Retroceder 1 paso\n";
  ayuda += "  H    → Habilitar motor\n";
  ayuda += "  E    → Deshabilitar motor\n";
  ayuda += "  P    → Ver posicion actual\n";
  ayuda += "\n";
  ayuda += "【BOMBA】\n";
  ayuda += "  P    → Arrancar bomba\n";
  ayuda += "  A    → Apagar bomba\n";
  ayuda += "\n";
  ayuda += "【GENERAL】\n";
  ayuda += "  ?    → Mostrar esta ayuda\n";
  ayuda += "=============================\n";
  return ayuda;
}

// ===== FUNCIONES DE LAS VÁLVULAS =====
void abrirValvula(int num) {
  int idx = num - 1;
  if (estadoValvula[idx]) {
    Serial.print("Válvula "); Serial.print(num); Serial.println(" ya está ABIERTA");
  } else {
    digitalWrite(pinesRele[idx], LOW);  // LOW = activa relé = abre válvula
    estadoValvula[idx] = true;
    Serial.print("✓ Válvula "); Serial.print(num); Serial.println(" ABIERTA");
  }
}

void cerrarValvula(int num) {
  int idx = num - 1;
  if (!estadoValvula[idx]) {
    Serial.print("Válvula "); Serial.print(num); Serial.println(" ya está CERRADA");
  } else {
    digitalWrite(pinesRele[idx], HIGH);  // HIGH = desactiva relé = cierra válvula
    estadoValvula[idx] = false;
    Serial.print("✓ Válvula "); Serial.print(num); Serial.println(" CERRADA");
  }
}

void abrirTodas() {
  for (int i = 0; i < 22; i++) {
    digitalWrite(pinesRele[i], LOW);
    estadoValvula[i] = true;
  }
  Serial.println("✓ TODAS las válvulas ABIERTAS");
}

void cerrarTodas() {
  for (int i = 0; i < 22; i++) {
    digitalWrite(pinesRele[i], HIGH);
    estadoValvula[i] = false;
  }
  Serial.println("✓ TODAS las válvulas CERRADAS");
}

void mostrarEstado() {
  Serial.println("=== ESTADO DE VÁLVULAS ===");
  for (int i = 0; i < 22; i++) {
    Serial.print("V");
    Serial.print(i+1);
    Serial.print(": ");
    Serial.print(estadoValvula[i] ? "ABIERTA" : "CERRADA");
    if ((i+1) % 5 == 0) Serial.println();
    else Serial.print(" | ");
  }
  Serial.println("\n===========================");
  Serial.print("Motor: ");
  Serial.print(habilitado1 ? "HABILITADO" : "DESHABILITADO");
  Serial.print(" | Posición: ");
  Serial.println(pos1);
  Serial.println("=== BOMBA ===");
  Serial.println("Comandos: P = Arrancar, A = Apagar");
}

// ===== FUNCIONES DEL MOTOR =====
void moverMotor(bool dir, int pasos) {
  if (!habilitado1) {
    Serial.println("Motor deshabilitado. Use 'H' para habilitar");
    return;
  }
  if (pasos <= 0) return;

  // Aplicar límites
  if (dir && pos1 + pasos > LIMITE_PASOS) {
    pasos = LIMITE_PASOS - pos1;
    if (pasos <= 0) {
      Serial.println("Límite superior alcanzado");
      return;
    }
    Serial.print("Limitando a "); Serial.print(pasos); Serial.println(" pasos");
  }
  else if (!dir && pos1 - pasos < 0) {
    pasos = pos1;
    if (pasos <= 0) {
      Serial.println("Límite inferior alcanzado");
      return;
    }
    Serial.print("Limitando a "); Serial.print(pasos); Serial.println(" pasos");
  }

  direccionMotor1 = dir;
  pasosRestantesMotor1 = pasos;
  motor1Moviendo = true;
  tiempoUltimoPasoMotor1 = millis();

  Serial.print("▶ Motor: ");
  Serial.print(pasos);
  Serial.print(" pasos ");
  Serial.println(dir ? "ADELANTE" : "ATRÁS");
}

void actualizarMotor1() {
  if (!motor1Moviendo || !habilitado1) return;

  if (millis() - tiempoUltimoPasoMotor1 >= velocidad) {
    tiempoUltimoPasoMotor1 = millis();

    digitalWrite(DIR_PIN_1, direccionMotor1);
    digitalWrite(STEP_PIN_1, HIGH);
    delayMicroseconds(10);
    digitalWrite(STEP_PIN_1, LOW);

    if (direccionMotor1) pos1++;
    else pos1--;

    pasosRestantesMotor1--;

    if (pasosRestantesMotor1 == 0) {
      motor1Moviendo = false;
      Serial.print("✅ Motor en posición: ");
      Serial.println(pos1);
    }
  }
}

void habilitarMotor() {
  digitalWrite(ENABLE_PIN_1, LOW);
  habilitado1 = true;
  Serial.println("✅ Motor HABILITADO");
}

void deshabilitarMotor() {
  motor1Moviendo = false;
  digitalWrite(ENABLE_PIN_1, HIGH);
  habilitado1 = false;
  Serial.println("⚠ Motor DESHABILITADO");
}

// ===== FUNCIONES DE LA BOMBA =====
void arrancarBomba() {
  Serial.println("Arrancando bomba...");
  digitalWrite(releMarcha, LOW);   // Activa el relé
  delay(tiempoPulsoBomba);
  digitalWrite(releMarcha, HIGH);  // Libera el pulsador
  Serial.println("✓ Bomba ARRANCADA");
}

void apagarBomba() {
  Serial.println("Apagando bomba...");
  digitalWrite(releParada, LOW);
  delay(tiempoPulsoBomba);
  digitalWrite(releParada, HIGH);
  Serial.println("✓ Bomba APAGADA");
}

// ===== MENÚ DE AYUDA =====
void mostrarAyuda() {
  Serial.println("");
  Serial.println("=== COMANDOS DISPONIBLES ===");
  Serial.println("");
  Serial.println("【VÁLVULAS 1 a 22】");
  Serial.println("  1A  → Abrir válvula 1");
  Serial.println("  1C  → Cerrar válvula 1");
  Serial.println("  2A  → Abrir válvula 2");
  Serial.println("  2C  → Cerrar válvula 2");
  Serial.println("  ... hasta 22A / 22C");
  Serial.println("  T   → Abrir TODAS las válvulas");
  Serial.println("  X   → Cerrar TODAS las válvulas");
  Serial.println("  S   → Mostrar estado de todo");
  Serial.println("");
  Serial.println("【MOTOR PASO A PASO】");
  Serial.println("  M+30 → Avanzar 30 pasos");
  Serial.println("  M-20 → Retroceder 20 pasos");
  Serial.println("  A    → Avanzar 1 paso");
  Serial.println("  B    → Retroceder 1 paso");
  Serial.println("  H    → Habilitar motor");
  Serial.println("  E    → Deshabilitar motor");
  Serial.println("  P    → Ver posición actual");
  Serial.println("");
  Serial.println("【BOMBA】");
  Serial.println("  P    → Arrancar bomba");
  Serial.println("  A    → Apagar bomba");
  Serial.println("");
  Serial.println("【GENERAL】");
  Serial.println("  ?    → Mostrar esta ayuda");
  Serial.println("=============================");
}

// ============================================================
// SETUP - CONFIGURACIÓN INICIAL
// ============================================================

void setup() {
  // Configurar pines de relés como salida (TODAS CERRADAS al inicio)
  for (int i = 0; i < 22; i++) {
    pinMode(pinesRele[i], OUTPUT);
    digitalWrite(pinesRele[i], HIGH);  // HIGH = relé desactivado = válvula cerrada
    estadoValvula[i] = false;
  }
  
  // Configurar pines del motor
  pinMode(STEP_PIN_1, OUTPUT);
  pinMode(DIR_PIN_1, OUTPUT);
  pinMode(ENABLE_PIN_1, OUTPUT);
  digitalWrite(ENABLE_PIN_1, LOW);  // Motor habilitado por defecto
  
  // Configurar pines de la bomba                     
  pinMode(releMarcha, OUTPUT);
  pinMode(releParada, OUTPUT);
  digitalWrite(releMarcha, HIGH);   // Relés desactivados (activo en LOW)
  digitalWrite(releParada, HIGH);
  
  // Iniciar comunicación serie USB (depuración)
  Serial.begin(115200);
  
  // Iniciar comunicación Serial1 (pines 18=TX, 19=RX) para conversor UART-Ethernet
  Serial1.begin(115200);
  
  // Mostrar mensaje de inicio
  Serial.println("");
  Serial.println("=================================================");
  Serial.println("SISTEMA DE 22 ELECTROVÁLVULAS + MOTOR + BOMBA");
  Serial.println("CONVERSOR UART A ETHERNET ACTIVADO");
  Serial.println("=================================================");
  Serial.println("Serial (USB):      115200 baudios (depuración)");
  Serial.println("Serial1 (pines 18,19): 115200 baudios (conversor)");
  Serial.println("=================================================");
  Serial.println("ENVIE COMANDOS POR ETHERNET VIA EL CONVERSOR");
  Serial.println("=================================================");
  
  // Mostrar menú de ayuda
  mostrarAyuda();
  
  // Enviar mensaje de inicio por Ethernet
  enviarPorEthernet("SYS:STARTED|INFO:Sistema de 22 valvulas listo");
  enviarPorEthernet("INFO:MOTOR_LIMIT=" + String(LIMITE_PASOS));
}

// ============================================================
// LOOP PRINCIPAL
// ============================================================

void loop() {
  // Actualizar movimiento del motor
  actualizarMotor1();
  
  // ===== PROCESAR COMANDOS POR Serial1 (CONVERSOR UART-ETHERNET) =====
  while (Serial1.available()) {
    char c = Serial1.read();

    // Acumula caracteres hasta encontrar \n o \r
    if (c == '\n' || c == '\r') {
      if (comandoBuffer.length() > 0) {
        Serial.print("Comando recibido por ETHERNET: ");
        Serial.println(comandoBuffer);
        procesarComandoEthernet(comandoBuffer);
        comandoBuffer = "";
      }
    } else {
      if (c != '\0') {
        comandoBuffer += c;
      }
    }
  }
  
  // ===== PROCESAR COMANDOS POR USB (MONITOR SERIE) =====
  while (Serial.available()) {
    char c = Serial.read();
    
    if (c == '\n' || c == '\r') {
      if (comandoBuffer.length() > 0) {
        Serial.print("Comando recibido por USB: ");
        Serial.println(comandoBuffer);
        procesarComandoEthernet(comandoBuffer);
        comandoBuffer = "";
      }
    } else {
      if (c != '\0') {
        comandoBuffer += c;
      }
    }
  }
}