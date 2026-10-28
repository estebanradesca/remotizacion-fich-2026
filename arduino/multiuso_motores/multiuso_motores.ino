// VERSION OPTIMIZADA PARA ARDUINO UNO CLASICO (ATmega328P)
// Cambios: cadenas constantes en flash mediante F(), reserva y limite del buffer Ethernet.
// IMPORTANTE: Ethernet SoftwareSerial usa A4/A5 para no interferir con M3 (pines 11/12).

// CONTROL DE TRES MOTORES NEMA CON A4988 + SERVO 40KG
// Arduino Uno clasico (ATmega328P) - 115200 baud
// Motor 2 ahora funciona igual que Motor 1 (pasos variables 0-1300)
// + MÓDULO ETHERNET (conversor UART-TTL a Ethernet) vía SoftwareSerial

#include <Servo.h>
#include <SoftwareSerial.h>

// ===== DEFINICIÓN DE PINES =====
// MOTOR 1
#define DIR1_PIN   2
#define STEP1_PIN  3
#define EN1_PIN    4

// MOTOR 2
#define DIR2_PIN   8
#define STEP2_PIN  9
#define EN2_PIN    10

// MOTOR 3
#define DIR3_PIN   11
#define STEP3_PIN  12
#define EN3_PIN    13

// ===== SERVO 40KG =====
#define SERVO_PIN    6

// ===== ETHERNET (SoftwareSerial) =====
// TX del conversor Ethernet -> RX del SoftwareSerial
// RX del conversor Ethernet -> TX del SoftwareSerial
#define ETH_RX_PIN   A4   // RX SoftwareSerial: conectar al TX del conversor
#define ETH_TX_PIN   A5   // TX SoftwareSerial: conectar al RX del conversor

SoftwareSerial ethernetSerial(ETH_RX_PIN, ETH_TX_PIN);

// ===== DIRECCIONES DE AVANCE PARA CADA MOTOR =====
const bool AVANCE_MOTOR_1 = true;   // true = HIGH en el pin DIR (gira derecha)
const bool AVANCE_MOTOR_2 = true;   // true = HIGH en el pin DIR (gira derecha)
const bool AVANCE_MOTOR_3 = true;   // true = HIGH en el pin DIR (gira derecha)

// ===== VARIABLES DEL MOTOR 1 =====
const unsigned int DELAY_MOTOR_1 = 2000;
const int RECORRIDO_TOTAL_1 = 1300;
int pos1 = 0;
bool habilitado1 = true;
bool motor1Moviendo = false;
int pasosRestantesMotor1 = 0;
unsigned long tiempoUltimoPasoMotor1 = 0;
unsigned long totalPasosMotor1 = 0;

// ===== VARIABLES DEL MOTOR 2 (AHORA IGUAL AL MOTOR 1) =====
const unsigned int DELAY_MOTOR_2 = 2000;    // Velocidad fija (igual que M1)
const int RECORRIDO_TOTAL_2 = 1300;         // Mismo límite que M1 (era 750)
int pos2 = 0;
bool habilitado2 = true;
bool motor2Moviendo = false;
int pasosRestantesMotor2 = 0;
unsigned long tiempoUltimoPasoMotor2 = 0;
unsigned long totalPasosMotor2 = 0;

// ===== VARIABLES DEL MOTOR 3 =====
unsigned int delayMotor3 = 1500;        // Velocidad por defecto (microsegundos)
const int RECORRIDO_TOTAL_3 = 1000;     // Recorrido total del motor 3
int pos3 = 0;
bool habilitado3 = true;
bool motor3Moviendo = false;
int pasosRestantesMotor3 = 0;
unsigned long tiempoUltimoPasoMotor3 = 0;
unsigned long totalPasosMotor3 = 0;

// ===== VARIABLES SERVO =====
Servo servo;
float anguloActual = 0.0;          // Ángulo actual en grados
const float ANGULO_MAXIMO = 11.5;  // Máximo ángulo del servo
const float ANGULO_MINIMO = 0.0;   // Mínimo ángulo del servo
bool servoInicializado = false;

// ===== VARIABLES DE CONTROL =====
bool diagnosticando = false;
unsigned long tiempoDiagnostico = 0;

// ===== VARIABLES ETHERNET (NUEVO) =====
String comandoBufferEthernet = "";

// ============================================================
// FUNCIONES DE ENVÍO POR ETHERNET (NUEVO)
// ============================================================

void enviarPorEthernet(String mensaje) {
  // Enviar por SoftwareSerial (conversor UART-TTL a Ethernet)
  ethernetSerial.println(mensaje);
  // También enviar por USB para depuración
  Serial.println(mensaje);
}

void imprimir(String mensaje) {
  Serial.print(mensaje);
  ethernetSerial.print(mensaje);
}

void imprimirLinea(String mensaje) {
  Serial.println(mensaje);
  ethernetSerial.println(mensaje);
}

void enviarRespuestaEthernet(String comando, String respuesta) {
  String mensaje = "CMD:" + comando + "|RES:" + respuesta;
  enviarPorEthernet(mensaje);
}

// ============================================================
// PROCESAMIENTO DE COMANDOS POR ETHERNET (NUEVO)
// ============================================================

void procesarComandoEthernet(String cmd) {
  Serial.println("sdassa");
  cmd.trim();
  if (cmd.length() == 0) return;

  Serial.print(F("Comando recibido por ETHERNET: "));
  Serial.println(cmd);

  // Ejecuta el mismo procesador de comandos que usa el USB
  procesarComando(cmd);

  // Envía acuse de recibo por Ethernet
  enviarRespuestaEthernet(cmd, "OK");
}

