#include <EEPROM.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>
#include <Servo.h>

// Pines
#define LED_SALA 2
#define LED_COMEDOR 3
#define LED_COCINA 4
#define LED_BANO 5
#define LED_HABITACION 6
#define PIN_VENTILADOR 7
#define PIN_LED_AZUL 8
#define PIN_SERVO 9
#define PIN_BOTON_PUERTA 12
#define PIN_LED_VERDE A0
#define PIN_LED_ROJO A1

// EEPROM
#define EEPROM_FIESTA 0
#define EEPROM_RELAJADO 10
#define EEPROM_NOCHE 20
#define EEPROM_ENCENDER_TODO 30
#define EEPROM_APAGAR_TODO 40

enum Modo {
  FIESTA = 1,
  RELAJADO = 2,
  NOCHE = 3,
  ENCENDER_TODO = 4,
  APAGAR_TODO = 5
};

struct EstadoModo {
  bool sala;
  bool comedor;
  bool cocina;
  bool bano;
  bool habitacion;
  bool ventilador;
  bool alternar;
};

LiquidCrystal_I2C lcd(0x27, 16, 2);
SoftwareSerial bluetooth(10, 11); // RX, TX
Servo servoPuerta;

Modo modoActual = APAGAR_TODO;

bool puertaAbierta = false;
bool estadoParpadeo = true;

unsigned long ultimoParpadeo = 0;
unsigned long ultimoBoton = 0;

void setup() {
  pinMode(LED_SALA, OUTPUT);
  pinMode(LED_COMEDOR, OUTPUT);
  pinMode(LED_COCINA, OUTPUT);
  pinMode(LED_BANO, OUTPUT);
  pinMode(LED_HABITACION, OUTPUT);
  pinMode(PIN_VENTILADOR, OUTPUT);

  pinMode(PIN_LED_AZUL, OUTPUT);
  pinMode(PIN_LED_VERDE, OUTPUT);
  pinMode(PIN_LED_ROJO, OUTPUT);
  pinMode(PIN_BOTON_PUERTA, INPUT_PULLUP);

  digitalWrite(PIN_LED_AZUL, HIGH);
  digitalWrite(PIN_LED_VERDE, LOW);
  digitalWrite(PIN_LED_ROJO, LOW);

  Serial.begin(9600);
  bluetooth.begin(9600);

  lcd.init();
  lcd.backlight();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Sistema Listo");
  lcd.setCursor(0, 1);
  lcd.print("LCD I2C OK");
  delay(1500);

  servoPuerta.attach(PIN_SERVO);
  cerrarPuerta();

  cargarDefaults();

  Serial.println("Sistema iniciado");
  bluetooth.println("Sistema iniciado");
  bluetooth.println("1 = Modo Fiesta");
  bluetooth.println("2 = Modo Relajado");
  bluetooth.println("3 = Modo Noche");
  bluetooth.println("4 = Encender Todo");
  bluetooth.println("5 = Apagar Todo");
  bluetooth.println("A = Abrir Puerta");
  bluetooth.println("C = Cerrar Puerta");
  bluetooth.println("E = Estado");

  activarModo(APAGAR_TODO);
}

void loop() {
  manejarBluetooth();
  manejarSerial();
  manejarBotonPuerta();
  manejarParpadeo();
}

void manejarBluetooth() {
  if (bluetooth.available()) {
    char comando = bluetooth.read();
    procesarComando(comando, true);
  }
}

void manejarSerial() {
  if (Serial.available()) {
    char comando = Serial.read();
    procesarComando(comando, false);
  }
}

void procesarComando(char comando, bool desdeBT) {
  Serial.print("Comando recibido: ");
  Serial.println(comando);

  if (desdeBT) {
    bluetooth.print("Comando recibido: ");
    bluetooth.println(comando);
  }

  if (comando == '1') {
    logModo("Modo Fiesta");
    activarModo(FIESTA);
  }

  else if (comando == '2') {
    logModo("Modo Relajado");
    activarModo(RELAJADO);
  }

  else if (comando == '3') {
    logModo("Modo Noche");
    activarModo(NOCHE);
  }

  else if (comando == '4') {
    logModo("Encender Todo");
    activarModo(ENCENDER_TODO);
  }

  else if (comando == '5') {
    logModo("Apagar Todo");
    activarModo(APAGAR_TODO);
  }

  else if (comando == 'A' || comando == 'a') {
    Serial.println("Abriendo puerta");
    bluetooth.println("Abriendo puerta");
    abrirPuerta();
  }

  else if (comando == 'C' || comando == 'c') {
    Serial.println("Cerrando puerta");
    bluetooth.println("Cerrando puerta");
    cerrarPuerta();
  }

  else if (comando == 'E' || comando == 'e') {
    enviarEstado();
  }

  else if (comando == '\n' || comando == '\r') {
    // Ignorar saltos de línea
  }

  else {
    Serial.println("ERROR: Comando invalido");
    bluetooth.println("ERROR: Comando invalido");

    digitalWrite(PIN_LED_ROJO, HIGH);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("ERROR:");
    lcd.setCursor(0, 1);
    lcd.print("Modo invalido");
  }
}

