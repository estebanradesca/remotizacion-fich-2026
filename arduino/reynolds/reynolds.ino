#include <Wire.h>

/* ============================================================
   SISTEMA DE CONTROL DE LÍQUIDOS - PROTOCOLO NMEA
   VERSIÓN COMPLETA PARA CONVERSOR UART-TTL A ETHERNET
   ============================================================

   HARDWARE: Arduino Uno / Nano
   CONVERSOR: UART-TTL a Ethernet RJ45 (TCP/UDP/MQTT/Modbus)

   ============================================================
   CONEXIONES COMPLETAS
   ============================================================

   // ===== ALIMENTACIÓN =====
   Arduino 5V      --> ProtoBoard
   Arduino GND     --> ProtoBoard
   Fuente 12V      --> VMOT Drivers
   Fuente 12V GND  --> GND Drivers
   Fuente 24V      --> COM Relé
   Fuente 24V GND  --> Negativo ElectroVálvula

   // ===== MOTOR 1 - Válvula de tinta (Driver A4988) =====
   DIR Driver 1    --> PIN 2
   STEP Driver 1   --> PIN 3
   ENABLE Driver 1 --> PIN 4

   // ===== MOTOR 2 - Regulador de caudal (Driver A4988) =====
   DIR Driver 2    --> PIN 5
   STEP Driver 2   --> PIN 6
   ENABLE Driver 2 --> PIN 7

   // ===== SENSOR DE NIVEL =====
   Signal (S)      --> A0

   // ===== RELÉ =====
   IN1 Relé        --> PIN 9

   // ===== CAUDALÍMETRO =====
   Signal           --> PIN 8 (Interrupción)

   // ===== TEMPERATURA DS18B20 =====
   Data            --> PIN 10 (con resistencia 4.7kΩ pull-up)

   // ===== CONVERSOR UART-TTL A ETHERNET ====
   RX Conversor    --> PIN 11 (TX del Arduino)
   TX Conversor    --> PIN 12 (RX del Arduino)
   GND Conversor   --> GND ProtoBoard
   VCC Conversor   --> 5V o 3.3V (verificar voltaje del módulo)

   // ===== PUERTO USB (Depuración) =====
   USB Arduino     --> PC

   ============================================================ */

#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <SPI.h>
#include <SoftwareSerial.h>

// ============================================================
// DEFINICIÓN DE PINES
// ============================================================

// Motores
#define STEP_PIN_1 3
#define DIR_PIN_1 2
#define ENABLE_PIN_1 4

#define STEP_PIN_2 6
#define DIR_PIN_2 5
#define ENABLE_PIN_2 7

// Sensores
#define SENSOR_NIVEL_PIN A0
#define RELE_PIN 9
const int sensorPin = 8;  // Caudalímetro

// Temperatura
#define ONE_WIRE_BUS 10

SoftwareSerial puertoSerial(11,12);

// ============================================================
// VARIABLES GLOBALES
// ============================================================

// ----- CAUDALÍMETRO -----
volatile unsigned long pulseCounter = 0;
const float factorK = 5.4109;
float caudalActual = 0;
float frecuenciaActual = 0;

// ----- TEMPERATURA -----
OneWire ourWire(ONE_WIRE_BUS);
DallasTemperature sensors(&ourWire);
float temperaturaActual = 0;

// ----- MOTORES -----
unsigned int velocidad = 5;  // ms entre pasos (5 = rápido, 100 = lento)
const int LIMITE_PASOS = 50;
const int LIMITE_PASOS_M2 = 600;

int pos1 = 0;
int pos2 = 0;
bool habilitado1 = true;
bool habilitado2 = true;

bool motor1Moviendo = false;
bool motor2Moviendo = false;
bool direccionMotor1 = true;
bool direccionMotor2 = true;
int pasosRestantesMotor1 = 0;
int pasosRestantesMotor2 = 0;
unsigned long tiempoUltimoPasoMotor1 = 0;
unsigned long tiempoUltimoPasoMotor2 = 0;

