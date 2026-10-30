
// CONTROL DE TRES MOTORES NEMA CON A4988 + SERVO 40KG
// Arduino Uno clasico (ATmega328P)
// Ethernet SoftwareSerial: 115200 baudios
// Servo con attach/detach automatico

#include <SoftwareSerial.h>
#include <Servo.h>

// ===== DEFINICION DE PINES =====

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

// SERVO
#define SERVO_PIN  6

// ETHERNET
#define ETH_RX_PIN A4
#define ETH_TX_PIN A5

SoftwareSerial ethernetSerial(ETH_RX_PIN, ETH_TX_PIN);

// ===== DIRECCIONES DE AVANCE =====

const bool AVANCE_MOTOR_1 = true;
const bool AVANCE_MOTOR_2 = true;
const bool AVANCE_MOTOR_3 = true;

// ===== MOTOR 1 =====

const unsigned int DELAY_MOTOR_1 = 2000;
const int RECORRIDO_TOTAL_1 = 1300;

int pos1 = 0;
bool habilitado1 = true;
bool motor1Moviendo = false;
int pasosRestantesMotor1 = 0;
unsigned long tiempoUltimoPasoMotor1 = 0;
unsigned long totalPasosMotor1 = 0;

// ===== MOTOR 2 =====

const unsigned int DELAY_MOTOR_2 = 2000;
const int RECORRIDO_TOTAL_2 = 1300;

int pos2 = 0;
bool habilitado2 = true;
bool motor2Moviendo = false;
int pasosRestantesMotor2 = 0;
unsigned long tiempoUltimoPasoMotor2 = 0;
unsigned long totalPasosMotor2 = 0;

// ===== MOTOR 3 =====

unsigned int delayMotor3 = 1500;
const int RECORRIDO_TOTAL_3 = 1000;

int pos3 = 0;
bool habilitado3 = true;
bool motor3Moviendo = false;
int pasosRestantesMotor3 = 0;
unsigned long tiempoUltimoPasoMotor3 = 0;
unsigned long totalPasosMotor3 = 0;

// ===== SERVO =====

Servo servo;

float anguloActual = 0.0;

const float ANGULO_MAXIMO = 11.5;
const float ANGULO_MINIMO = 0.0;

bool servoInicializado = false;

unsigned long tiempoUltimoComandoServo = 0;
const unsigned long TIEMPO_SERVO_ACTIVO = 700;

// ===== DIAGNOSTICO =====

bool diagnosticando = false;
unsigned long tiempoDiagnostico = 0;

// ===== ETHERNET =====

char comandoBufferEthernet[64];
uint8_t indiceComandoEthernet = 0;

// ============================================================
// CONTROL DEL SERVO
// ============================================================

void activarServo() {
  if (!servo.attached()) {
    servo.attach(SERVO_PIN);
  }
}

void moverServoMicrosegundos(int microsegundos) {
  activarServo();

  servo.writeMicroseconds(microsegundos);

  tiempoUltimoComandoServo = millis();
}

void controlarServo() {
  if (servo.attached() &&
      millis() - tiempoUltimoComandoServo >= TIEMPO_SERVO_ACTIVO) {

    servo.detach();
  }
}

// ============================================================
// ENVIO POR ETHERNET
// ============================================================

void enviarPorEthernet(String mensaje) {
  ethernetSerial.println(mensaje);
  Serial.println(mensaje);
}

void enviarRespuestaEthernet(String comando, String respuesta) {
  String mensaje = "CMD:" + comando + "|RES:" + respuesta;
  enviarPorEthernet(mensaje);
}

void imprimir(String mensaje) {
  Serial.print(mensaje);
  ethernetSerial.print(mensaje);
}

void imprimirLinea(String mensaje) {
  Serial.println(mensaje);
  ethernetSerial.println(mensaje);
}

// ============================================================
// PROCESAMIENTO DE COMANDOS ETHERNET
// ============================================================

void procesarComando(String cmd);

void procesarComandoEthernet(String cmd) {
  cmd.trim();

  if (cmd.length() == 0) return;

  Serial.print(F("Comando recibido por ETHERNET: "));
  Serial.println(cmd);

  procesarComando(cmd);

  enviarRespuestaEthernet(cmd, "OK");
}