void logModo(String nombre) {
  Serial.print("Activando: ");
  Serial.println(nombre);

  bluetooth.print("Activando: ");
  bluetooth.println(nombre);
}

void activarModo(Modo modo) {
  modoActual = modo;

  EstadoModo estado = leerEstadoModo(modo);

  aplicarEstadosLEDs(estado, false);
  digitalWrite(PIN_VENTILADOR, estado.ventilador ? HIGH : LOW);

  actualizarLCD(modo, estado);

  digitalWrite(PIN_LED_ROJO, LOW);
  parpadearVerde(1);

  Serial.print("Modo aplicado correctamente: ");
  Serial.println(nombreModo(modo));

  bluetooth.print("Modo aplicado correctamente: ");
  bluetooth.println(nombreModo(modo));
}

String nombreModo(Modo modo) {
  switch (modo) {
    case FIESTA: return "Modo Fiesta";
    case RELAJADO: return "Modo Relajado";
    case NOCHE: return "Modo Noche";
    case ENCENDER_TODO: return "Encender Todo";
    case APAGAR_TODO: return "Apagar Todo";
  }

  return "Desconocido";
}

EstadoModo leerEstadoModo(Modo modo) {
  int addr = getEEPROMAddr(modo);

  byte byte1 = EEPROM.read(addr);
  byte byte2 = EEPROM.read(addr + 1);

  EstadoModo estado;

  estado.sala = bitRead(byte1, 0);
  estado.comedor = bitRead(byte1, 1);
  estado.cocina = bitRead(byte1, 2);
  estado.bano = bitRead(byte1, 3);
  estado.habitacion = bitRead(byte1, 4);
  estado.ventilador = bitRead(byte1, 5);
  estado.alternar = bitRead(byte2, 0);

  return estado;
}

void aplicarEstadosLEDs(EstadoModo estado, bool parpadeo) {
  bool on = parpadeo ? estadoParpadeo : true;

  digitalWrite(LED_SALA, estado.sala ? (on ? HIGH : LOW) : LOW);
  digitalWrite(LED_COMEDOR, estado.comedor ? (on ? HIGH : LOW) : LOW);
  digitalWrite(LED_COCINA, estado.cocina ? (on ? HIGH : LOW) : LOW);
  digitalWrite(LED_BANO, estado.bano ? (on ? HIGH : LOW) : LOW);
  digitalWrite(LED_HABITACION, estado.habitacion ? (on ? HIGH : LOW) : LOW);
}

void manejarParpadeo() {
  EstadoModo estado = leerEstadoModo(modoActual);

  if (estado.alternar && millis() - ultimoParpadeo >= 500) {
    ultimoParpadeo = millis();
    estadoParpadeo = !estadoParpadeo;

    aplicarEstadosLEDs(estado, true);

    Serial.println("LEDs alternandose");
  }
}

void actualizarLCD(Modo modo, EstadoModo estado) {
  lcd.clear();

  lcd.setCursor(0, 0);

  switch (modo) {
    case FIESTA:
      lcd.print("Modo: FIESTA");
      break;

    case RELAJADO:
      lcd.print("Modo: RELAJADO");
      break;

    case NOCHE:
      lcd.print("Modo: NOCHE");
      break;

    case ENCENDER_TODO:
      lcd.print("LEDs: ON");
      break;

    case APAGAR_TODO:
      lcd.print("LEDs: OFF");
      break;
  }

  lcd.setCursor(0, 1);

  if (modo == ENCENDER_TODO) {
    lcd.print("Ventilador: ON");
  }

  else if (modo == APAGAR_TODO) {
    lcd.print("Ventilador: OFF");
  }

  else {
    lcd.print("Vent:");

    if (estado.ventilador) {
      lcd.print("ON ");
    } else {
      lcd.print("OFF ");
    }

    if (estado.alternar) {
      lcd.print("LED:ALT");
    } else {
      bool algunLed = estado.sala || estado.comedor || estado.cocina || estado.bano || estado.habitacion;

      if (algunLed) {
        lcd.print("LED:ON");
      } else {
        lcd.print("LED:OFF");
      }
    }
  }
}

