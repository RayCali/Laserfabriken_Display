#include <esp_now.h>
#include <WiFi.h>

// Hardware UART → STM32 (TX=GPIO21, RX=GPIO20)
HardwareSerial SerialSTM(0);

#include "../laser_protocol.h"

struct_message myData;

static void send_cmd(const char *fmt, ...) {
    char buf[64];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    Serial.print(buf);       // USB CDC — for debugging
    SerialSTM.print(buf);    // hardware UART → STM32
}

void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {
    memcpy(&myData, incomingData, sizeof(myData));

    send_cmd("set laser.power %d\r\n",          (int)myData.power_pct);
    send_cmd("set crystal.wavelength %.2f\r\n", myData.crystal_wavelength_nm);
    send_cmd("%s\r\n", myData.laser_on ? "laser on" : "laser off");
}

void setup() {
    Serial.begin(115200); // USB CDC for debugging SUPER IMPORTANT TO FLASH WITH USB CDC ON BOOT. OR THIS WILL USE UART0 WHICH IS ALSO USED FOR STM32 COMMUNICATION.
    SerialSTM.begin(115200, SERIAL_8N1, 20, 21);  // RX=GPIO20, TX=GPIO21
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
