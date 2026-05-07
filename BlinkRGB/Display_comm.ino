#include <esp_now.h>
#include <WiFi.h>

typedef struct struct_message {
    float current_A;
    float laser_setpoint_C;
    float crystal_setpoint_C;
    bool  laser_on;
} struct_message;

struct_message myData;

void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {
    memcpy(&myData, incomingData, sizeof(myData));

    // Print CLI commands matching what Laserfabriken_v1 expects over UART
    Serial.printf("set laser.current %.4f\r\n",      myData.current_A);
    Serial.printf("set laser.setpoint %.4f\r\n",     myData.laser_setpoint_C);
    Serial.printf("set crystal.setpoint %.4f\r\n",   myData.crystal_setpoint_C);
    Serial.printf("%s\r\n", myData.laser_on ? "laser on" : "laser off");
}

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("Laserfabriken receiver ready");

    WiFi.mode(WIFI_STA);
    if (esp_now_init() != ESP_OK) {
        Serial.println("ESP-NOW init failed");
        return;
    }
    esp_now_register_recv_cb(OnDataRecv);
    Serial.println("Waiting for commands...");
}

void loop() {}
