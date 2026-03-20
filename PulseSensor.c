const int sensorPin = A0;
const int pulsePin  = 2;
const int analogOutPin = 9;

int prevSignal = 0;
int currSignal = 0;
int nextSignal = 0;

unsigned long lastBeatTime = 0;
unsigned long startTime = 0;

const int refractoryPeriod = 300;
const int fingerThreshold  = 450;
const int minPeakHeight    = 20;

int bpmValues[50];
int bpmIndex = 0;


// ---------- Smooth Read ----------
int readSmooth() {
  long sum = 0;
  for (int i = 0; i < 5; i++) {
    sum += analogRead(sensorPin);
  }
  return sum / 5;
}


// ================= SETUP =================
void setup() {
  Serial.begin(9600);
  pinMode(pulsePin, OUTPUT);
  pinMode(analogOutPin, OUTPUT);

  Serial.println("Pulse Sensor Ready...");
  startTime = millis();
}


// ================= LOOP =================
void loop() {

  prevSignal = currSignal;
  currSignal = nextSignal;
  nextSignal = readSmooth();

  unsigned long now = millis();

  // ⭐ ส่งสัญญาณไป Oscilloscope (กราฟจริง)
  analogWrite(analogOutPin, currSignal / 4);  // 0-1023 → 0-255

  // ===== Finger Detection =====
  if (currSignal < fingerThreshold) {
    digitalWrite(pulsePin, LOW);
    return;
  }

  // ===== TRUE PEAK DETECTION =====
  if (currSignal > prevSignal &&
      currSignal > nextSignal &&
      (currSignal - prevSignal) > minPeakHeight &&
      (now - lastBeatTime > refractoryPeriod)) {

      unsigned long interval = now - lastBeatTime;

      if (interval > 450 && interval < 1400) {

        int bpm = 60000 / interval;

        Serial.print("Instant BPM: ");
        Serial.println(bpm);

        if (bpmIndex < 50) {
          bpmValues[bpmIndex++] = bpm;
        }

        // ⭐ pulse ชัด ๆ สำหรับ oscilloscope
        digitalWrite(pulsePin, HIGH);
        delay(20);
        digitalWrite(pulsePin, LOW);
      }

      lastBeatTime = now;
  }

  // ===== AVERAGE EVERY 20s =====
  if (now - startTime >= 20000) {

    long sum = 0;

    for (int i = 0; i < bpmIndex; i++) {
      sum += bpmValues[i];
    }

    if (bpmIndex > 0) {
      float avgBPM = sum / (float)bpmIndex;

      Serial.println("====================");
      Serial.print("Average BPM (20s): ");
      Serial.println(avgBPM);
      Serial.println("====================");
    } else {
      Serial.println("No beats detected");
    }

    bpmIndex = 0;
    startTime = now;
  }

  delay(5);
}