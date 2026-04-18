const int clockPin = 9;

void setup() {
  pinMode(clockPin, OUTPUT);
  digitalWrite(clockPin, LOW);
}

void loop() {
  digitalWrite(clockPin, HIGH);
  delay(50);   // pulso corto (flanco limpio)
  digitalWrite(clockPin, LOW);
  delay(500);  // tiempo entre conteos
}