void setup() {
  pinMode(DIR1_PIN, OUTPUT);
  pinMode(STEP1_PIN, OUTPUT);
  pinMode(EN1_PIN, OUTPUT);
  
  pinMode(DIR2_PIN, OUTPUT);
  pinMode(STEP2_PIN, OUTPUT);
  pinMode(EN2_PIN, OUTPUT);
  
  pinMode(DIR3_PIN, OUTPUT);
  pinMode(STEP3_PIN, OUTPUT);
  pinMode(EN3_PIN, OUTPUT);
  
  // Configurar servo
  servo.attach(SERVO_PIN);
  servo.write(0);  // Posición inicial 0°
  
  digitalWrite(EN1_PIN, LOW);
  digitalWrite(EN2_PIN, LOW);
  digitalWrite(EN3_PIN, LOW);
  
  Serial.begin(115200);
  delay(1000);

  // ===== INICIO SOFTWARESERIAL PARA ETHERNET (NUEVO) =====
  ethernetSerial.begin(115200);
  comandoBufferEthernet.reserve(800);
  ethernetSerial.println("hlool\r\n");
  Serial.println(F("=== CONTROL DE 3 MOTORES NEMA + SERVO 40KG ==="));
  Serial.println(F("COMANDOS DISPONIBLES:"));
  Serial.println(F(""));
  Serial.println(F("  MOTOR 1 (Válvula 1 - 0 a 1300 pasos):"));
  Serial.println(F("    1+[pasos] - Girar derecha N pasos (ABRIR)"));
  Serial.println(F("    1-[pasos] - Girar izquierda N pasos (CERRAR)"));
  Serial.println(F("    1S        - Detener motor 1"));
  Serial.println(F("    1P        - Ver posición motor 1"));
  Serial.println(F("    1H        - Habilitar motor 1"));
  Serial.println(F("    1D        - Deshabilitar motor 1"));
  Serial.println(F(""));
  Serial.println(F("  MOTOR 2 (Válvula 2 - 0 a 1300 pasos):"));
  Serial.println(F("    2+[pasos] - Girar derecha N pasos (ABRIR)"));
  Serial.println(F("    2-[pasos] - Girar izquierda N pasos (CERRAR)"));
  Serial.println(F("    2S        - Detener motor 2"));
  Serial.println(F("    2P        - Ver posición motor 2"));
  Serial.println(F("    2H        - Habilitar motor 2"));
  Serial.println(F("    2D        - Deshabilitar motor 2"));
  Serial.println(F(""));
  Serial.println(F("  MOTOR 3 (Válvula 3 - 1000 pasos fijos):"));
  Serial.println(F("    3A [vel]  - Abrir 1000 pasos con velocidad [vel] µs"));
  Serial.println(F("    3C [vel]  - Cerrar 1000 pasos con velocidad [vel] µs"));
  Serial.println(F("    3S        - Detener motor 3"));
  Serial.println(F("    3P        - Ver posición motor 3"));
  Serial.println(F("    3H        - Habilitar motor 3"));
  Serial.println(F("    3D        - Deshabilitar motor 3"));
  Serial.println(F(""));
  Serial.println(F("  SERVOMOTOR (INICIALIZACIÓN):"));
  Serial.println(F("    INIT      - Inicializar servo en 0°"));
  Serial.println(F(""));
  Serial.println(F("  SERVOMOTOR (Movimiento a ángulo específico):"));
  Serial.println(F("    S0.0      - Mover a 0.0°"));
  Serial.println(F("    S2.5      - Mover a 2.5°"));
  Serial.println(F("    S5.5      - Mover a 5.5°"));
  Serial.println(F("    S8.3      - Mover a 8.3°"));
  Serial.println(F("    S11.5     - Mover a 11.5° (máximo)"));
  Serial.println(F(""));
  Serial.println(F("  SERVOMOTOR (Movimiento incremental):"));
  Serial.println(F("    +         - Avanzar 0.1°"));
  Serial.println(F("    -         - Retroceder 0.1°"));
  Serial.println(F("    ++        - Avanzar 1.0°"));
  Serial.println(F("    --        - Retroceder 1.0°"));
  Serial.println(F(""));
  Serial.println(F("  VELOCIDAD MOTOR 3 (ejemplos):"));
  Serial.println(F("    3A 500   - M3 abre rápido (500 µs)"));
  Serial.println(F("    3C 3000  - M3 cierra lento (3000 µs)"));
  Serial.println(F("    3A       - M3 abre con velocidad por defecto"));
  Serial.println(F("    3C       - M3 cierra con velocidad por defecto"));
  Serial.println(F(""));
  Serial.println(F("  TODOS LOS MOTORES:"));
  Serial.println(F("    +[pasos]  - M1 abre N / M2 abre N / M3 abre 1000"));
  Serial.println(F("    -[pasos]  - M1 cierra N / M2 cierra N / M3 cierra 1000"));
  Serial.println(F("    SS        - Detener todos los motores"));
  Serial.println(F(""));
  Serial.println(F("  VELOCIDAD POR DEFECTO MOTOR 3:"));
  Serial.println(F("    V3+       - Aumentar velocidad M3"));
  Serial.println(F("    V3-       - Disminuir velocidad M3"));
  Serial.println(F(""));
  Serial.println(F("  DIAGNÓSTICO:"));
  Serial.println(F("    DIAG      - Activar/Desactivar diagnóstico"));
  Serial.println(F("    ESTADO    - Ver estado completo"));
  Serial.println(F("================================"));

  // ===== MENSAJES DE INICIO POR ETHERNET (NUEVO) =====
  enviarPorEthernet("SYS:STARTED|INFO:Control 3 motores NEMA + Servo listo");
  enviarPorEthernet("INFO:Use los mismos comandos del monitor serie");
  
}