// ----- SENSOR NIVEL Y RELÉ -----
int porcentajeActual = 0;
bool estadoRele = false;

// ----- TIMERS PARA LECTURAS PERIÓDICAS -----
unsigned long tiempoAnteriorSensor = 0;
unsigned long tiempoAnteriorCaudal = 0;
unsigned long tiempoAnteriorTemp = 0;
unsigned long tiempoAnteriorNMEA = 0;

const unsigned long INTERVALO_SENSOR = 1000;   // Leer nivel cada 1 segundo
const unsigned long INTERVALO_CAUDAL = 2000;   // Medir caudal cada 2 segundos
const unsigned long INTERVALO_TEMP = 3000;     // Medir temperatura cada 3 segundos
const unsigned long INTERVALO_NMEA = 1000;     // Enviar datos NMEA cada 1 segundo

bool midiendoCaudal = false;
unsigned long tiempoInicioCaudal = 0;

// ----- BUFFER PARA COMANDOS SERIAL -----
String comandoBuffer = "";

// ============================================================
// FUNCIONES NMEA (Checksum y envío)
// ============================================================

byte calcularChecksum(String oracion) {
  byte checksum = 0;
  for (int i = 0; i < oracion.length(); i++) {
    checksum ^= (byte)oracion[i];
  }
  return checksum;
}

void enviarNMEA(String tipo, String datos) {
  String oracion = "$" + tipo + "," + datos;
  byte checksum = calcularChecksum(oracion.substring(1));
  char hex[3];
  sprintf(hex, "%02X", checksum);
  oracion += "*" + String(hex);

  // Enviar por puertoSerial (conversor UART-Ethernet en pines 11,12)
  puertoSerial.println(oracion);

  // También enviar por USB para depuración
  Serial.println(oracion);
}

void enviarDatosNMEA() {
  // Formato: $SD,caudal,temperatura,nivel,rele,motor1_pos,motor2_pos*CS
  String datos = String(caudalActual, 2) + ",";
  datos += String(temperaturaActual, 1) + ",";
  datos += String(porcentajeActual) + ",";
  datos += String(estadoRele ? 1 : 0) + ",";
  datos += String(pos1) + ",";
  datos += String(pos2);

  enviarNMEA("SD", datos);
}

void enviarACK(String comando, String estado) {
  enviarNMEA("ACK", comando + "," + estado);
}

void enviarERROR(String comando, String motivo) {
  enviarNMEA("ERR", comando + "," + motivo);
}

// ============================================================
// PROCESAMIENTO DE COMANDOS NMEA
// ============================================================

