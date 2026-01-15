#define IN1 13
#define IN2 12

void setup() {
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);

 
}

void loop() {
  // Pump ON (forward direction)
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);



}