// ============================================================
// SETUP
// ============================================================

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

  // El servo comienza desconectado.
  // Se conectara cuando reciba INIT o una orden de movimiento.

  digitalWrite(EN1_PIN, LOW);
  digitalWrite(EN2_PIN, LOW);
  digitalWrite(EN3_PIN, LOW);

  Serial.begin(115200);
  delay(1000);

  // Ethernet a 115200 baudios
  ethernetSerial.begin(115200);

  Serial.println(F("=== SISTEMA UNIFICADO ==="));
  Serial.println(F("Tres motores + servo"));
  Serial.println(F("Ethernet: 115200 baudios"));
  Serial.println(F("Servo: attach/detach automatico"));
  Serial.println(F("================================"));

  enviarPorEthernet(
    "SYS:STARTED|INFO:Sistema de motores y servo listo"
  );

  enviarPorEthernet(
    "INFO:Use los mismos comandos del monitor serie"
  );
}

// ============================================================
// LOOP
// ============================================================

void loop() {

  // ===== RECEPCION ETHERNET =====

  while (ethernetSerial.available() > 0) {

    char c = ethernetSerial.read();

    if (c == '\n' || c == '\r') {

      if (indiceComandoEthernet > 0) {

        comandoBufferEthernet[indiceComandoEthernet] = '\0';

        procesarComandoEthernet(
          String(comandoBufferEthernet)
        );

        indiceComandoEthernet = 0;
      }

    } else if (c != '\0') {

      if (indiceComandoEthernet <
          sizeof(comandoBufferEthernet) - 1) {

        comandoBufferEthernet[indiceComandoEthernet++] = c;

      } else {

        // Descartar comando demasiado largo
        indiceComandoEthernet = 0;
      }
    }
  }

  // ===== RECEPCION USB =====

  if (Serial.available() > 0) {

    String comando = Serial.readStringUntil('\n');
    comando.trim();

    procesarComando(comando);
  }

  // ===== MOTOR 1 =====

  if (motor1Moviendo && habilitado1 &&
      pasosRestantesMotor1 > 0) {

    unsigned long ahora = micros();

    if (ahora - tiempoUltimoPasoMotor1 >= DELAY_MOTOR_1) {

      digitalWrite(STEP1_PIN, HIGH);
      delayMicroseconds(2);
      digitalWrite(STEP1_PIN, LOW);

      if (digitalRead(DIR1_PIN) == HIGH) {
        pos1++;
      } else {
        pos1--;
      }

      pasosRestantesMotor1--;
      totalPasosMotor1++;

      tiempoUltimoPasoMotor1 = ahora;

      if (pos1 >= RECORRIDO_TOTAL_1) {

        pos1 = RECORRIDO_TOTAL_1;
        motor1Moviendo = false;
        pasosRestantesMotor1 = 0;

        Serial.println();
        Serial.println(F("Motor 1: VALVULA COMPLETAMENTE ABIERTA"));
        Serial.print(F("Pasos totales: "));
        Serial.println(totalPasosMotor1);

      } else if (pos1 <= 0) {

        pos1 = 0;
        motor1Moviendo = false;
        pasosRestantesMotor1 = 0;

        Serial.println();
        Serial.println(F("Motor 1: VALVULA COMPLETAMENTE CERRADA"));
        Serial.print(F("Pasos totales: "));
        Serial.println(totalPasosMotor1);
      }

      if (pasosRestantesMotor1 == 0 && motor1Moviendo) {

        motor1Moviendo = false;

        Serial.println();
        Serial.print(F("Motor 1: Movimiento completado. Posicion: "));
        Serial.print(pos1);
        Serial.print(F(" de "));
        Serial.println(RECORRIDO_TOTAL_1);

        Serial.print(F("Pasos totales: "));
        Serial.println(totalPasosMotor1);
      }
    }
  }

  // ===== MOTOR 2 =====

  if (motor2Moviendo && habilitado2 &&
      pasosRestantesMotor2 > 0) {

    unsigned long ahora = micros();

    if (ahora - tiempoUltimoPasoMotor2 >= DELAY_MOTOR_2) {

      digitalWrite(STEP2_PIN, HIGH);
      delayMicroseconds(2);
      digitalWrite(STEP2_PIN, LOW);

      if (digitalRead(DIR2_PIN) == HIGH) {
        pos2++;
      } else {
        pos2--;
      }

      pasosRestantesMotor2--;
      totalPasosMotor2++;

      tiempoUltimoPasoMotor2 = ahora;

      if (pos2 >= RECORRIDO_TOTAL_2) {

        pos2 = RECORRIDO_TOTAL_2;
        motor2Moviendo = false;
        pasosRestantesMotor2 = 0;

        Serial.println();
        Serial.println(F("Motor 2: VALVULA COMPLETAMENTE ABIERTA"));
        Serial.print(F("Pasos totales: "));
        Serial.println(totalPasosMotor2);

      } else if (pos2 <= 0) {

        pos2 = 0;
        motor2Moviendo = false;
        pasosRestantesMotor2 = 0;

        Serial.println();
        Serial.println(F("Motor 2: VALVULA COMPLETAMENTE CERRADA"));
        Serial.print(F("Pasos totales: "));
        Serial.println(totalPasosMotor2);
      }

      if (pasosRestantesMotor2 == 0 && motor2Moviendo) {

        motor2Moviendo = false;

        Serial.println();
        Serial.print(F("Motor 2: Movimiento completado. Posicion: "));
        Serial.print(pos2);
        Serial.print(F(" de "));
        Serial.println(RECORRIDO_TOTAL_2);

        Serial.print(F("Pasos totales: "));
        Serial.println(totalPasosMotor2);
      }
    }
  }

  // ===== MOTOR 3 =====

  if (motor3Moviendo && habilitado3 &&
      pasosRestantesMotor3 > 0) {

    unsigned long ahora = micros();

    if (ahora - tiempoUltimoPasoMotor3 >= delayMotor3) {

      digitalWrite(STEP3_PIN, HIGH);
      delayMicroseconds(2);
      digitalWrite(STEP3_PIN, LOW);

      if (digitalRead(DIR3_PIN) == HIGH) {
        pos3++;
      } else {
        pos3--;
      }

      pasosRestantesMotor3--;
      totalPasosMotor3++;

      tiempoUltimoPasoMotor3 = ahora;

      if (pos3 >= RECORRIDO_TOTAL_3) {

        pos3 = RECORRIDO_TOTAL_3;
        motor3Moviendo = false;
        pasosRestantesMotor3 = 0;

        Serial.println();
        Serial.println(F("Motor 3: VALVULA COMPLETAMENTE ABIERTA"));
        Serial.print(F("Pasos totales: "));
        Serial.println(totalPasosMotor3);

      } else if (pos3 <= 0) {

        pos3 = 0;
        motor3Moviendo = false;
        pasosRestantesMotor3 = 0;

        Serial.println();
        Serial.println(F("Motor 3: VALVULA COMPLETAMENTE CERRADA"));
        Serial.print(F("Pasos totales: "));
        Serial.println(totalPasosMotor3);
      }

      if (pasosRestantesMotor3 == 0 && motor3Moviendo) {

        motor3Moviendo = false;

        Serial.println();
        Serial.print(F("Motor 3: Movimiento completado. Posicion: "));
        Serial.print(pos3);
        Serial.print(F(" de "));
        Serial.println(RECORRIDO_TOTAL_3);

        Serial.print(F("Pasos totales: "));
        Serial.println(totalPasosMotor3);
      }
    }
  }

  // ===== DIAGNOSTICO =====

  if (diagnosticando &&
      millis() - tiempoDiagnostico >= 5000) {

    tiempoDiagnostico = millis();
    mostrarDiagnostico();
  }

  // ===== CONTROL AUTOMATICO DEL SERVO =====

  controlarServo();
}

