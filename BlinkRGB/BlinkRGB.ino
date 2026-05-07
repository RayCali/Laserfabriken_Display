#include <esp_now.h>
#include <WiFi.h>

#define RGB_BRIGHTNESS 255 // Change white brightness (max 255)
#define RGB_BUILTIN 10

typedef struct struct_message {
    byte power;
    byte red;
    byte green;
    byte blue;
} struct_message;

struct_message myData;

void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {
  memcpy(&myData, incomingData, sizeof(myData));
  Serial.printf("Received: power=%d r=%d g=%d b=%d\n", myData.power, myData.red, myData.green, myData.blue);
  neopixelWrite(RGB_BUILTIN, myData.green*(myData.power/100.00), myData.red*(myData.power/100.00), myData.blue*(myData.power/100.00));
}


void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Booting...");

  WiFi.mode(WIFI_STA);
  Serial.print("MAC: ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  Serial.println("ESP-NOW init OK");

  esp_now_register_recv_cb(OnDataRecv);
  Serial.println("Waiting for data...");
}

// the loop function runs over and over again forever
void loop() {

}