void procesarComando(String comando) {
  comando.trim();

  if (comando.length() == 0) return;

  // Verificar formato NMEA (debe empezar con $)
  if (!comando.startsWith("$")) {
    enviarERROR("INVALID", "Missing $ prefix");
    return;
  }

  // Eliminar checksum si existe
  int asterisco = comando.indexOf('*');
  String cmdSinChecksum;
  String checksumRecibido = "";

  if (asterisco > 0) {
    Serial.println(comando.substring(1, asterisco));
    cmdSinChecksum = comando.substring(1, asterisco);
    checksumRecibido = comando.substring(asterisco + 1);

    // Verificar checksum
    byte checksumCalc = calcularChecksum(cmdSinChecksum);
    char hex[3];
    sprintf(hex, "%02X", checksumCalc);
    if (String(hex) != checksumRecibido) {
      enviarERROR("CHECKSUM", "Invalid checksum");
      return;
    }
  } else {
    cmdSinChecksum = comando.substring(1);
  }

  // Separar campos por coma
  int indice = 0;
  String campos[10];
  int numCampos = 0;

  while (indice < cmdSinChecksum.length() && numCampos < 10) {
    int coma = cmdSinChecksum.indexOf(',', indice);
    if (coma == -1) {
      campos[numCampos++] = cmdSinChecksum.substring(indice);
      break;
    } else {
      campos[numCampos++] = cmdSinChecksum.substring(indice, coma);
      indice = coma + 1;
    }
  }

  if (numCampos == 0) return;

  String tipo = campos[0];

  // ===== COMANDO: OBTENER DATOS (GET) =====
  if (tipo == "GET") {
    enviarDatosNMEA();
    enviarACK("GET", "OK");
  }

  // ===== COMANDO: PING =====
  else if (tipo == "PING") {
    enviarNMEA("PONG", "OK");
  }

  // ===== COMANDO: MOTOR 1 =====
  else if (tipo == "M1") {
    if (numCampos < 2) {
      enviarERROR("M1", "Missing argument");
      return;
    }

    String accion = campos[1];

    if (accion == "STEP") {
      if (numCampos >= 3) {
        int pasos = campos[2].toInt();
        if (pasos > 0) {
          moverMotor1NoBloqueante(true, pasos);
        } else if (pasos < 0) {
          moverMotor1NoBloqueante(false, -pasos);
        }
        enviarACK("M1,STEP", String(pasos));
      } else {
        enviarERROR("M1", "STEP requires number");
      }
    }
    else if (accion == "POS") {
      if (numCampos >= 3) {
        int posDestino = campos[2].toInt();
        posDestino = constrain(posDestino, 0, LIMITE_PASOS);
        int diferencia = posDestino - pos1;
        if (diferencia > 0) {
          moverMotor1NoBloqueante(true, diferencia);
        } else if (diferencia < 0) {
          moverMotor1NoBloqueante(false, -diferencia);
        }
        enviarACK("M1,POS", String(posDestino));
      } else {
        enviarERROR("M1", "POS requires position");
      }
    }
    else if (accion == "HOME") {
      moverMotor1NoBloqueante(false, pos1);
      enviarACK("M1,HOME", "OK");
    }
    else if (accion == "ENABLE") {
      if (numCampos >= 3) {
        if (campos[2] == "1") {
          habilitarMotor1();
        } else {
          deshabilitarMotor1();
        }
        enviarACK("M1,ENABLE", campos[2]);
      } else {
        enviarERROR("M1", "ENABLE requires 1 or 0");
      }
    }
    else {
      enviarERROR("M1", "Unknown action: " + accion);
    }
  }

  // ===== COMANDO: MOTOR 2 =====
  else if (tipo == "M2") {
    if (numCampos < 2) {
      enviarERROR("M2", "Missing argument");
      return;
    }

    String accion = campos[1];

    if (accion == "STEP") {
      if (numCampos >= 3) {
        int pasos = campos[2].toInt();
        if (pasos > 0) {
          moverMotor2NoBloqueante(true, pasos);
        } else if (pasos < 0) {
          moverMotor2NoBloqueante(false, -pasos);
        }
        enviarACK("M2,STEP", String(pasos));
      } else {
        enviarERROR("M2", "STEP requires number");
      }
    }
    else if (accion == "POS") {
      if (numCampos >= 3) {
        int posDestino = campos[2].toInt();
        posDestino = constrain(posDestino, 0, LIMITE_PASOS_M2);
        int diferencia = posDestino - pos2;
        if (diferencia > 0) {
          moverMotor2NoBloqueante(true, diferencia);
        } else if (diferencia < 0) {
          moverMotor2NoBloqueante(false, -diferencia);
        }
        enviarACK("M2,POS", String(posDestino));
      } else {
        enviarERROR("M2", "POS requires position");
      }
    }
    else if (accion == "HOME") {
      moverMotor2NoBloqueante(false, pos2);
      enviarACK("M2,HOME", "OK");
    }
    else if (accion == "MAX") {
      moverMotor2NoBloqueante(true, LIMITE_PASOS_M2 - pos2);
      enviarACK("M2,MAX", "OK");
    }
    else if (accion == "ENABLE") {
      if (numCampos >= 3) {
        if (campos[2] == "1") {
          habilitarMotor2();
        } else {
          deshabilitarMotor2();
        }
        enviarACK("M2,ENABLE", campos[2]);
      } else {
        enviarERROR("M2", "ENABLE requires 1 or 0");
      }
    }
    else {
      enviarERROR("M2", "Unknown action: " + accion);
    }
  }

  // ===== COMANDO: RELÉ =====
  else if (tipo == "RELE") {
    if (numCampos < 2) {
      enviarERROR("RELE", "Missing argument");
      return;
    }

    String accion = campos[1];
    if (accion == "ON") {
      activarRele();
      enviarACK("RELE,ON", "OK");
    }
    else if (accion == "OFF") {
      desactivarRele();
      enviarACK("RELE,OFF", "OK");
    }
    else if (accion == "TOGGLE") {
      if (estadoRele) {
        desactivarRele();
      } else {
        activarRele();
      }
      enviarACK("RELE,TOGGLE", String(estadoRele ? 1 : 0));
    }
    else {
      enviarERROR("RELE", "Unknown action: " + accion);
    }
  }

  // ===== COMANDO: CONFIGURACIÓN =====
  else if (tipo == "CONFIG") {
    if (numCampos < 2) {
      enviarERROR("CONFIG", "Missing argument");
      return;
    }

    String param = campos[1];
    if (param == "VEL" && numCampos >= 3) {
      velocidad = campos[2].toInt();
      if (velocidad < 1) velocidad = 1;
      if (velocidad > 100) velocidad = 100;
      enviarACK("CONFIG,VEL", String(velocidad));
    }
    else {
      enviarERROR("CONFIG", "Unknown parameter: " + param);
    }
  }

  // ===== COMANDO DESCONOCIDO =====
  else {
    enviarERROR("UNKNOWN", tipo);
  }
}