void loop() {
  // ===== PROCESAR COMANDOS POR SoftwareSerial (ETHERNET) (NUEVO) =====
  while (ethernetSerial.available()) {
    Serial.println("Llegué acascsa");
    char c = ethernetSerial.read();
    if (c == '\n' || c == '\r') {
      if (comandoBufferEthernet.length() > 0) {
        procesarComandoEthernet(comandoBufferEthernet);
        comandoBufferEthernet = "";
      }
    } else {
      if (c != '\0') {
        if (comandoBufferEthernet.length() < 80) {
          comandoBufferEthernet += c;
        } else {
          comandoBufferEthernet = "";
          Serial.println(F("ERROR: comando Ethernet demasiado largo; buffer reiniciado"));
        }
      }
    }
  }

  // ===== PROCESAR COMANDOS POR USB (MONITOR SERIE) =====
  if (Serial.available() > 0) {
    Serial.println("Llegué");
    String comando = Serial.readStringUntil('\n');
    comando.trim();
    procesarComando(comando);
  }
  
  // ===== MOTOR 1 =====
  if (motor1Moviendo && habilitado1 && pasosRestantesMotor1 > 0) {
    unsigned long ahora = micros();
    if (ahora - tiempoUltimoPasoMotor1 >= DELAY_MOTOR_1) {
      digitalWrite(STEP1_PIN, HIGH);
      delayMicroseconds(2);
      digitalWrite(STEP1_PIN, LOW);
      
      if (digitalRead(DIR1_PIN) == HIGH) {
        pos1++;
        Serial.print(F("+"));
      } else {
        pos1--;
        Serial.print(F("-"));
      }
      
      pasosRestantesMotor1--;
      totalPasosMotor1++;
      tiempoUltimoPasoMotor1 = ahora;
      
      if (pos1 >= RECORRIDO_TOTAL_1) {
        pos1 = RECORRIDO_TOTAL_1;
        motor1Moviendo = false;
        pasosRestantesMotor1 = 0;
        Serial.println();
        Serial.println(F("🔔 Motor 1: VÁLVULA COMPLETAMENTE ABIERTA (1300 pasos)"));
        Serial.print(F("📊 Pasos totales dados: "));
        Serial.println(totalPasosMotor1);
      }
      else if (pos1 <= 0) {
        pos1 = 0;
        motor1Moviendo = false;
        pasosRestantesMotor1 = 0;
        Serial.println();
        Serial.println(F("🔔 Motor 1: VÁLVULA COMPLETAMENTE CERRADA (0 pasos)"));
        Serial.print(F("📊 Pasos totales dados: "));
        Serial.println(totalPasosMotor1);
      }
      
      if (pasosRestantesMotor1 == 0 && motor1Moviendo) {
        motor1Moviendo = false;
        Serial.println();
        Serial.print(F("✅ Motor 1: Movimiento completado. Posición: "));
        Serial.print(pos1);
        Serial.print(F(" de "));
        Serial.println(RECORRIDO_TOTAL_1);
        Serial.print(F("📊 Pasos totales dados: "));
        Serial.println(totalPasosMotor1);
      }
    }
  }
  
  // ===== MOTOR 2 (AHORA IGUAL AL MOTOR 1) =====
  if (motor2Moviendo && habilitado2 && pasosRestantesMotor2 > 0) {
    unsigned long ahora = micros();
    if (ahora - tiempoUltimoPasoMotor2 >= DELAY_MOTOR_2) {
      digitalWrite(STEP2_PIN, HIGH);
      delayMicroseconds(2);
      digitalWrite(STEP2_PIN, LOW);
      
      if (digitalRead(DIR2_PIN) == HIGH) {
        pos2++;
        Serial.print(F("A"));
      } else {
        pos2--;
        Serial.print(F("C"));
      }
      
      pasosRestantesMotor2--;
      totalPasosMotor2++;
      tiempoUltimoPasoMotor2 = ahora;
      
      if (pos2 >= RECORRIDO_TOTAL_2) {
        pos2 = RECORRIDO_TOTAL_2;
        motor2Moviendo = false;
        pasosRestantesMotor2 = 0;
        Serial.println();
        Serial.println(F("🔔 Motor 2: VÁLVULA COMPLETAMENTE ABIERTA (1300 pasos)"));
        Serial.print(F("📊 Pasos totales dados: "));
        Serial.println(totalPasosMotor2);
      }
      else if (pos2 <= 0) {
        pos2 = 0;
        motor2Moviendo = false;
        pasosRestantesMotor2 = 0;
        Serial.println();
        Serial.println(F("🔔 Motor 2: VÁLVULA COMPLETAMENTE CERRADA (0 pasos)"));
        Serial.print(F("📊 Pasos totales dados: "));
        Serial.println(totalPasosMotor2);
      }
      
      if (pasosRestantesMotor2 == 0 && motor2Moviendo) {
        motor2Moviendo = false;
        Serial.println();
        Serial.print(F("✅ Motor 2: Movimiento completado. Posición: "));
        Serial.print(pos2);
        Serial.print(F(" de "));
        Serial.println(RECORRIDO_TOTAL_2);
        Serial.print(F("📊 Pasos totales dados: "));
        Serial.println(totalPasosMotor2);
      }
    }
  }
  
  // ===== MOTOR 3 =====
  if (motor3Moviendo && habilitado3 && pasosRestantesMotor3 > 0) {
    unsigned long ahora = micros();
    if (ahora - tiempoUltimoPasoMotor3 >= delayMotor3) {
      digitalWrite(STEP3_PIN, HIGH);
      delayMicroseconds(2);
      digitalWrite(STEP3_PIN, LOW);
      
      if (digitalRead(DIR3_PIN) == HIGH) {
        pos3++;
        Serial.print(F("B"));
      } else {
        pos3--;
        Serial.print(F("D"));
      }
      
      pasosRestantesMotor3--;
      totalPasosMotor3++;
      tiempoUltimoPasoMotor3 = ahora;
      
      if (pos3 >= RECORRIDO_TOTAL_3) {
        pos3 = RECORRIDO_TOTAL_3;
        motor3Moviendo = false;
        pasosRestantesMotor3 = 0;
        Serial.println();
        Serial.println(F("🔔 Motor 3: VÁLVULA COMPLETAMENTE ABIERTA (1000 pasos)"));
        Serial.print(F("📊 Pasos totales dados: "));
        Serial.println(totalPasosMotor3);
      }
      else if (pos3 <= 0) {
        pos3 = 0;
        motor3Moviendo = false;
        pasosRestantesMotor3 = 0;
        Serial.println();
        Serial.println(F("🔔 Motor 3: VÁLVULA COMPLETAMENTE CERRADA (0 pasos)"));
        Serial.print(F("📊 Pasos totales dados: "));
        Serial.println(totalPasosMotor3);
      }
      
      if (pasosRestantesMotor3 == 0 && motor3Moviendo) {
        motor3Moviendo = false;
        Serial.println();
        Serial.print(F("✅ Motor 3: Movimiento completado. Posición: "));
        Serial.print(pos3);
        Serial.print(F(" de "));
        Serial.println(RECORRIDO_TOTAL_3);
        Serial.print(F("📊 Pasos totales dados: "));
        Serial.println(totalPasosMotor3);
      }
    }
  }
  
  if (diagnosticando && (millis() - tiempoDiagnostico >= 5000)) {
    tiempoDiagnostico = millis();
    mostrarDiagnostico();
  }
}

