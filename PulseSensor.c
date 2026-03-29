const int pulsePin = A0;        
const int digitalOutPin = 2;    

unsigned long lastBeatTime = 0;
int lastValidBPM = 0; 

// ================= ตัวแปรสำหรับ Moving Average =================
int recentBPMs[4] = {0, 0, 0, 0}; 
int recentIndex = 0;
int validCount = 0;

// ================= ตัวแปรสำหรับหาค่าเฉลี่ย 30 วินาที =================
unsigned long startTime = 0;
int bpmValues[100];
int bpmIndex = 0;

// ================= ตัวแปรสำหรับ Filters & Dynamic Threshold (⭐ จุดที่อัปเกรด) =================
float dcOffset = 512.0;    
float filteredSignal = 0;  
float peakValue = 50.0;     // เก็บสถิติความสูงยอดคลื่น (เริ่มต้นที่ 50)
int threshold = 25;         // เส้นตัดเกณฑ์ที่จะปรับเปลี่ยนอัตโนมัติ
boolean wasHigh = false;

// ================= SETUP =================
void setup() {
  Serial.begin(9600);
  pinMode(pulsePin, INPUT); 
  pinMode(digitalOutPin, OUTPUT); 
  
  dcOffset = analogRead(pulsePin);
  
  Serial.println("Pulse Sensor Ready (Ultra Stable + Auto Threshold)...");
  startTime = millis();
}

// ================= LOOP =================
void loop() {
  unsigned long now = millis();
  int sensorValue = analogRead(pulsePin);

  // 1. High-Pass Filter (ปรับให้ตาม baseline ได้กระชับขึ้น)
  dcOffset = (0.95 * dcOffset) + (0.05 * sensorValue); 
  filteredSignal = sensorValue - dcOffset; 

  // 2. ⭐ ระบบปรับ Threshold อัตโนมัติ (Dynamic Threshold)
  // ถ้าเจอคลื่นที่สูงกว่ายอดเดิม ให้จำยอดที่สูงกว่าไว้
  if (filteredSignal > peakValue) {
    peakValue = filteredSignal; 
  }
  // ค่อยๆ สลายค่ายอดคลื่นลงทีละนิด (เผื่อผู้ใช้วางนิ้วเบาลง คลื่นเล็กลง จะได้จับค่าได้ต่อ)
  peakValue = peakValue * 0.998; 
  
  // ตั้งเส้นตัดเกณฑ์ไว้ที่ 50% ของความสูงยอดคลื่นเสมอ
  threshold = peakValue * 0.5;
  if (threshold < 20) threshold = 20; // ป้องกัน Threshold ต่ำเกินไปจนไปจับเอา Noise

  // 3. ตรวจจับจังหวะขาขึ้น (Blanking Time 300ms)
  if (filteredSignal > threshold && !wasHigh && (now - lastBeatTime > 300)) {
    wasHigh = true; 
    
    digitalWrite(digitalOutPin, HIGH); // ⭐ ส่งสัญญาณ Digital กราฟสี่เหลี่ยมขึ้น
    
    unsigned long interval = now - lastBeatTime;

    // กรองขอบเขตช่วง 40 - 160 BPM
    if (interval >= 375 && interval <= 1500) {
      int rawBPM = 60000 / interval; 
      
      // ระบบกรองค่ากระโดด (บีบให้แคบลง ห้ามกระโดดเกิน 20 BPM ต่อจังหวะ)
      if (lastValidBPM == 0 || abs(rawBPM - lastValidBPM) < 20) {
        lastValidBPM = rawBPM; 

        // ระบบ Moving Average (เกลี่ย 4 ค่า)
        recentBPMs[recentIndex] = rawBPM;
        recentIndex = (recentIndex + 1) % 4; 
        if (validCount < 4) validCount++;

        int sum = 0;
        for (int i = 0; i < validCount; i++) {
          sum += recentBPMs[i];
        }
        int smoothedBPM = sum / validCount;

        Serial.print("❤️ BPM: ");
        Serial.println(smoothedBPM);
        
        // Serial.print(threshold); // พิมพ์บอกด้วยว่าตอนนี้ใช้ Threshold เท่าไหร่
        // Serial.println(")");

        if (bpmIndex < 100) {
          bpmValues[bpmIndex] = smoothedBPM;
          bpmIndex++;
        }
      }
    } 
    // ถ้านานเกิน 2.5 วินาที (เอานิ้วออก) ให้รีเซ็ตค่าการจำทั้งหมด
    else if (interval > 2500) {
      lastValidBPM = 0;
      validCount = 0;
      peakValue = 50.0; // รีเซ็ตยอดคลื่น
    }
    
    lastBeatTime = now; 
  } 
  // 4. ⭐ ตรวจจับจังหวะขาลง (ดึงกราฟสี่เหลี่ยมลง)
  // เงื่อนไข: คลื่นตกลงต่ำกว่า 0 หรือผ่านไปแล้ว 150ms (ทำให้กราฟสี่เหลี่ยมบนสโคปมีขนาดกว้างเท่ากันสวยงาม)
  else if (wasHigh && (filteredSignal < 0 || (now - lastBeatTime > 150))) {
    wasHigh = false; 
    digitalWrite(digitalOutPin, LOW); 
  }

  // 5. คำนวณค่าเฉลี่ย 30 วินาที
  if (now - startTime >= 30000) {
    if (bpmIndex > 2) {
      long sum = 0;
      int maxBPM = 0;
      int minBPM = 999;

      for (int i = 0; i < bpmIndex; i++) {
        sum += bpmValues[i];
        if (bpmValues[i] > maxBPM) maxBPM = bpmValues[i];
        if (bpmValues[i] < minBPM) minBPM = bpmValues[i];
      }
      
      sum = sum - maxBPM - minBPM;
      float avgBPM = (float)sum / (bpmIndex - 2);
      
      Serial.println("====================");
      Serial.print("Average BPM (30s) [Trimmed]: ");
      Serial.println(avgBPM);
      Serial.println("====================");
      
    } else if (bpmIndex > 0) {
      long sum = 0;
      for (int i = 0; i < bpmIndex; i++) {
        sum += bpmValues[i];
      }
      float avgBPM = (float)sum / bpmIndex;
      Serial.println("====================");
      Serial.print("Average BPM (30s): ");
      Serial.println(avgBPM);
      Serial.println("====================");
    } else {
      Serial.println("====================");
      Serial.println("Measure not found. Please check your finger placement.");
      Serial.println("====================");
    }
    
    bpmIndex = 0; 
    startTime = now;
    Serial.println("New 30s measurement started...");
  }
  
  delay(5); 
}