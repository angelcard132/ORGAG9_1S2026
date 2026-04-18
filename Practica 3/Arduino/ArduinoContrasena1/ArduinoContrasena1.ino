// Cable conectado al pin A0
int errorPulse = A0;

void setup() {
  pinMode(errorPulse, INPUT);
  Serial.begin(9600);
}

void loop() {
  if (digitalRead(errorPulse) == HIGH) {
    Serial.println("ERROR DE CONTRASEÑA DETECTADO");
    delay(300); // antirebote simple
  }
}