// ============================================================
// FUNCIONES DE CONTROL DE MOTOR 1
// ============================================================

void moverMotor1NoBloqueante(bool dir, int pasos) {
  if (!habilitado1) {
    enviarERROR("M1", "Motor disabled");
    return;
  }
  if (pasos < 0) {
    enviarERROR("M1", "Invalid steps");
    return;
  }

  // Verificar límites y ajustar si es necesario
  if (dir && pos1 + pasos > LIMITE_PASOS) {
    pasos = LIMITE_PASOS - pos1;
    if (pasos < 0) {
      enviarERROR("M1", "At upper limit");
      return;
    }
    Serial.print("Motor 1 - Limitando a ");
    Serial.print(pasos);
    Serial.println(" pasos");
  }
  else if (!dir && pos1 - pasos < 0) {
    pasos = pos1;
    if (pasos < 0) {
      enviarERROR("M1", "At lower limit");
      return;
    }
    Serial.print("Motor 1 - Limitando a ");
    Serial.print(pasos);
    Serial.println(" pasos");
  }

  direccionMotor1 = dir;
  pasosRestantesMotor1 = pasos;
  motor1Moviendo = true;
  tiempoUltimoPasoMotor1 = millis();

  Serial.print("Motor 1: ");
  Serial.print(pasos);
  Serial.print(" pasos ");
  Serial.println(dir ? "ADELANTE" : "ATRAS");
}

void actualizarMotor1() {
  if (!motor1Moviendo || !habilitado1) return;

  // Control de tiempo entre pasos (velocidad)
  if (millis() - tiempoUltimoPasoMotor1 >= velocidad) {
    tiempoUltimoPasoMotor1 = millis();

    digitalWrite(DIR_PIN_1, direccionMotor1);
    digitalWrite(STEP_PIN_1, HIGH);
    delayMicroseconds(10);
    digitalWrite(STEP_PIN_1, LOW);

    if (direccionMotor1) {
      pos1++;
    } else {
      pos1--;
    }

    pasosRestantesMotor1--;

    if (pasosRestantesMotor1 == 0) {
      motor1Moviendo = false;
      Serial.print("Motor 1 en posición: ");
      Serial.println(pos1);
      enviarNMEA("M1POS", String(pos1));
    }
  }
}

void habilitarMotor1() {
  digitalWrite(ENABLE_PIN_1, LOW);  // LOW = habilitado
  habilitado1 = true;
  Serial.println("Motor 1 HABILITADO");
  enviarNMEA("M1STA", "ENABLED");
}

