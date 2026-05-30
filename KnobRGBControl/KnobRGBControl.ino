#include "lcd_bsp.h"
#include "cst816.h"
#include "lcd_bl_pwm_bsp.h"
#include "lcd_config.h"
#include "ui.h"
#include "bidi_switch_knob.h"

#include <esp_now.h>
#include <WiFi.h>

uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

#include "../laser_protocol.h"

struct_message myData;
esp_now_peer_info_t peerInfo;

void OnDataSent(const wifi_tx_info_t *mac_addr, esp_now_send_status_t status) {
  // silent — send confirmation not needed in normal operation
}

static const char *TAG = "encoder";

static lv_obj_t * meter;
lv_meter_indicator_t * needle;

static lv_obj_t * meter2;
lv_meter_indicator_t * needle2;

static lv_obj_t * meter3;
lv_meter_indicator_t * needle3;

static lv_obj_t * ui_laserBtn = NULL;

#define EXAMPLE_ENCODER_ECA_PIN    2
#define EXAMPLE_ENCODER_ECB_PIN    1

#define SET_BIT(reg,bit)   (reg |= ((uint32_t)0x01<<bit))
#define CLEAR_BIT(reg,bit) (reg &= (~((uint32_t)0x01<<bit)))
#define READ_BIT(reg,bit)  (((uint32_t)reg>>bit) & 0x01)
#define BIT_EVEN_ALL (0x00ffffff)

EventGroupHandle_t knob_even_ = NULL;
static knob_handle_t s_knob = 0;
SemaphoreHandle_t mutex;

// value[0] = power (%, 0–100)
// value[1] = laser wavelength (nm, 1540.00–1560.00)
// value[2] = crystal wavelength (nm, 1540.00–1560.00)
float value[3] = {0.0f, 1550.0f, 1550.0f};
bool  laser_on  = false;

float       step[3]   = {1.0f,   0.1f,   0.1f};
const float minVal[3] = {0.0f,  1540.0f, 1540.0f};
const float maxVal[3] = {100.0f, 1560.0f, 1560.0f};

static int         wl_step_idx = 2; // 0=10, 1=1, 2=0.1, 3=0.01
static const float wl_steps[]  = {10.0f, 1.0f, 0.1f, 0.01f};
static lv_obj_t  * ui_wlStepLabel = NULL;

static int         lw_step_idx = 1; // 0=1, 1=0.1, 2=0.01
static const float lw_steps[]  = {1.0f, 0.1f, 0.01f};
static lv_obj_t  * ui_lwStepLabel = NULL;

int chosen = 0;

// ── Meter init ────────────────────────────────────────────────────────────────