void manejarBotonPuerta() {
  if (digitalRead(PIN_BOTON_PUERTA) == LOW && millis() - ultimoBoton > 300) {
    ultimoBoton = millis();

    if (puertaAbierta) {
      Serial.println("Boton presionado: cerrando puerta");
      bluetooth.println("Boton presionado: cerrando puerta");
      cerrarPuerta();
    } else {
      Serial.println("Boton presionado: abriendo puerta");
      bluetooth.println("Boton presionado: abriendo puerta");
      abrirPuerta();
    }
  }
}

void abrirPuerta() {
  servoPuerta.write(90);
  puertaAbierta = true;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Puerta abierta");

  Serial.println("Puerta abierta");
  bluetooth.println("Puerta abierta");
}

void cerrarPuerta() {
  servoPuerta.write(0);
  puertaAbierta = false;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Puerta cerrada");

  Serial.println("Puerta cerrada");
  bluetooth.println("Puerta cerrada");
}

void enviarEstado() {
  EstadoModo estado = leerEstadoModo(modoActual);

  bluetooth.println("----- ESTADO -----");
  bluetooth.print("Modo actual: ");
  bluetooth.println(nombreModo(modoActual));

  bluetooth.print("Sala: ");
  bluetooth.println(estado.sala ? "ON" : "OFF");

  bluetooth.print("Comedor: ");
  bluetooth.println(estado.comedor ? "ON" : "OFF");

  bluetooth.print("Cocina: ");
  bluetooth.println(estado.cocina ? "ON" : "OFF");

  bluetooth.print("Bano: ");
  bluetooth.println(estado.bano ? "ON" : "OFF");

  bluetooth.print("Habitacion: ");
  bluetooth.println(estado.habitacion ? "ON" : "OFF");

  bluetooth.print("Ventilador: ");
  bluetooth.println(estado.ventilador ? "ON" : "OFF");

  bluetooth.print("Puerta: ");
  bluetooth.println(puertaAbierta ? "ABIERTA" : "CERRADA");

  Serial.println("Estado enviado por Bluetooth");
}

void parpadearVerde(int veces) {
  for (int i = 0; i < veces; i++) {
    digitalWrite(PIN_LED_VERDE, HIGH);
    delay(150);
    digitalWrite(PIN_LED_VERDE, LOW);
    delay(150);
  }
}

void guardarEstadoModo(Modo modo, EstadoModo estado) {
  int addr = getEEPROMAddr(modo);

  byte byte1 = 0;
  bitWrite(byte1, 0, estado.sala);
  bitWrite(byte1, 1, estado.comedor);
  bitWrite(byte1, 2, estado.cocina);
  bitWrite(byte1, 3, estado.bano);
  bitWrite(byte1, 4, estado.habitacion);
  bitWrite(byte1, 5, estado.ventilador);

  byte byte2 = 0;
  bitWrite(byte2, 0, estado.alternar);

  EEPROM.write(addr, byte1);
  EEPROM.write(addr + 1, byte2);
}

int getEEPROMAddr(Modo modo) {
  switch (modo) {
    case FIESTA: return EEPROM_FIESTA;
    case RELAJADO: return EEPROM_RELAJADO;
    case NOCHE: return EEPROM_NOCHE;
    case ENCENDER_TODO: return EEPROM_ENCENDER_TODO;
    case APAGAR_TODO: return EEPROM_APAGAR_TODO;
  }

  return 0;
}

void cargarDefaults() {
  guardarEstadoModo(FIESTA, {true, true, true, true, true, true, true});
  guardarEstadoModo(RELAJADO, {false, false, false, false, false, false, false});
  guardarEstadoModo(NOCHE, {false, false, false, false, false, false, false});
  guardarEstadoModo(ENCENDER_TODO, {true, true, true, true, true, true, false});
  guardarEstadoModo(APAGAR_TODO, {false, false, false, false, false, false, false});
}