void deshabilitarMotor1() {
  motor1Moviendo = false;
  digitalWrite(ENABLE_PIN_1, HIGH); // HIGH = deshabilitado
  habilitado1 = false;
  Serial.println("Motor 1 DESHABILITADO");
  enviarNMEA("M1STA", "DISABLED");
}

// ============================================================
// FUNCIONES DE CONTROL DE MOTOR 2
// ============================================================

void moverMotor2NoBloqueante(bool dir, int pasos) {
  if (!habilitado2) {
    enviarERROR("M2", "Motor disabled");
    return;
  }
  if (pasos < 0) {
    enviarERROR("M2", "Invalid steps");
    return;
  }

  // Verificar límites y ajustar si es necesario
  if (dir && pos2 + pasos > LIMITE_PASOS_M2) {
    pasos = LIMITE_PASOS_M2 - pos2;
    if (pasos < 0) {
      enviarERROR("M2", "At upper limit");
      return;
    }
    Serial.print("Motor 2 - Limitando a ");
    Serial.print(pasos);
    Serial.println(" pasos");
  }
  else if (!dir && pos2 - pasos < 0) {
    pasos = pos2;
    if (pasos < 0) {
      enviarERROR("M2", "At lower limit");
      return;
    }
    Serial.print("Motor 2 - Limitando a ");
    Serial.print(pasos);
    Serial.println(" pasos");
  }

  direccionMotor2 = dir;
  pasosRestantesMotor2 = pasos;
  motor2Moviendo = true;
  tiempoUltimoPasoMotor2 = millis();

  Serial.print("Motor 2: ");
  Serial.print(pasos);
  Serial.print(" pasos ");
  Serial.println(dir ? "ADELANTE" : "ATRAS");
}

void actualizarMotor2() {
  if (!motor2Moviendo || !habilitado2) return;

  // Control de tiempo entre pasos (velocidad)
  if (millis() - tiempoUltimoPasoMotor2 >= velocidad) {
    tiempoUltimoPasoMotor2 = millis();

    digitalWrite(DIR_PIN_2, direccionMotor2);
    digitalWrite(STEP_PIN_2, HIGH);
    delayMicroseconds(10);
    digitalWrite(STEP_PIN_2, LOW);

    if (direccionMotor2) {
      pos2++;
    } else {
      pos2--;
    }

    pasosRestantesMotor2--;

    if (pasosRestantesMotor2 == 0) {
      motor2Moviendo = false;
      Serial.print("Motor 2 en posición: ");
      Serial.println(pos2);
      enviarNMEA("M2POS", String(pos2));
    }
  }
}

void habilitarMotor2() {
  digitalWrite(ENABLE_PIN_2, LOW);
  habilitado2 = true;
  Serial.println("Motor 2 HABILITADO");
  enviarNMEA("M2STA", "ENABLED");
}

void deshabilitarMotor2() {
  motor2Moviendo = false;
  digitalWrite(ENABLE_PIN_2, HIGH);
  habilitado2 = false;
  Serial.println("Motor 2 DESHABILITADO");
  enviarNMEA("M2STA", "DISABLED");
}

// ============================================================
// FUNCIONES DE SENSORES
// ============================================================

void medirCaudal() {
  if (!midiendoCaudal) {
    // Inicia nuevo período de medición
    pulseCounter = 0;
    midiendoCaudal = true;
    tiempoInicioCaudal = millis();
  } else {
    // Finaliza medición después de 1 segundo
    if (millis() - tiempoInicioCaudal >= 1000) {
      float frecuencia = pulseCounter;           // pulsos por segundo = Hz
      caudalActual = frecuencia / factorK;       // Conversión a L/min
      frecuenciaActual = frecuencia;
      midiendoCaudal = false;

      Serial.print("Caudal: ");
      Serial.print(caudalActual);
      Serial.println(" L/min");
    }
  }
}

void medirTemperatura() {
  sensors.requestTemperatures();                 // Solicita lectura
  temperaturaActual = sensors.getTempCByIndex(0); // Obtiene temperatura

  Serial.print("Temperatura: ");
  Serial.print(temperaturaActual);
  Serial.println(" °C");
}