void lv_example_meter_1(void)
{
    extern lv_obj_t *ui_Screen1;
    meter = lv_meter_create(ui_Screen1);
    lv_obj_add_event_cb(meter, meter_event_cb, LV_EVENT_ALL, NULL);
    lv_obj_center(meter);
    lv_obj_set_size(meter, 200, 200);
    lv_obj_set_pos(meter, -71, 0);
    lv_obj_set_style_bg_opa(meter, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(meter, 0, LV_PART_MAIN);

    lv_meter_scale_t * scale = lv_meter_add_scale(meter);
    lv_meter_set_scale_range(meter, scale, 0, 100, 270, 135);
    lv_meter_set_scale_ticks(meter, scale, 21, 3, 10, lv_palette_main(LV_PALETTE_GREY));
    lv_meter_set_scale_major_ticks(meter, scale, 5, 5, 15, lv_color_white(), 10);
    lv_obj_set_style_text_font(meter, &lv_font_montserrat_10, LV_PART_TICKS | LV_STATE_DEFAULT);

    lv_meter_indicator_t * indic;
    indic = lv_meter_add_arc(meter, scale, 3, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_meter_set_indicator_start_value(meter, indic, 0);
    lv_meter_set_indicator_end_value(meter, indic, 10);
    indic = lv_meter_add_scale_lines(meter, scale, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_BLUE), false, 0);
    lv_meter_set_indicator_start_value(meter, indic, 0);
    lv_meter_set_indicator_end_value(meter, indic, 10);
    indic = lv_meter_add_arc(meter, scale, 3, lv_palette_main(LV_PALETTE_RED), 0);
    lv_meter_set_indicator_start_value(meter, indic, 90);
    lv_meter_set_indicator_end_value(meter, indic, 100);
    indic = lv_meter_add_scale_lines(meter, scale, lv_palette_main(LV_PALETTE_RED), lv_palette_main(LV_PALETTE_RED), false, 0);
    lv_meter_set_indicator_start_value(meter, indic, 90);
    lv_meter_set_indicator_end_value(meter, indic, 100);

    needle = lv_meter_add_needle_line(meter, scale, 4, lv_color_hex(0xFFFFFF), -10);

    lv_arc_set_range(ui_Arc1, 0, 100);
}

void lv_example_meter_2(void)
{
    extern lv_obj_t *ui_Screen1;
    meter2 = lv_meter_create(ui_Screen1);
    lv_obj_add_event_cb(meter2, meter2_event_cb, LV_EVENT_ALL, NULL);
    lv_obj_center(meter2);
    lv_obj_set_size(meter2, 146, 146);
    lv_obj_set_pos(meter2, 94, 15);
    lv_obj_set_style_bg_opa(meter2, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(meter2, 0, LV_PART_MAIN);

    lv_meter_scale_t * scale = lv_meter_add_scale(meter2);
    lv_meter_set_scale_range(meter2, scale, 15400, 15600, 270, 135);
    lv_meter_set_scale_ticks(meter2, scale, 41, 2, 8, lv_palette_main(LV_PALETTE_GREY));
    lv_meter_set_scale_major_ticks(meter2, scale, 10, 3, 11, lv_color_white(), 10);
    lv_obj_set_style_text_font(meter2, &lv_font_montserrat_10, LV_PART_TICKS | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(meter2, meter2_label_cb, LV_EVENT_ALL, NULL);

    needle2 = lv_meter_add_needle_line(meter2, scale, 3, lv_color_hex(0xFFFFFF), -10);

    lv_arc_set_range(ui_Arc2, 15400, 15600);
}

void lv_example_meter_3(void)
{
    extern lv_obj_t *ui_Screen1;
    meter3 = lv_meter_create(ui_Screen1);
    lv_obj_add_event_cb(meter3, meter3_event_cb, LV_EVENT_ALL, NULL);
    lv_obj_center(meter3);
    lv_obj_set_size(meter3, 128, 128);
    lv_obj_set_pos(meter3, 43, -107);
    lv_obj_set_style_bg_opa(meter3, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(meter3, 0, LV_PART_MAIN);

    lv_meter_scale_t * scale = lv_meter_add_scale(meter3);
    lv_meter_set_scale_range(meter3, scale, 1540, 1560, 270, 135);
    lv_meter_set_scale_ticks(meter3, scale, 21, 1, 4, lv_palette_main(LV_PALETTE_GREY));
    lv_meter_set_scale_major_ticks(meter3, scale, 5, 2, 8, lv_color_white(), 10);
    lv_obj_set_style_text_font(meter3, &lv_font_montserrat_10, LV_PART_TICKS | LV_STATE_DEFAULT);

    needle3 = lv_meter_add_needle_line(meter3, scale, 3, lv_color_hex(0xFFFFFF), -10);

    lv_arc_set_range(ui_Arc3, 1540, 1560);
}

static void meter2_label_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_DRAW_PART_BEGIN) return;
    lv_obj_draw_part_dsc_t *dsc = lv_event_get_draw_part_dsc(e);
    if (dsc->part != LV_PART_TICKS) return;
    if (dsc->type != LV_METER_DRAW_PART_TICK) return;
    if (dsc->text == NULL) return;
    static char buf[16];
    lv_snprintf(buf, sizeof(buf), "%" LV_PRId32, dsc->value / 10);
    dsc->text = buf;
}

void lv_create_laser_wl_step_ui(void)
{
    extern lv_obj_t *ui_Screen1;
    ui_lwStepLabel = lv_label_create(ui_Screen1);
    lv_obj_set_width(ui_lwStepLabel, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_lwStepLabel, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_lwStepLabel, 94);
    lv_obj_set_y(ui_lwStepLabel, 92);
    lv_obj_set_align(ui_lwStepLabel, LV_ALIGN_CENTER);
    lv_label_set_text(ui_lwStepLabel, "step:0.10");
    lv_obj_set_style_text_color(ui_lwStepLabel, lv_color_hex(0x808080), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_lwStepLabel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_lwStepLabel, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
}

void lv_create_wavelength_step_ui(void)
{
    extern lv_obj_t *ui_Screen1;
    ui_wlStepLabel = lv_label_create(ui_Screen1);
    lv_obj_set_width(ui_wlStepLabel, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_wlStepLabel, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_wlStepLabel, 43);
    lv_obj_set_y(ui_wlStepLabel, -44);
    lv_obj_set_align(ui_wlStepLabel, LV_ALIGN_CENTER);
    lv_label_set_text(ui_wlStepLabel, "step:0.10");
    lv_obj_set_style_text_color(ui_wlStepLabel, lv_color_hex(0x808080), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_wlStepLabel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_wlStepLabel, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
}

// ── Laser on/off button ───────────────────────────────────────────────────────

static void send_espnow(void)
{
    myData.power_pct          = (uint8_t)value[0];
    myData.laser_wavelength_nm    = value[1];
    myData.crystal_wavelength_nm = value[2];
    myData.laser_on          = laser_on;
    esp_now_send(broadcastAddress, (uint8_t *)&myData, sizeof(myData));
}

void laser_btn_event_cb(lv_event_t * e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        if (xSemaphoreTake(mutex, portMAX_DELAY)) {
            laser_on = !laser_on;
            lv_obj_t * lbl = lv_obj_get_child(ui_laserBtn, 0);
            if (laser_on) {
                lv_label_set_text(lbl, "ON");
                lv_obj_set_style_bg_color(ui_laserBtn, lv_color_hex(0x006600), LV_PART_MAIN | LV_STATE_DEFAULT);
            } else {
                lv_label_set_text(lbl, "OFF");
                lv_obj_set_style_bg_color(ui_laserBtn, lv_color_hex(0x880000), LV_PART_MAIN | LV_STATE_DEFAULT);
            }
            send_espnow();
            xSemaphoreGive(mutex);
        }
    }
}

void lv_create_laser_button(void)
{
    extern lv_obj_t *ui_Screen1;
    ui_laserBtn = lv_btn_create(ui_Screen1);
    lv_obj_center(ui_laserBtn);
    lv_obj_set_size(ui_laserBtn, 85, 38);
    lv_obj_set_pos(ui_laserBtn, 43, 122);
    lv_obj_set_style_bg_color(ui_laserBtn, lv_color_hex(0x880000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_laserBtn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(ui_laserBtn, laser_btn_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t * lbl = lv_label_create(ui_laserBtn);
    lv_label_set_text(lbl, "OFF");
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_center(lbl);
}

// ── Knob callbacks ────────────────────────────────────────────────────────────

static void _knob_left_cb(void *arg, void *data)
{
    uint8_t eventBits_ = 0;
    SET_BIT(eventBits_, 0);
    xEventGroupSetBits(knob_even_, eventBits_);
}
static void _knob_right_cb(void *arg, void *data)
{
    uint8_t eventBits_ = 0;
    SET_BIT(eventBits_, 1);
    xEventGroupSetBits(knob_even_, eventBits_);
}

// ── Setup ─────────────────────────────────────────────────────────────────────

void setup()
{
    mutex = xSemaphoreCreateMutex();
    Serial.begin(115200);
    Touch_Init();
    lcd_lvgl_Init();
    lv_example_meter_1();
    lv_example_meter_2();
    lv_example_meter_3();
    lv_create_laser_wl_step_ui();
    lv_create_wavelength_step_ui();
    lv_create_laser_button();

    set_active_meter(chosen);
    lcd_bl_pwm_bsp_init(40);

    knob_even_ = xEventGroupCreate();
    knob_config_t cfg = {
        .gpio_encoder_a = EXAMPLE_ENCODER_ECA_PIN,
        .gpio_encoder_b = EXAMPLE_ENCODER_ECB_PIN,
    };
    s_knob = iot_knob_create(&cfg);
    iot_knob_register_cb(s_knob, KNOB_LEFT,  _knob_right_cb, NULL);
    iot_knob_register_cb(s_knob, KNOB_RIGHT, _knob_left_cb,  NULL);

    xTaskCreate(user_encoder_loop_task, "user_encoder_loop_task", 3000, NULL, 2, NULL);
    xTaskCreate(example_lvgl_port_task, "LVGL", EXAMPLE_LVGL_TASK_STACK_SIZE, NULL, EXAMPLE_LVGL_TASK_PRIORITY, NULL);

    WiFi.mode(WIFI_STA);
    if (esp_now_init() != ESP_OK) { Serial.println("ESP-NOW init failed"); return; }
    esp_now_register_send_cb(OnDataSent);
    memcpy(peerInfo.peer_addr, broadcastAddress, 6);
    peerInfo.channel  = 0;
    peerInfo.encrypt  = false;
    if (esp_now_add_peer(&peerInfo) != ESP_OK) { Serial.println("Failed to add peer"); return; }
}

// ── Encoder task ──────────────────────────────────────────────────────────────

static void user_encoder_loop_task(void *arg)
{
    for (;;) {
        EventBits_t even = xEventGroupWaitBits(knob_even_, BIT_EVEN_ALL, pdTRUE, pdFALSE, pdMS_TO_TICKS(5000));

        if (READ_BIT(even, 0)) {
            if (xSemaphoreTake(mutex, portMAX_DELAY)) {
                value[chosen] -= step[chosen];
                if (value[chosen] < minVal[chosen]) value[chosen] = minVal[chosen];
                send_espnow();
                xSemaphoreGive(mutex);
            }
        }
        if (READ_BIT(even, 1)) {
            if (xSemaphoreTake(mutex, portMAX_DELAY)) {
                value[chosen] += step[chosen];
                if (value[chosen] > maxVal[chosen]) value[chosen] = maxVal[chosen];
                send_espnow();
                xSemaphoreGive(mutex);
            }
        }
    }
}

// ── LVGL render task ──────────────────────────────────────────────────────────

static void example_lvgl_port_task(void *arg)
{
    float prev[3] = {-1.0f, -1.0f, -1.0f};

    for (;;) {
        lv_timer_handler();

        if (xSemaphoreTake(mutex, portMAX_DELAY)) {
            char buf[16];

            if (value[0] != prev[0]) {
                prev[0] = value[0];
                snprintf(buf, sizeof(buf), "%d%%", (int)value[0]);
                lv_label_set_text(ui_power, buf);
                lv_meter_set_indicator_value(meter, needle, (int)value[0]);
                lv_arc_set_value(ui_Arc1, (int)value[0]);
            }

            if (value[1] != prev[1]) {
                prev[1] = value[1];
                snprintf(buf, sizeof(buf), "%.2f", value[1]);
                lv_label_set_text(ui_power1, buf);
                lv_meter_set_indicator_value(meter2, needle2, (int)(value[1] * 10.0f));
                lv_arc_set_value(ui_Arc2, (int)(value[1] * 10.0f));
            }

            if (value[2] != prev[2]) {
                prev[2] = value[2];
                snprintf(buf, sizeof(buf), "%.2f", value[2]);
                lv_label_set_text(ui_power3, buf);
                lv_meter_set_indicator_value(meter3, needle3, (int)value[2]);
                lv_arc_set_value(ui_Arc3, (int)value[2]);
            }

            xSemaphoreGive(mutex);
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

// ── Meter touch callbacks ─────────────────────────────────────────────────────

void meter_event_cb(lv_event_t * e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) { chosen = 0; set_active_meter(chosen); }
}
void meter2_event_cb(lv_event_t * e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        if (chosen == 1) {
            if (xSemaphoreTake(mutex, portMAX_DELAY)) {
                lw_step_idx = (lw_step_idx + 1) % 3;
                step[1] = lw_steps[lw_step_idx];
                xSemaphoreGive(mutex);
            }
            char buf[16];
            snprintf(buf, sizeof(buf), "step:%.2f", lw_steps[lw_step_idx]);
            lv_label_set_text(ui_lwStepLabel, buf);
        } else {
            chosen = 1;
            set_active_meter(chosen);
        }
    }
}
void meter3_event_cb(lv_event_t * e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        if (chosen == 2) {
            if (xSemaphoreTake(mutex, portMAX_DELAY)) {
                wl_step_idx = (wl_step_idx + 1) % 4;
                step[2] = wl_steps[wl_step_idx];
                xSemaphoreGive(mutex);
            }
            char buf[16];
            snprintf(buf, sizeof(buf), "step:%.2f", wl_steps[wl_step_idx]);
            lv_label_set_text(ui_wlStepLabel, buf);
        } else {
            chosen = 2;
            set_active_meter(chosen);
        }
    }
}

void set_active_meter(int index)
{
    lv_obj_set_style_border_width(meter,  0, LV_PART_MAIN);
    lv_obj_set_style_border_width(meter2, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(meter3, 0, LV_PART_MAIN);
    switch (index) {
        case 0:
            lv_obj_set_style_border_width(meter, 2, LV_PART_MAIN);
            lv_obj_set_style_border_color(meter, lv_color_hex(0x574509), LV_PART_MAIN);
            break;
        case 1:
            lv_obj_set_style_border_width(meter2, 2, LV_PART_MAIN);
            lv_obj_set_style_border_color(meter2, lv_color_hex(0x574509), LV_PART_MAIN);
            break;
        case 2:
            lv_obj_set_style_border_width(meter3, 2, LV_PART_MAIN);
            lv_obj_set_style_border_color(meter3, lv_color_hex(0x574509), LV_PART_MAIN);
            break;
    }
}

void loop() {}