void procesarComando(String cmd) {
  String comando = cmd;
  int parametro = 0;
  
  // ===== COMANDOS DEL SERVO (antes de toUpperCase para conservar + y -) =====
  
  // ===== INICIALIZAR SERVO =====
  if (comando == "INIT" || comando == "init") {
    servo.write(0);
    anguloActual = 0.0;
    servoInicializado = true;
    Serial.println(F("✅ Servo inicializado en 0°"));
    return;
  }
  
  // ===== MOVIMIENTO A ÁNGULO ESPECÍFICO (S0.0, S2.5, S5.5, S8.3, S11.5) =====
  if (comando.length() > 1 && (comando.charAt(0) == 'S' || comando.charAt(0) == 's')) {
    String numStr = comando.substring(1);
    float angulo = numStr.toFloat();
    
    if (!servoInicializado) {
      Serial.println(F("❌ ERROR: Servo no inicializado. Usa INIT primero"));
      return;
    }
    
    if (angulo < ANGULO_MINIMO || angulo > ANGULO_MAXIMO) {
      Serial.print(F("❌ ERROR: Ángulo fuera de rango (0.0° a "));
      Serial.print(ANGULO_MAXIMO);
      Serial.println(F("°)"));
      return;
    }
    
    int microsegundos = map(angulo * 10, 0, 1800, 500, 2500);
    servo.writeMicroseconds(microsegundos);
    anguloActual = angulo;
    
    Serial.print(F("✅ Servo movido a "));
    Serial.print(angulo, 1);
    Serial.println(F("°"));
    return;
  }
  
  // ===== MOVIMIENTO INCREMENTAL (+ y -) =====
  if (comando == "+") {
    if (!servoInicializado) {
      Serial.println(F("❌ ERROR: Servo no inicializado. Usa INIT primero"));
      return;
    }
    float nuevoAngulo = anguloActual + 0.1;
    if (nuevoAngulo > ANGULO_MAXIMO) {
      Serial.print(F("⚠ Límite máximo alcanzado ("));
      Serial.print(ANGULO_MAXIMO);
      Serial.println(F("°)"));
      return;
    }
    int microsegundos = map(nuevoAngulo * 10, 0, 1800, 500, 2500);
    servo.writeMicroseconds(microsegundos);
    anguloActual = nuevoAngulo;
    Serial.print(F("✅ Servo: "));
    Serial.print(anguloActual, 1);
    Serial.println(F("° (+0.1°)"));
    return;
  }
  
  if (comando == "-") {
    if (!servoInicializado) {
      Serial.println(F("❌ ERROR: Servo no inicializado. Usa INIT primero"));
      return;
    }
    float nuevoAngulo = anguloActual - 0.1;
    if (nuevoAngulo < ANGULO_MINIMO) {
      Serial.println(F("⚠ Límite mínimo alcanzado (0.0°)"));
      return;
    }
    int microsegundos = map(nuevoAngulo * 10, 0, 1800, 500, 2500);
    servo.writeMicroseconds(microsegundos);
    anguloActual = nuevoAngulo;
    Serial.print(F("✅ Servo: "));
    Serial.print(anguloActual, 1);
    Serial.println(F("° (-0.1°)"));
    return;
  }
  
  if (comando == "++") {
    if (!servoInicializado) {
      Serial.println(F("❌ ERROR: Servo no inicializado. Usa INIT primero"));
      return;
    }
    float nuevoAngulo = anguloActual + 1.0;
    if (nuevoAngulo > ANGULO_MAXIMO) {
      Serial.print(F("⚠ Límite máximo alcanzado ("));
      Serial.print(ANGULO_MAXIMO);
      Serial.println(F("°)"));
      return;
    }
    int microsegundos = map(nuevoAngulo * 10, 0, 1800, 500, 2500);
    servo.writeMicroseconds(microsegundos);
    anguloActual = nuevoAngulo;
    Serial.print(F("✅ Servo: "));
    Serial.print(anguloActual, 1);
    Serial.println(F("° (+1.0°)"));
    return;
  }
  
  if (comando == "--") {
    if (!servoInicializado) {
      Serial.println(F("❌ ERROR: Servo no inicializado. Usa INIT primero"));
      return;
    }
    float nuevoAngulo = anguloActual - 1.0;
    if (nuevoAngulo < ANGULO_MINIMO) {
      Serial.println(F("⚠ Límite mínimo alcanzado (0.0°)"));
      return;
    }
    int microsegundos = map(nuevoAngulo * 10, 0, 1800, 500, 2500);
    servo.writeMicroseconds(microsegundos);
    anguloActual = nuevoAngulo;
    Serial.print(F("✅ Servo: "));
    Serial.print(anguloActual, 1);
    Serial.println(F("° (-1.0°)"));
    return;
  }
  
  // ===== COMANDOS DE MOTORES =====
  if (cmd.length() > 0) {
    char primerChar = cmd.charAt(0);
    char segundoChar = (cmd.length() > 1) ? cmd.charAt(1) : ' ';
    
    // ===== COMANDO PARA MOTOR 1 (1+ o 1-) =====
    if (primerChar == '1' && (segundoChar == '+' || segundoChar == '-')) {
      String numStr = cmd.substring(2);
      parametro = numStr.toInt();
      
      if (parametro <= 0) {
        Serial.println(F("❌ ERROR: Especifica número de pasos (ej: 1+500)"));
        return;
      }
      
      if (!habilitado1) {
        Serial.println(F("❌ ERROR: Motor 1 deshabilitado. Usa 1H"));
        return;
      }
      
      if (segundoChar == '+') {
        int nuevaPosicion = pos1 + parametro;
        
        if (nuevaPosicion > RECORRIDO_TOTAL_1) {
          Serial.print(F("❌ ERROR: Supera el límite de 1300 pasos ("));
          Serial.print(pos1);
          Serial.print(F(" + "));
          Serial.print(parametro);
          Serial.print(F(" = "));
          Serial.print(nuevaPosicion);
          Serial.println(F(" > 1300)"));
          return;
        }
        
        if (nuevaPosicion == RECORRIDO_TOTAL_1) {
          Serial.println(F("⚠ AVISO: Llegará al límite máximo (1300 pasos)"));
        }
        
        digitalWrite(DIR1_PIN, AVANCE_MOTOR_1 ? HIGH : LOW);
        pasosRestantesMotor1 = parametro;
        motor1Moviendo = true;
        Serial.print(F("▶ Motor 1: Girando DERECHA (ABRIENDO) "));
        Serial.print(parametro);
        Serial.print(F(" pasos ("));
        Serial.print(pos1);
        Serial.print(F(" → "));
        Serial.print(nuevaPosicion);
        Serial.println(F(")"));
        Serial.print(F("  "));
      }
      else if (segundoChar == '-') {
        int nuevaPosicion = pos1 - parametro;
        
        if (nuevaPosicion < 0) {
          Serial.print(F("❌ ERROR: Supera el límite de 0 pasos ("));
          Serial.print(pos1);
          Serial.print(F(" - "));
          Serial.print(parametro);
          Serial.print(F(" = "));
          Serial.print(nuevaPosicion);
          Serial.println(F(" < 0)"));
          return;
        }
        
        if (nuevaPosicion == 0) {
          Serial.println(F("⚠ AVISO: Llegará al límite mínimo (0 pasos)"));
        }
        
        digitalWrite(DIR1_PIN, AVANCE_MOTOR_1 ? LOW : HIGH);
        pasosRestantesMotor1 = parametro;
        motor1Moviendo = true;
        Serial.print(F("◀ Motor 1: Girando IZQUIERDA (CERRANDO) "));
        Serial.print(parametro);
        Serial.print(F(" pasos ("));
        Serial.print(pos1);
        Serial.print(F(" → "));
        Serial.print(nuevaPosicion);
        Serial.println(F(")"));
        Serial.print(F("  "));
      }
      return;
    }
    
    // ===== COMANDO PARA MOTOR 2 (2+ o 2-) - AHORA IGUAL AL MOTOR 1 =====
    else if (primerChar == '2' && (segundoChar == '+' || segundoChar == '-')) {
      String numStr = cmd.substring(2);
      parametro = numStr.toInt();
      
      if (parametro <= 0) {
        Serial.println(F("❌ ERROR: Especifica número de pasos (ej: 2+500)"));
        return;
      }
      
      if (!habilitado2) {
        Serial.println(F("❌ ERROR: Motor 2 deshabilitado. Usa 2H"));
        return;
      }
      
      if (segundoChar == '+') {
        int nuevaPosicion = pos2 + parametro;
        
        if (nuevaPosicion > RECORRIDO_TOTAL_2) {
          Serial.print(F("❌ ERROR: Supera el límite de 1300 pasos ("));
          Serial.print(pos2);
          Serial.print(F(" + "));
          Serial.print(parametro);
          Serial.print(F(" = "));
          Serial.print(nuevaPosicion);
          Serial.println(F(" > 1300)"));
          return;
        }
        
        if (nuevaPosicion == RECORRIDO_TOTAL_2) {
          Serial.println(F("⚠ AVISO: Llegará al límite máximo (1300 pasos)"));
        }
        
        digitalWrite(DIR2_PIN, AVANCE_MOTOR_2 ? HIGH : LOW);
        pasosRestantesMotor2 = parametro;
        motor2Moviendo = true;
        Serial.print(F("▶ Motor 2: Girando DERECHA (ABRIENDO) "));
        Serial.print(parametro);
        Serial.print(F(" pasos ("));
        Serial.print(pos2);
        Serial.print(F(" → "));
        Serial.print(nuevaPosicion);
        Serial.println(F(")"));
        Serial.print(F("  "));
      }
      else if (segundoChar == '-') {
        int nuevaPosicion = pos2 - parametro;
        
        if (nuevaPosicion < 0) {
          Serial.print(F("❌ ERROR: Supera el límite de 0 pasos ("));
          Serial.print(pos2);
          Serial.print(F(" - "));
          Serial.print(parametro);
          Serial.print(F(" = "));
          Serial.print(nuevaPosicion);
          Serial.println(F(" < 0)"));
          return;
        }
        
        if (nuevaPosicion == 0) {
          Serial.println(F("⚠ AVISO: Llegará al límite mínimo (0 pasos)"));
        }
        
        digitalWrite(DIR2_PIN, AVANCE_MOTOR_2 ? LOW : HIGH);
        pasosRestantesMotor2 = parametro;
        motor2Moviendo = true;
        Serial.print(F("◀ Motor 2: Girando IZQUIERDA (CERRANDO) "));
        Serial.print(parametro);
        Serial.print(F(" pasos ("));
        Serial.print(pos2);
        Serial.print(F(" → "));
        Serial.print(nuevaPosicion);
        Serial.println(F(")"));
        Serial.print(F("  "));
      }
      return;
    }
    
    // ===== COMANDO PARA MOTOR 3 (3A o 3C) =====
    else if (primerChar == '3' && (segundoChar == 'A' || segundoChar == 'a' || segundoChar == 'C' || segundoChar == 'c')) {
      
      if (!habilitado3) {
        Serial.println(F("❌ ERROR: Motor 3 deshabilitado. Usa 3H"));
        return;
      }
      
      // Extraer el parámetro de velocidad (después de 3A o 3C)
      String resto = cmd.substring(2);
      resto.trim();
      int velocidad = resto.toInt();
      
      // Si no se especifica velocidad, usar la actual
      if (velocidad <= 0) {
        velocidad = delayMotor3;
        Serial.print(F("ℹ Usando velocidad por defecto M3: "));
        Serial.println(velocidad);
      } else {
        // Validar rango de velocidad (mínimo 200 µs, máximo 5000 µs)
        if (velocidad < 200) {
          Serial.println(F("⚠ Velocidad muy alta. Ajustando a 200 µs (máxima)"));
          velocidad = 200;
        } else if (velocidad > 5000) {
          Serial.println(F("⚠ Velocidad muy baja. Ajustando a 5000 µs (mínima)"));
          velocidad = 5000;
        }
      }
      
      // 3A = ABRIR (sentido A)
      if (segundoChar == 'A' || segundoChar == 'a') {
        int pasosNecesarios = RECORRIDO_TOTAL_3 - pos3;
        
        if (pasosNecesarios <= 0) {
          Serial.println(F("🔔 Motor 3: YA ESTÁ COMPLETAMENTE ABIERTO"));
          return;
        }
        
        delayMotor3 = velocidad;
        
        digitalWrite(DIR3_PIN, AVANCE_MOTOR_3 ? HIGH : LOW);
        pasosRestantesMotor3 = pasosNecesarios;
        motor3Moviendo = true;
        Serial.print(F("▶ Motor 3: Girando sentido A (ABRIENDO) "));
        Serial.print(pasosNecesarios);
        Serial.print(F(" pasos ("));
        Serial.print(pos3);
        Serial.print(F(" → "));
        Serial.print(RECORRIDO_TOTAL_3);
        Serial.print(F(") a velocidad "));
        Serial.print(delayMotor3);
        Serial.println(F(" µs"));
        Serial.print(F("  "));
      }
      // 3C = CERRAR (sentido C)
      else if (segundoChar == 'C' || segundoChar == 'c') {
        int pasosNecesarios = pos3;
        
        if (pasosNecesarios <= 0) {
          Serial.println(F("🔔 Motor 3: YA ESTÁ COMPLETAMENTE CERRADO"));
          return;
        }
        
        delayMotor3 = velocidad;
        
        digitalWrite(DIR3_PIN, AVANCE_MOTOR_3 ? LOW : HIGH);
        pasosRestantesMotor3 = pasosNecesarios;
        motor3Moviendo = true;
        Serial.print(F("◀ Motor 3: Girando sentido C (CERRANDO) "));
        Serial.print(pasosNecesarios);
        Serial.print(F(" pasos ("));
        Serial.print(pos3);
        Serial.print(F(" → "));
        Serial.print(F("0"));
        Serial.print(F(") a velocidad "));
        Serial.print(delayMotor3);
        Serial.println(F(" µs"));
        Serial.print(F("  "));
      }
      return;
    }
    
    // ===== COMANDO PARA TODOS LOS MOTORES (+ o -) =====
    else if (primerChar == '+' || primerChar == '-') {
      String numStr = cmd.substring(1);
      parametro = numStr.toInt();
      
      if (parametro <= 0) {
        Serial.println(F("❌ ERROR: Especifica número de pasos (ej: +300)"));
        return;
      }
      
      if (primerChar == '+') {
        int nuevaPosicion1 = pos1 + parametro;
        int nuevaPosicion2 = pos2 + parametro;
        bool error = false;
        
        if (nuevaPosicion1 > RECORRIDO_TOTAL_1) {
          Serial.print(F("❌ ERROR Motor 1: Supera el límite de 1300 pasos ("));
          Serial.print(pos1);
          Serial.print(F(" + "));
          Serial.print(parametro);
          Serial.print(F(" = "));
          Serial.print(nuevaPosicion1);
          Serial.println(F(" > 1300)"));
          error = true;
        }
        
        if (nuevaPosicion2 > RECORRIDO_TOTAL_2) {
          Serial.print(F("❌ ERROR Motor 2: Supera el límite de 1300 pasos ("));
          Serial.print(pos2);
          Serial.print(F(" + "));
          Serial.print(parametro);
          Serial.print(F(" = "));
          Serial.print(nuevaPosicion2);
          Serial.println(F(" > 1300)"));
          error = true;
        }
        
        int pasosNecesarios3 = RECORRIDO_TOTAL_3 - pos3;
        if (pasosNecesarios3 <= 0) {
          Serial.println(F("🔔 Motor 3: YA ESTÁ COMPLETAMENTE ABIERTO"));
          error = true;
        }
        
        if (error) {
          Serial.println(F("❌ Comando cancelado"));
          return;
        }
        
        digitalWrite(DIR1_PIN, AVANCE_MOTOR_1 ? HIGH : LOW);
        pasosRestantesMotor1 = parametro;
        motor1Moviendo = true;
        
        digitalWrite(DIR2_PIN, AVANCE_MOTOR_2 ? HIGH : LOW);
        pasosRestantesMotor2 = parametro;
        motor2Moviendo = true;
        
        digitalWrite(DIR3_PIN, AVANCE_MOTOR_3 ? HIGH : LOW);
        pasosRestantesMotor3 = pasosNecesarios3;
        motor3Moviendo = true;
        
        Serial.print(F("▶▶▶ Todos: ABRIENDO (M1: "));
        Serial.print(parametro);
        Serial.print(F(" pasos, M2: "));
        Serial.print(parametro);
        Serial.print(F(" pasos, M3: "));
        Serial.print(pasosNecesarios3);
        Serial.println(F(" pasos)"));
      }
      else if (primerChar == '-') {
        int nuevaPosicion1 = pos1 - parametro;
        int nuevaPosicion2 = pos2 - parametro;
        bool error = false;
        
        if (nuevaPosicion1 < 0) {
          Serial.print(F("❌ ERROR Motor 1: Supera el límite de 0 pasos ("));
          Serial.print(pos1);
          Serial.print(F(" - "));
          Serial.print(parametro);
          Serial.print(F(" = "));
          Serial.print(nuevaPosicion1);
          Serial.println(F(" < 0)"));
          error = true;
        }
        
        if (nuevaPosicion2 < 0) {
          Serial.print(F("❌ ERROR Motor 2: Supera el límite de 0 pasos ("));
          Serial.print(pos2);
          Serial.print(F(" - "));
          Serial.print(parametro);
          Serial.print(F(" = "));
          Serial.print(nuevaPosicion2);
          Serial.println(F(" < 0)"));
          error = true;
        }
        
        int pasosNecesarios3 = pos3;
        if (pasosNecesarios3 <= 0) {
          Serial.println(F("🔔 Motor 3: YA ESTÁ COMPLETAMENTE CERRADO"));
          error = true;
        }
        
        if (error) {
          Serial.println(F("❌ Comando cancelado"));
          return;
        }
        
        digitalWrite(DIR1_PIN, AVANCE_MOTOR_1 ? LOW : HIGH);
        pasosRestantesMotor1 = parametro;
        motor1Moviendo = true;
        
        digitalWrite(DIR2_PIN, AVANCE_MOTOR_2 ? LOW : HIGH);
        pasosRestantesMotor2 = parametro;
        motor2Moviendo = true;
        
        digitalWrite(DIR3_PIN, AVANCE_MOTOR_3 ? LOW : HIGH);
        pasosRestantesMotor3 = pasosNecesarios3;
        motor3Moviendo = true;
        
        Serial.print(F("◀◀◀ Todos: CERRANDO (M1: "));
        Serial.print(parametro);
        Serial.print(F(" pasos, M2: "));
        Serial.print(parametro);
        Serial.print(F(" pasos, M3: "));
        Serial.print(pasosNecesarios3);
        Serial.println(F(" pasos)"));
      }
      return;
    }
  }
  
  // ===== OTROS COMANDOS =====
  comando.toUpperCase();
  
  // MOTOR 1
  if (comando == "1S") {
    motor1Moviendo = false;
    pasosRestantesMotor1 = 0;
    Serial.println(F("■ Motor 1: DETENIDO"));
    Serial.print(F("📊 Posición: "));
    Serial.print(pos1);
    Serial.print(F(" de "));
    Serial.println(RECORRIDO_TOTAL_1);
    Serial.print(F("📊 Pasos totales dados: "));
    Serial.println(totalPasosMotor1);
  }
  else if (comando == "1P") {
    Serial.print(F("📊 Motor 1 - Posición: "));
    Serial.print(pos1);
    Serial.print(F(" de "));
    Serial.println(RECORRIDO_TOTAL_1);
    int porcentaje = map(pos1, 0, RECORRIDO_TOTAL_1, 0, 100);
    Serial.print(F("  Apertura: "));
    Serial.print(porcentaje);
    Serial.println(F("%"));
    Serial.print(F("📊 Pasos totales dados: "));
    Serial.println(totalPasosMotor1);
  }
  else if (comando == "1H") {
    digitalWrite(EN1_PIN, LOW);
    habilitado1 = true;
    Serial.println(F("✅ Motor 1 HABILITADO"));
  }
  else if (comando == "1D") {
    digitalWrite(EN1_PIN, HIGH);
    habilitado1 = false;
    motor1Moviendo = false;
    pasosRestantesMotor1 = 0;
    Serial.println(F("⛔ Motor 1 DESHABILITADO"));
  }
  
  // MOTOR 2
  else if (comando == "2S") {
    motor2Moviendo = false;
    pasosRestantesMotor2 = 0;
    Serial.println(F("■ Motor 2: DETENIDO"));
    Serial.print(F("📊 Posición: "));
    Serial.print(pos2);
    Serial.print(F(" de "));
    Serial.println(RECORRIDO_TOTAL_2);
    Serial.print(F("📊 Pasos totales dados: "));
    Serial.println(totalPasosMotor2);
  }
  else if (comando == "2P") {
    Serial.print(F("📊 Motor 2 - Posición: "));
    Serial.print(pos2);
    Serial.print(F(" de "));
    Serial.println(RECORRIDO_TOTAL_2);
    int porcentaje = map(pos2, 0, RECORRIDO_TOTAL_2, 0, 100);
    Serial.print(F("  Apertura: "));
    Serial.print(porcentaje);
    Serial.println(F("%"));
    Serial.print(F("📊 Pasos totales dados: "));
    Serial.println(totalPasosMotor2);
  }
  else if (comando == "2H") {
    digitalWrite(EN2_PIN, LOW);
    habilitado2 = true;
    Serial.println(F("✅ Motor 2 HABILITADO"));
  }
  else if (comando == "2D") {
    digitalWrite(EN2_PIN, HIGH);
    habilitado2 = false;
    motor2Moviendo = false;
    pasosRestantesMotor2 = 0;
    Serial.println(F("⛔ Motor 2 DESHABILITADO"));
  }
  
  // MOTOR 3
  else if (comando == "3S") {
    motor3Moviendo = false;
    pasosRestantesMotor3 = 0;
    Serial.println(F("■ Motor 3: DETENIDO"));
    Serial.print(F("📊 Posición: "));
    Serial.print(pos3);
    Serial.print(F(" de "));
    Serial.println(RECORRIDO_TOTAL_3);
    Serial.print(F("📊 Pasos totales dados: "));
    Serial.println(totalPasosMotor3);
  }
  else if (comando == "3P") {
    Serial.print(F("📊 Motor 3 - Posición: "));
    Serial.print(pos3);
    Serial.print(F(" de "));
    Serial.println(RECORRIDO_TOTAL_3);
    int porcentaje = map(pos3, 0, RECORRIDO_TOTAL_3, 0, 100);
    Serial.print(F("  Apertura: "));
    Serial.print(porcentaje);
    Serial.println(F("%"));
    Serial.print(F("📊 Pasos totales dados: "));
    Serial.println(totalPasosMotor3);
  }
  else if (comando == "3H") {
    digitalWrite(EN3_PIN, LOW);
    habilitado3 = true;
    Serial.println(F("✅ Motor 3 HABILITADO"));
  }
  else if (comando == "3D") {
    digitalWrite(EN3_PIN, HIGH);
    habilitado3 = false;
    motor3Moviendo = false;
    pasosRestantesMotor3 = 0;
    Serial.println(F("⛔ Motor 3 DESHABILITADO"));
  }
  
  // VELOCIDAD POR DEFECTO MOTOR 3
  else if (comando == "V3+") {
    if (delayMotor3 > 300) {
      delayMotor3 -= 200;
      Serial.print(F("⚡ Velocidad M3 aumentada. Delay: "));
      Serial.print(delayMotor3);
      Serial.println(F(" µs"));
    } else {
      Serial.println(F("⚠ Velocidad máxima M3 (200 µs)"));
    }
  }
  else if (comando == "V3-") {
    if (delayMotor3 < 5000) {
      delayMotor3 += 200;
      Serial.print(F("🐢 Velocidad M3 disminuida. Delay: "));
      Serial.print(delayMotor3);
      Serial.println(F(" µs"));
    } else {
      Serial.println(F("⚠ Velocidad mínima M3 (5000 µs)"));
    }
  }
  
  // TODOS DETENER
  else if (comando == "SS") {
    motor1Moviendo = false;
    motor2Moviendo = false;
    motor3Moviendo = false;
    pasosRestantesMotor1 = 0;
    pasosRestantesMotor2 = 0;
    pasosRestantesMotor3 = 0;
    Serial.println(F("■■■ Todos los motores: DETENIDOS"));
    Serial.print(F("📊 Posiciones - M1: "));
    Serial.print(pos1);
    Serial.print(F(" de "));
    Serial.print(RECORRIDO_TOTAL_1);
    Serial.print(F(" | M2: "));
    Serial.print(pos2);
    Serial.print(F(" de "));
    Serial.print(RECORRIDO_TOTAL_2);
    Serial.print(F(" | M3: "));
    Serial.print(pos3);
    Serial.print(F(" de "));
    Serial.println(RECORRIDO_TOTAL_3);
  }
  
  // DIAGNÓSTICO
  else if (comando == "DIAG") {
    diagnosticando = !diagnosticando;
    if (diagnosticando) {
      Serial.println(F("🔍 DIAGNÓSTICO ACTIVADO (cada 5 segundos)"));
      tiempoDiagnostico = millis();
    } else {
      Serial.println(F("🔍 DIAGNÓSTICO DESACTIVADO"));
    }
  }
  else if (comando == "ESTADO") {
    mostrarDiagnostico();
  }
  else if (cmd != "") {
    ethernetSerial.println("sdadsadas");
    Serial.print(F("❌ Comando no válido: "));
    Serial.println(cmd);
    Serial.println(F("Ejemplos:"));
    Serial.println(F("  1+500   - Motor1 abre 500 pasos"));
    Serial.println(F("  1-300   - Motor1 cierra 300 pasos"));
    Serial.println(F("  2+500   - Motor2 abre 500 pasos"));
    Serial.println(F("  2-300   - Motor2 cierra 300 pasos"));
    Serial.println(F("  3A 500  - Motor3 abre 1000 pasos a 500 µs"));
    Serial.println(F("  3C 3000 - Motor3 cierra 1000 pasos a 3000 µs"));
    Serial.println(F("  3A      - Motor3 abre con velocidad por defecto"));
    Serial.println(F("  INIT    - Inicializar servo en 0°"));
    Serial.println(F("  S0.0    - Mover servo a 0.0°"));
    Serial.println(F("  S5.5    - Mover servo a 5.5°"));
    Serial.println(F("  S11.5   - Mover servo a 11.5° (máximo)"));
    Serial.println(F("  +       - Avanzar 0.1°"));
    Serial.println(F("  -       - Retroceder 0.1°"));
    Serial.println(F("  ++      - Avanzar 1.0°"));
    Serial.println(F("  --      - Retroceder 1.0°"));
    Serial.println(F("  +300    - M1 abre 300 / M2 abre 300 / M3 abre 1000"));
    Serial.println(F("  -200    - M1 cierra 200 / M2 cierra 200 / M3 cierra 1000"));
    Serial.println(F("  SS      - Detener todos"));
    Serial.println(F("  ESTADO  - Ver estado completo"));
  }
}