void leerYMostrarNivel() {
  int valorSensor = analogRead(SENSOR_NIVEL_PIN);
  porcentajeActual = map(valorSensor, 0, 1023, 0, 100);

  Serial.print("Nivel de agua: ");
  Serial.print(porcentajeActual);
  Serial.println("%");
 /*
  // Control automático del relé basado en nivel
  if (porcentajeActual >= 50 && !estadoRele) {
    Serial.println("Nivel alto - Activando relé");
    activarRele();
    enviarNMEA("NIVEL", String(porcentajeActual) + ",RELE_ON");
  }
  else if (porcentajeActual <= 30 && estadoRele) {
    Serial.println("Nivel bajo - Desactivando relé");
    desactivarRele();
    enviarNMEA("NIVEL", String(porcentajeActual) + ",RELE_OFF");
  }
  */
}

void activarRele() {
  delay(10);
  digitalWrite(RELE_PIN, HIGH);
  delay(10);
  estadoRele = true;
  Serial.println("RELÉ ACTIVADO");
  enviarNMEA("RELE", "ON");
}

void desactivarRele() {
  delay(10);
  digitalWrite(RELE_PIN, LOW);
  delay(10);
  estadoRele = false;
  Serial.println("RELÉ DESACTIVADO");
  enviarNMEA("RELE", "OFF");
}

// ============================================================
// INTERRUPCIÓN PARA CAUDALÍMETRO
// ============================================================

void ISRCountPulse() {
  if (midiendoCaudal) {
    pulseCounter++;
  }
}

// ============================================================
// SETUP - CONFIGURACIÓN INICIAL
// ============================================================

void setup() {
  // ===== CONFIGURACIÓN DE PINES =====
  // Motores como salidas
  pinMode(STEP_PIN_1, OUTPUT);
  pinMode(DIR_PIN_1, OUTPUT);
  pinMode(ENABLE_PIN_1, OUTPUT);

  pinMode(STEP_PIN_2, OUTPUT);
  pinMode(DIR_PIN_2, OUTPUT);
  pinMode(ENABLE_PIN_2, OUTPUT);

  // Sensores y actuadores
  pinMode(SENSOR_NIVEL_PIN, INPUT);
  pinMode(RELE_PIN, OUTPUT);
  pinMode(sensorPin, INPUT_PULLUP);

  // ===== ESTADO INICIAL =====
  digitalWrite(RELE_PIN, LOW);
  estadoRele = false;

  digitalWrite(ENABLE_PIN_1, LOW);  // Habilitar motor 1
  digitalWrite(ENABLE_PIN_2, LOW);  // Habilitar motor 2
  delay(100);

  // ===== INICIALIZAR SENSORES =====
  sensors.begin();
  attachInterrupt(digitalPinToInterrupt(sensorPin), ISRCountPulse, RISING);

  // ===== CONFIGURACIÓN DE PUERTOS SERIAL =====
  // Serial (USB) para depuración en PC
  Serial.begin(115200);

  // puertoSerial (pines 11=TX, 12=RX) para comunicación con conversor UART-Ethernet
  // ¡ESTE ES EL PUERTO QUE USA EL CONVERSOR!
  puertoSerial.begin(115200);

  // ===== MENSAJE DE BIENVENIDA =====
  Serial.println("=================================================");
  Serial.println("SISTEMA NMEA INICIADO - CONVERSOR UART A ETHERNET");
  Serial.println("=================================================");
  Serial.println("Serial (USB):   115200 baudios (depuración)");
  Serial.println("puertoSerial (pines 11,12): 115200 baudios (conversor)");
  Serial.println("=================================================");
  Serial.println("COMANDOS DISPONIBLES:");
  Serial.println("  $GET*3A                - Obtener datos");
  Serial.println("  $PING*5B               - Verificar conexión");
  Serial.println("  $M1,STEP,30*2F        - Motor1 avanza 30 pasos");
  Serial.println("  $M1,STEP,-20*1C       - Motor1 retrocede 20 pasos");
  Serial.println("  $M1,POS,25*4B         - Motor1 a posición 25");
  Serial.println("  $M1,HOME*7A           - Motor1 a cero");
  Serial.println("  $M1,ENABLE,1*3C       - Habilitar Motor1");
  Serial.println("  $M1,ENABLE,0*3D       - Deshabilitar Motor1");
  Serial.println("  $M2,STEP,150*2E       - Motor2 avanza 150 pasos");
  Serial.println("  $M2,STEP,-75*1D       - Motor2 retrocede 75 pasos");
  Serial.println("  $M2,POS,300*4A        - Motor2 a posición 300");
  Serial.println("  $M2,HOME*7B           - Motor2 a cero");
  Serial.println("  $M2,MAX*7C            - Motor2 al límite máximo (600)");
  Serial.println("  $M2,ENABLE,1*3F       - Habilitar Motor2");
  Serial.println("  $M2,ENABLE,0*3E       - Deshabilitar Motor2");
  Serial.println("  $RELE,ON*1C           - Activar relé");
  Serial.println("  $RELE,OFF*1D          - Desactivar relé");
  Serial.println("  $RELE,TOGGLE*1E       - Cambiar estado relé");
  Serial.println("  $CONFIG,VEL,10*5A     - Velocidad motores (1-100 ms)");
  Serial.println("=================================================");
  Serial.println("FORMATO: $COMANDO*XX (siempre terminar con \\n)");
  Serial.println("=================================================");

  // Enviar mensaje de inicio por el conversor
  enviarNMEA("SYS", "STARTED,UART-Ethernet");
  enviarNMEA("INFO", "M1_LIMIT=50,M2_LIMIT=600");
}