// ============================================================
// PROCESAMIENTO GENERAL DE COMANDOS
// ============================================================

void procesarComando(String cmd) {

  String comando = cmd;
  int parametro = 0;

  // ==========================================================
  // COMANDOS DEL SERVO
  // ==========================================================

  // INIT

  if (comando == "INIT" || comando == "init") {

    activarServo();

    servo.write(0);

    tiempoUltimoComandoServo = millis();

    anguloActual = 0.0;
    servoInicializado = true;

    Serial.println(F("Servo inicializado en 0 grados"));

    return;
  }

  // ANGULO ESPECIFICO: S0.0, S2.5, S5.5, S8.3, S11.5

  if (comando.length() > 1 &&
      (comando.charAt(0) == 'S' ||
       comando.charAt(0) == 's')) {

    String numStr = comando.substring(1);
    float angulo = numStr.toFloat();

    if (!servoInicializado) {
      Serial.println(F("ERROR: Servo no inicializado. Usa INIT primero"));
      return;
    }

    if (angulo < ANGULO_MINIMO ||
        angulo > ANGULO_MAXIMO) {

      Serial.print(F("ERROR: Angulo fuera de rango (0.0 a "));
      Serial.print(ANGULO_MAXIMO);
      Serial.println(F(" grados)"));

      return;
    }

    int microsegundos = map(
      angulo * 10,
      0,
      1800,
      500,
      2500
    );

    moverServoMicrosegundos(microsegundos);

    anguloActual = angulo;

    Serial.print(F("Servo movido a "));
    Serial.print(angulo, 1);
    Serial.println(F(" grados"));

    return;
  }

  // MOVIMIENTO INCREMENTAL +

  if (comando == "+") {

    if (!servoInicializado) {
      Serial.println(F("ERROR: Servo no inicializado. Usa INIT primero"));
      return;
    }

    float nuevoAngulo = anguloActual + 0.1;

    if (nuevoAngulo > ANGULO_MAXIMO) {
      Serial.println(F("Limite maximo alcanzado"));
      return;
    }

    int microsegundos = map(
      nuevoAngulo * 10,
      0,
      1800,
      500,
      2500
    );

    moverServoMicrosegundos(microsegundos);

    anguloActual = nuevoAngulo;

    Serial.print(F("Servo: "));
    Serial.print(anguloActual, 1);
    Serial.println(F(" grados (+0.1)"));

    return;
  }

  // MOVIMIENTO INCREMENTAL -

  if (comando == "-") {

    if (!servoInicializado) {
      Serial.println(F("ERROR: Servo no inicializado. Usa INIT primero"));
      return;
    }

    float nuevoAngulo = anguloActual - 0.1;

    if (nuevoAngulo < ANGULO_MINIMO) {
      Serial.println(F("Limite minimo alcanzado"));
      return;
    }

    int microsegundos = map(
      nuevoAngulo * 10,
      0,
      1800,
      500,
      2500
    );

    moverServoMicrosegundos(microsegundos);

    anguloActual = nuevoAngulo;

    Serial.print(F("Servo: "));
    Serial.print(anguloActual, 1);
    Serial.println(F(" grados (-0.1)"));

    return;
  }

  // MOVIMIENTO INCREMENTAL ++

  if (comando == "++") {

    if (!servoInicializado) {
      Serial.println(F("ERROR: Servo no inicializado. Usa INIT primero"));
      return;
    }

    float nuevoAngulo = anguloActual + 1.0;

    if (nuevoAngulo > ANGULO_MAXIMO) {
      Serial.println(F("Limite maximo alcanzado"));
      return;
    }

    int microsegundos = map(
      nuevoAngulo * 10,
      0,
      1800,
      500,
      2500
    );

    moverServoMicrosegundos(microsegundos);

    anguloActual = nuevoAngulo;

    Serial.print(F("Servo: "));
    Serial.print(anguloActual, 1);
    Serial.println(F(" grados (+1.0)"));

    return;
  }

  // MOVIMIENTO INCREMENTAL --

  if (comando == "--") {

    if (!servoInicializado) {
      Serial.println(F("ERROR: Servo no inicializado. Usa INIT primero"));
      return;
    }

    float nuevoAngulo = anguloActual - 1.0;

    if (nuevoAngulo < ANGULO_MINIMO) {
      Serial.println(F("Limite minimo alcanzado"));
      return;
    }

    int microsegundos = map(
      nuevoAngulo * 10,
      0,
      1800,
      500,
      2500
    );

    moverServoMicrosegundos(microsegundos);

    anguloActual = nuevoAngulo;

    Serial.print(F("Servo: "));
    Serial.print(anguloActual, 1);
    Serial.println(F(" grados (-1.0)"));

    return;
  }

  // ==========================================================
  // COMANDOS DE MOTORES
  // ==========================================================

  if (cmd.length() > 0) {

    char primerChar = cmd.charAt(0);

    char segundoChar =
      (cmd.length() > 1) ? cmd.charAt(1) : ' ';

    // ===== MOTOR 1 =====

    if (primerChar == '1' &&
        (segundoChar == '+' || segundoChar == '-')) {

      String numStr = cmd.substring(2);
      parametro = numStr.toInt();

      if (parametro <= 0) {
        Serial.println(F("ERROR: Especifica numero de pasos (ej: 1+500)"));
        return;
      }

      if (!habilitado1) {
        Serial.println(F("ERROR: Motor 1 deshabilitado. Usa 1H"));
        return;
      }

      if (segundoChar == '+') {

        int nuevaPosicion = pos1 + parametro;

        if (nuevaPosicion > RECORRIDO_TOTAL_1) {
          Serial.println(F("ERROR: Supera el limite de 1300 pasos"));
          return;
        }

        if (nuevaPosicion == RECORRIDO_TOTAL_1) {
          Serial.println(F("AVISO: Llegara al limite maximo"));
        }

        digitalWrite(
          DIR1_PIN,
          AVANCE_MOTOR_1 ? HIGH : LOW
        );

        pasosRestantesMotor1 = parametro;
        motor1Moviendo = true;

        Serial.print(F("Motor 1: ABRIENDO "));
        Serial.print(parametro);
        Serial.println(F(" pasos"));

      } else {

        int nuevaPosicion = pos1 - parametro;

        if (nuevaPosicion < 0) {
          Serial.println(F("ERROR: Supera el limite de 0 pasos"));
          return;
        }

        if (nuevaPosicion == 0) {
          Serial.println(F("AVISO: Llegara al limite minimo"));
        }

        digitalWrite(
          DIR1_PIN,
          AVANCE_MOTOR_1 ? LOW : HIGH
        );

        pasosRestantesMotor1 = parametro;
        motor1Moviendo = true;

        Serial.print(F("Motor 1: CERRANDO "));
        Serial.print(parametro);
        Serial.println(F(" pasos"));
      }

      return;
    }

    // ===== MOTOR 2 =====

    else if (primerChar == '2' &&
             (segundoChar == '+' || segundoChar == '-')) {

      String numStr = cmd.substring(2);
      parametro = numStr.toInt();

      if (parametro <= 0) {
        Serial.println(F("ERROR: Especifica numero de pasos (ej: 2+500)"));
        return;
      }

      if (!habilitado2) {
        Serial.println(F("ERROR: Motor 2 deshabilitado. Usa 2H"));
        return;
      }

      if (segundoChar == '+') {

        int nuevaPosicion = pos2 + parametro;

        if (nuevaPosicion > RECORRIDO_TOTAL_2) {
          Serial.println(F("ERROR: Supera el limite de 1300 pasos"));
          return;
        }

        if (nuevaPosicion == RECORRIDO_TOTAL_2) {
          Serial.println(F("AVISO: Llegara al limite maximo"));
        }

        digitalWrite(
          DIR2_PIN,
          AVANCE_MOTOR_2 ? HIGH : LOW
        );

        pasosRestantesMotor2 = parametro;
        motor2Moviendo = true;

        Serial.print(F("Motor 2: ABRIENDO "));
        Serial.print(parametro);
        Serial.println(F(" pasos"));

      } else {

        int nuevaPosicion = pos2 - parametro;

        if (nuevaPosicion < 0) {
          Serial.println(F("ERROR: Supera el limite de 0 pasos"));
          return;
        }

        if (nuevaPosicion == 0) {
          Serial.println(F("AVISO: Llegara al limite minimo"));
        }

        digitalWrite(
          DIR2_PIN,
          AVANCE_MOTOR_2 ? LOW : HIGH
        );

        pasosRestantesMotor2 = parametro;
        motor2Moviendo = true;

        Serial.print(F("Motor 2: CERRANDO "));
        Serial.print(parametro);
        Serial.println(F(" pasos"));
      }

      return;
    }

    // ===== MOTOR 3 =====

    else if (primerChar == '3' &&
             (segundoChar == 'A' || segundoChar == 'a' ||
              segundoChar == 'C' || segundoChar == 'c')) {

      if (!habilitado3) {
        Serial.println(F("ERROR: Motor 3 deshabilitado. Usa 3H"));
        return;
      }

      String resto = cmd.substring(2);
      resto.trim();

      int velocidad = resto.toInt();

      if (velocidad <= 0) {

        velocidad = delayMotor3;

        Serial.print(F("Usando velocidad por defecto M3: "));
        Serial.println(velocidad);

      } else {

        if (velocidad < 200) {
          velocidad = 200;
          Serial.println(F("Velocidad ajustada a 200 us"));

        } else if (velocidad > 5000) {
          velocidad = 5000;
          Serial.println(F("Velocidad ajustada a 5000 us"));
        }
      }

      // ABRIR

      if (segundoChar == 'A' || segundoChar == 'a') {

        int pasosNecesarios =
          RECORRIDO_TOTAL_3 - pos3;

        if (pasosNecesarios <= 0) {
          Serial.println(F("Motor 3 ya esta completamente abierto"));
          return;
        }

        delayMotor3 = velocidad;

        digitalWrite(
          DIR3_PIN,
          AVANCE_MOTOR_3 ? HIGH : LOW
        );

        pasosRestantesMotor3 = pasosNecesarios;
        motor3Moviendo = true;

        Serial.print(F("Motor 3: ABRIENDO "));
        Serial.print(pasosNecesarios);
        Serial.print(F(" pasos a "));
        Serial.print(delayMotor3);
        Serial.println(F(" us"));
      }

      // CERRAR

      else {

        int pasosNecesarios = pos3;

        if (pasosNecesarios <= 0) {
          Serial.println(F("Motor 3 ya esta completamente cerrado"));
          return;
        }

        delayMotor3 = velocidad;

        digitalWrite(
          DIR3_PIN,
          AVANCE_MOTOR_3 ? LOW : HIGH
        );

        pasosRestantesMotor3 = pasosNecesarios;
        motor3Moviendo = true;

        Serial.print(F("Motor 3: CERRANDO "));
        Serial.print(pasosNecesarios);
        Serial.print(F(" pasos a "));
        Serial.print(delayMotor3);
        Serial.println(F(" us"));
      }

      return;
    }

    // ===== TODOS LOS MOTORES =====

    else if (primerChar == '+' || primerChar == '-') {

      String numStr = cmd.substring(1);
      parametro = numStr.toInt();

      if (parametro <= 0) {
        Serial.println(F("ERROR: Especifica numero de pasos (ej: +300)"));
        return;
      }

      if (primerChar == '+') {

        int nuevaPosicion1 = pos1 + parametro;
        int nuevaPosicion2 = pos2 + parametro;

        bool error = false;

        if (nuevaPosicion1 > RECORRIDO_TOTAL_1) {
          Serial.println(F("ERROR Motor 1: Supera el limite"));
          error = true;
        }

        if (nuevaPosicion2 > RECORRIDO_TOTAL_2) {
          Serial.println(F("ERROR Motor 2: Supera el limite"));
          error = true;
        }

        int pasosNecesarios3 =
          RECORRIDO_TOTAL_3 - pos3;

        if (pasosNecesarios3 <= 0) {
          Serial.println(F("Motor 3 ya esta completamente abierto"));
          error = true;
        }

        if (error) {
          Serial.println(F("Comando cancelado"));
          return;
        }

        digitalWrite(
          DIR1_PIN,
          AVANCE_MOTOR_1 ? HIGH : LOW
        );

        pasosRestantesMotor1 = parametro;
        motor1Moviendo = true;

        digitalWrite(
          DIR2_PIN,
          AVANCE_MOTOR_2 ? HIGH : LOW
        );

        pasosRestantesMotor2 = parametro;
        motor2Moviendo = true;

        digitalWrite(
          DIR3_PIN,
          AVANCE_MOTOR_3 ? HIGH : LOW
        );

        pasosRestantesMotor3 = pasosNecesarios3;
        motor3Moviendo = true;

        Serial.println(F("Todos los motores: ABRIENDO"));
      }

      else {

        int nuevaPosicion1 = pos1 - parametro;
        int nuevaPosicion2 = pos2 - parametro;

        bool error = false;

        if (nuevaPosicion1 < 0) {
          Serial.println(F("ERROR Motor 1: Supera el limite"));
          error = true;
        }

        if (nuevaPosicion2 < 0) {
          Serial.println(F("ERROR Motor 2: Supera el limite"));
          error = true;
        }

        int pasosNecesarios3 = pos3;

        if (pasosNecesarios3 <= 0) {
          Serial.println(F("Motor 3 ya esta completamente cerrado"));
          error = true;
        }

        if (error) {
          Serial.println(F("Comando cancelado"));
          return;
        }

        digitalWrite(
          DIR1_PIN,
          AVANCE_MOTOR_1 ? LOW : HIGH
        );

        pasosRestantesMotor1 = parametro;
        motor1Moviendo = true;

        digitalWrite(
          DIR2_PIN,
          AVANCE_MOTOR_2 ? LOW : HIGH
        );

        pasosRestantesMotor2 = parametro;
        motor2Moviendo = true;

        digitalWrite(
          DIR3_PIN,
          AVANCE_MOTOR_3 ? LOW : HIGH
        );

        pasosRestantesMotor3 = pasosNecesarios3;
        motor3Moviendo = true;

        Serial.println(F("Todos los motores: CERRANDO"));
      }

      return;
    }
  }

  // ==========================================================
  // OTROS COMANDOS
  // ==========================================================

  comando.toUpperCase();

  // MOTOR 1

  if (comando == "1S") {

    motor1Moviendo = false;
    pasosRestantesMotor1 = 0;

    Serial.println(F("Motor 1: DETENIDO"));
    Serial.print(F("Posicion: "));
    Serial.println(pos1);

    Serial.print(F("Pasos totales: "));
    Serial.println(totalPasosMotor1);
  }

  else if (comando == "1P") {

    Serial.print(F("Motor 1 - Posicion: "));
    Serial.print(pos1);
    Serial.print(F(" de "));
    Serial.println(RECORRIDO_TOTAL_1);

    int porcentaje =
      map(pos1, 0, RECORRIDO_TOTAL_1, 0, 100);

    Serial.print(F("Apertura: "));
    Serial.print(porcentaje);
    Serial.println(F("%"));

    Serial.print(F("Pasos totales: "));
    Serial.println(totalPasosMotor1);
  }

  else if (comando == "1H") {

    digitalWrite(EN1_PIN, LOW);
    habilitado1 = true;

    Serial.println(F("Motor 1 HABILITADO"));
  }

  else if (comando == "1D") {

    digitalWrite(EN1_PIN, HIGH);
    habilitado1 = false;

    motor1Moviendo = false;
    pasosRestantesMotor1 = 0;

    Serial.println(F("Motor 1 DESHABILITADO"));
  }

  // MOTOR 2

  else if (comando == "2S") {

    motor2Moviendo = false;
    pasosRestantesMotor2 = 0;

    Serial.println(F("Motor 2: DETENIDO"));
    Serial.print(F("Posicion: "));
    Serial.println(pos2);

    Serial.print(F("Pasos totales: "));
    Serial.println(totalPasosMotor2);
  }

  else if (comando == "2P") {

    Serial.print(F("Motor 2 - Posicion: "));
    Serial.print(pos2);
    Serial.print(F(" de "));
    Serial.println(RECORRIDO_TOTAL_2);

    int porcentaje =
      map(pos2, 0, RECORRIDO_TOTAL_2, 0, 100);

    Serial.print(F("Apertura: "));
    Serial.print(porcentaje);
    Serial.println(F("%"));

    Serial.print(F("Pasos totales: "));
    Serial.println(totalPasosMotor2);
  }

  else if (comando == "2H") {

    digitalWrite(EN2_PIN, LOW);
    habilitado2 = true;

    Serial.println(F("Motor 2 HABILITADO"));
  }

  else if (comando == "2D") {

    digitalWrite(EN2_PIN, HIGH);
    habilitado2 = false;

    motor2Moviendo = false;
    pasosRestantesMotor2 = 0;

    Serial.println(F("Motor 2 DESHABILITADO"));
  }

  // MOTOR 3

  else if (comando == "3S") {

    motor3Moviendo = false;
    pasosRestantesMotor3 = 0;

    Serial.println(F("Motor 3: DETENIDO"));
    Serial.print(F("Posicion: "));
    Serial.println(pos3);

    Serial.print(F("Pasos totales: "));
    Serial.println(totalPasosMotor3);
  }

  else if (comando == "3P") {

    Serial.print(F("Motor 3 - Posicion: "));
    Serial.print(pos3);
    Serial.print(F(" de "));
    Serial.println(RECORRIDO_TOTAL_3);

    int porcentaje =
      map(pos3, 0, RECORRIDO_TOTAL_3, 0, 100);

    Serial.print(F("Apertura: "));
    Serial.print(porcentaje);
    Serial.println(F("%"));

    Serial.print(F("Pasos totales: "));
    Serial.println(totalPasosMotor3);
  }

  else if (comando == "3H") {

    digitalWrite(EN3_PIN, LOW);
    habilitado3 = true;

    Serial.println(F("Motor 3 HABILITADO"));
  }

  else if (comando == "3D") {

    digitalWrite(EN3_PIN, HIGH);
    habilitado3 = false;

    motor3Moviendo = false;
    pasosRestantesMotor3 = 0;

    Serial.println(F("Motor 3 DESHABILITADO"));
  }

  // VELOCIDAD MOTOR 3

  else if (comando == "V3+") {

    if (delayMotor3 > 300) {

      delayMotor3 -= 200;

      Serial.print(F("Velocidad M3 aumentada. Delay: "));
      Serial.println(delayMotor3);

    } else {

      Serial.println(F("Velocidad maxima M3 (200 us)"));
    }
  }

  else if (comando == "V3-") {

    if (delayMotor3 < 5000) {

      delayMotor3 += 200;

      Serial.print(F("Velocidad M3 disminuida. Delay: "));
      Serial.println(delayMotor3);

    } else {

      Serial.println(F("Velocidad minima M3 (5000 us)"));
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

    Serial.println(F("Todos los motores: DETENIDOS"));

    Serial.print(F("Posiciones - M1: "));
    Serial.print(pos1);

    Serial.print(F(" | M2: "));
    Serial.print(pos2);

    Serial.print(F(" | M3: "));
    Serial.println(pos3);
  }

  // DIAGNOSTICO

  else if (comando == "DIAG") {

    diagnosticando = !diagnosticando;

    if (diagnosticando) {

      Serial.println(F("DIAGNOSTICO ACTIVADO"));
      tiempoDiagnostico = millis();

    } else {

      Serial.println(F("DIAGNOSTICO DESACTIVADO"));
    }
  }

  else if (comando == "ESTADO") {

    mostrarDiagnostico();
  }

  else if (cmd != "") {

    ethernetSerial.println("sdadsadas");

    Serial.print(F("Comando no valido: "));
    Serial.println(cmd);
  }
}

// ============================================================
// DIAGNOSTICO
// ============================================================

void mostrarDiagnostico() {

  Serial.println(F("===== ESTADO COMPLETO ====="));

  int porcentaje1 =
    map(pos1, 0, RECORRIDO_TOTAL_1, 0, 100);

  int porcentaje2 =
    map(pos2, 0, RECORRIDO_TOTAL_2, 0, 100);

  int porcentaje3 =
    map(pos3, 0, RECORRIDO_TOTAL_3, 0, 100);

  // MOTOR 1

  Serial.print(F("Motor 1: "));
  Serial.print(habilitado1 ? F("HABILITADO") : F("DESHABILITADO"));

  Serial.print(F(" | Moviendo: "));
  Serial.print(motor1Moviendo ? F("SI") : F("NO"));

  Serial.print(F(" | Posicion: "));
  Serial.print(pos1);
  Serial.print(F("/"));
  Serial.print(RECORRIDO_TOTAL_1);

  Serial.print(F(" ("));
  Serial.print(porcentaje1);
  Serial.print(F("%) | Pasos totales: "));
  Serial.println(totalPasosMotor1);

  // MOTOR 2

  Serial.print(F("Motor 2: "));
  Serial.print(habilitado2 ? F("HABILITADO") : F("DESHABILITADO"));

  Serial.print(F(" | Moviendo: "));
  Serial.print(motor2Moviendo ? F("SI") : F("NO"));

  Serial.print(F(" | Posicion: "));
  Serial.print(pos2);
  Serial.print(F("/"));
  Serial.print(RECORRIDO_TOTAL_2);

  Serial.print(F(" ("));
  Serial.print(porcentaje2);
  Serial.print(F("%) | Pasos totales: "));
  Serial.println(totalPasosMotor2);

  // MOTOR 3

  Serial.print(F("Motor 3: "));
  Serial.print(habilitado3 ? F("HABILITADO") : F("DESHABILITADO"));

  Serial.print(F(" | Moviendo: "));
  Serial.print(motor3Moviendo ? F("SI") : F("NO"));

  Serial.print(F(" | Posicion: "));
  Serial.print(pos3);
  Serial.print(F("/"));
  Serial.print(RECORRIDO_TOTAL_3);

  Serial.print(F(" ("));
  Serial.print(porcentaje3);
  Serial.print(F("%) | Pasos totales: "));
  Serial.print(totalPasosMotor3);

  Serial.print(F(" | Delay: "));
  Serial.println(delayMotor3);

  // SERVO

  Serial.print(F("Servo: "));

  if (servoInicializado) {

    Serial.print(F("INICIALIZADO | Angulo: "));
    Serial.print(anguloActual, 1);

    Serial.print(F(" | Conectado: "));
    Serial.println(servo.attached() ? F("SI") : F("NO"));

  } else {

    Serial.println(F("NO INICIALIZADO (usa INIT)"));
  }

  Serial.println(F("=============================="));
}
