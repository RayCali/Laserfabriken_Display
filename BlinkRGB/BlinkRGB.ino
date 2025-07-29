#include <esp_now.h>
#include <WiFi.h>

#define RGB_BRIGHTNESS 255 // Change white brightness (max 255)
#define RGB_BUILTIN 21

typedef struct struct_message {
    byte power;
    byte red;
    byte green;
    byte blue;
} struct_message;

struct_message myData;

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&myData, incomingData, sizeof(myData));
  neopixelWrite(RGB_BUILTIN,myData.green*(myData.power/100.00),myData.red*(myData.power/100.00),myData.blue*(myData.power/100.00));
}


void setup() {
   WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));
}

// the loop function runs over and over again forever
void loop() {

}