// ============================================================
// LOOP PRINCIPAL
// ============================================================

void loop() {
  unsigned long ahora = millis();

  // ===== ACTUALIZAR MOVIMIENTOS DE MOTORES =====
  actualizarMotor1();
  actualizarMotor2();

  // ===== LECTURAS PERIÓDICAS DE SENSORES =====
  // Sensor de nivel (cada 1 segundo)
  if (ahora - tiempoAnteriorSensor >= INTERVALO_SENSOR) {
    tiempoAnteriorSensor = ahora;
    leerYMostrarNivel();
  }

  // Caudalímetro (cada 2 segundos)
  if (ahora - tiempoAnteriorCaudal >= INTERVALO_CAUDAL) {
    tiempoAnteriorCaudal = ahora;
    medirCaudal();
  }

  // Temperatura (cada 3 segundos)
  if (ahora - tiempoAnteriorTemp >= INTERVALO_TEMP) {
    tiempoAnteriorTemp = ahora;
    medirTemperatura();
  }

  // Enviar datos NMEA (cada 1 segundo)
  if (ahora - tiempoAnteriorNMEA >= INTERVALO_NMEA) {
    tiempoAnteriorNMEA = ahora;
    enviarDatosNMEA();
  }

  // ===== PROCESAR COMANDOS POR puertoSerial (CONVERSOR UART-ETHERNET) =====
  while (puertoSerial.available()) {
    char c = puertoSerial.read();

    // Acumula caracteres hasta encontrar \n o \r
    if (c == '\n' || c == '\r') {
      if (comandoBuffer.length() > 0) {
        Serial.print("Comando recibido por RED: ");
        Serial.println(comandoBuffer);
        procesarComando(comandoBuffer);
        comandoBuffer = "";
      }
    } else {
      if (c != '\0') {
        comandoBuffer += c;
      }
    }
  }

  // ===== PROCESAR COMANDOS POR USB (SOLO PARA PRUEBAS) =====
  while (Serial.available()) {
    char c = Serial.read();

    if (c == '\n' || c == '\r') {
      if (comandoBuffer.length() > 0) {
        Serial.print("Comando recibido por USB: ");
        Serial.println(comandoBuffer);
        procesarComando(comandoBuffer);
        comandoBuffer = "";
      }
    } else {
      if (c != '\0') {
        comandoBuffer += c;
      }
    }
  }
}