void mostrarDiagnostico() {
  Serial.println(F("===== 📊 ESTADO COMPLETO ====="));
  
  int porcentaje1 = map(pos1, 0, RECORRIDO_TOTAL_1, 0, 100);
  int porcentaje2 = map(pos2, 0, RECORRIDO_TOTAL_2, 0, 100);
  int porcentaje3 = map(pos3, 0, RECORRIDO_TOTAL_3, 0, 100);
  
  Serial.print(F("Motor 1 (Válvula 1): "));
  Serial.print(habilitado1 ? "✅ HABILITADO" : "⛔ DESHABILITADO");
  Serial.print(F(" | Moviendo: "));
  Serial.print(motor1Moviendo ? "SÍ" : "NO");
  Serial.print(F(" | Posición: "));
  Serial.print(pos1);
  Serial.print(F("/"));
  Serial.print(RECORRIDO_TOTAL_1);
  Serial.print(F(" ("));
  Serial.print(porcentaje1);
  Serial.print(F("%)"));
  Serial.print(F(" | Pasos totales: "));
  Serial.println(totalPasosMotor1);
  
  Serial.print(F("Motor 2 (Válvula 2): "));
  Serial.print(habilitado2 ? "✅ HABILITADO" : "⛔ DESHABILITADO");
  Serial.print(F(" | Moviendo: "));
  Serial.print(motor2Moviendo ? "SÍ" : "NO");
  Serial.print(F(" | Posición: "));
  Serial.print(pos2);
  Serial.print(F("/"));
  Serial.print(RECORRIDO_TOTAL_2);
  Serial.print(F(" ("));
  Serial.print(porcentaje2);
  Serial.print(F("%)"));
  Serial.print(F(" | Pasos totales: "));
  Serial.println(totalPasosMotor2);
  
  Serial.print(F("Motor 3 (Válvula 3): "));
  Serial.print(habilitado3 ? "✅ HABILITADO" : "⛔ DESHABILITADO");
  Serial.print(F(" | Moviendo: "));
  Serial.print(motor3Moviendo ? "SÍ" : "NO");
  Serial.print(F(" | Posición: "));
  Serial.print(pos3);
  Serial.print(F("/"));
  Serial.print(RECORRIDO_TOTAL_3);
  Serial.print(F(" ("));
  Serial.print(porcentaje3);
  Serial.print(F("%)"));
  Serial.print(F(" | Pasos totales: "));
  Serial.print(totalPasosMotor3);
  Serial.print(F(" | Delay: "));
  Serial.println(delayMotor3);
  
  Serial.print(F("Servo: "));
  if (servoInicializado) {
    Serial.print(F("INICIALIZADO | Ángulo: "));
    Serial.print(anguloActual, 1);
    Serial.println(F("°"));
  } else {
    Serial.println(F("❌ NO INICIALIZADO (usa INIT)"));
  }
  
  Serial.println(F("=============================="));
}