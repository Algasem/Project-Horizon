#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

const uint64_t pipeOut = 0xABCDABCD71LL;
RF24 radio(9, 10);

struct Signal {
  byte throttle;
  byte pitch;
  byte roll;
  byte yaw;
  byte aux1;
  byte aux2;
};

Signal data;

void ResetData() {
  data.throttle = 0;
  data.pitch    = 127;
  data.roll     = 127;
  data.yaw      = 127;
  data.aux1     = 0;
  data.aux2     = 0;
}

int Border_Map_Cal(int val, int lower, int middle, int upper, bool reverse) {
  val = constrain(val, lower, upper);
  int out;
  if (val < middle)
    out = map(val, lower, middle, 0, 127);
  else
    out = map(val, middle, upper, 127, 255);
  out = constrain(out, 0, 255);
  if (reverse) out = 255 - out;
  return out;
}

void setup() {
  Serial.begin(115200);
  Serial.println("=== PROJECT HORIZON TRANSMITTER ===");

  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);

  ResetData();

  if (!radio.begin()) {
    Serial.println("RADIO FAILED");
    while (1);
  }

  radio.openWritingPipe(pipeOut);
  radio.setChannel(100);
  radio.setAutoAck(false);
  radio.setDataRate(RF24_250KBPS);
  radio.setPALevel(RF24_PA_MAX);
  radio.stopListening();

  Serial.println("OK — A0=Yaw A1=Throttle A2=Pitch A3=Roll");
}

unsigned long lastPrint = 0;
unsigned long packetCount = 0;

void loop() {
  int rawYaw      = analogRead(A1);
  int rawThrottle = analogRead(A0);
  int rawPitch    = analogRead(A2);  // A2 = Pitch
  int rawRoll     = analogRead(A3);  // A3 = Roll

  data.yaw      = Border_Map_Cal(rawYaw,       0, 513,  1023, false);
  data.throttle = Border_Map_Cal(rawThrottle, 154, 515,  848,  false);
  data.pitch    = Border_Map_Cal(rawPitch,     0, 516,  1023, false);
  data.roll     = Border_Map_Cal(rawRoll,      0, 521,  1023, false);

  data.aux1 = !digitalRead(2);
  data.aux2 = !digitalRead(3);
  radio.write(&data, sizeof(Signal));
  packetCount++;

  if (millis() - lastPrint > 500) {
    Serial.println("--- TX STATUS ---");
    Serial.print("Packets: "); Serial.println(packetCount);
    Serial.print("THROTTLE:"); Serial.print(data.throttle);
    Serial.print(" PITCH:");   Serial.print(data.pitch);
    Serial.print(" ROLL:");    Serial.print(data.roll);
    Serial.print(" YAW:");     Serial.println(data.yaw);
    Serial.print("RAW A0(thr):"); Serial.print(rawThrottle);
    Serial.print(" A1(yaw):");    Serial.print(rawYaw);
    Serial.print(" A2(pitch):");  Serial.print(rawPitch);
    Serial.print(" A3(roll):");   Serial.println(rawRoll);
    lastPrint = millis();
  }
}