// ===============================
// PULSE SENSOR FINAL STABLE CODE
// ===============================

const int sensorPin = A0;
const int pulsePin  = 2;

int prevSignal = 0;
int currSignal = 0;
int nextSignal = 0;

unsigned long lastBeatTime = 0;

const int refractoryPeriod = 250;   // กัน detect ซ้ำ (ms)
const int fingerThreshold  = 450;   // ตรวจว่ามีนิ้วหรือไม่


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

  Serial.println("Pulse Sensor Ready...");
}


// ================= LOOP =================
void loop() {

  // shift window (สำหรับ peak detection)
  prevSignal = currSignal;
  currSignal = nextSignal;
  nextSignal = readSmooth();

  unsigned long now = millis();

  // ===== Finger Detection =====
  if (currSignal < fingerThreshold) {
    digitalWrite(pulsePin, LOW);
    return;   // ไม่มีนิ้ว → ไม่คำนวณ BPM
  }

  // ===== TRUE PEAK DETECTION =====
  if (currSignal > prevSignal &&
      currSignal > nextSignal &&
      (currSignal - prevSignal) > 3 &&     // กัน noise เล็ก
      (now - lastBeatTime > refractoryPeriod)) {

      unsigned long interval = now - lastBeatTime;

      // ช่วง BPM ของมนุษย์จริง
      if (interval > 350 && interval < 1500) {

        int bpm = 60000 / interval;

        Serial.print("BPM: ");
        Serial.println(bpm);

        // digital pulse output
        digitalWrite(pulsePin, HIGH);
        delay(20);
        digitalWrite(pulsePin, LOW);
      }

      lastBeatTime = now;
  }

  delay(5);
}