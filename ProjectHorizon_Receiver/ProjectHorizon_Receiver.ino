#include <SPI.h>
#include <RF24.h>
#include <Servo.h>

RF24 radio(4, 2); // CE=D4, CSN=D2

const byte address[6] = "00001";

struct Signal {
  int throttle;
  int yaw;
  int pitch;
  int roll;
  int aux1;
  int aux2;
};
Signal data;

Servo ch1, ch2, ch3, ch4, ch5, ch6;
unsigned long lastSignalTime = 0;

void ResetData() {
  data.throttle = 1000;
  data.yaw = 1500;
  data.pitch = 1500;
  data.roll = 1500;
  data.aux1 = 1000;
  data.aux2 = 1000;
}

void setup() {
  Serial.begin(115200);
  Serial.println("BOOT");        // confirms setup() is running
  ResetData();

  ch1.attach(5);
  ch2.attach(6);
  ch3.attach(7);
  ch4.attach(8);
  ch5.attach(9);
  ch6.attach(10);

  if (!radio.begin()) {
    Serial.println("ERROR: Radio not responding!");
  }

  radio.openReadingPipe(1, address);
  radio.setChannel(108);
  radio.setDataRate(RF24_250KBPS);
  radio.setPALevel(RF24_PA_MAX);
  radio.setAutoAck(true);
  radio.startListening();

  Serial.println("Receiver ready");
}

void loop() {
  if (radio.available()) {
    radio.read(&data, sizeof(Signal));
    lastSignalTime = millis();
  }

  if (millis() - lastSignalTime > 500) {
    ResetData();
  }

  ch1.writeMicroseconds(data.roll);
  ch2.writeMicroseconds(data.pitch);
  ch3.writeMicroseconds(data.throttle);
  ch4.writeMicroseconds(data.yaw);
  ch5.writeMicroseconds(data.aux1);
  ch6.writeMicroseconds(data.aux2);

  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 500) {
    Serial.print("Chip:"); Serial.print(radio.isChipConnected());
    if (millis() - lastSignalTime > 500) {
      Serial.print(" [SIGNAL LOST]");
    } else {
      Serial.print(" [LINK OK]");
    }
    Serial.print(" THR:"); Serial.print(data.throttle);
    Serial.print(" ROLL:"); Serial.print(data.roll);
    Serial.print(" PITCH:"); Serial.print(data.pitch);
    Serial.print(" YAW:"); Serial.println(data.yaw);
    lastPrint = millis();
  }
}

