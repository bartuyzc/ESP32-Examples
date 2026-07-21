#define BTN 27
#define LED 2

void setup() {
  Serial.begin(115200);

  pinMode(LED, OUTPUT);
  pinMode(BTN, INPUT);

}

void loop() {

  if (digitalRead(BTN) == LOW) {
      digitalWrite(LED, HIGH);
      Serial.println("Button Pressed"); 
    } 
  else {
      digitalWrite(LED, LOW);
      Serial.println("Button Released");  
    }
}
