#include "platter_ui.h"
#include "uart_protocol.h"

#include "lvgl.h"
#include <stdio.h>

/* LVGL labels from SquareLine Studio */
extern lv_obj_t *ui_labelPlatterHz;
extern lv_obj_t *ui_labelPlatterRPM;
extern lv_obj_t *ui_labelFrameCount;
extern lv_obj_t *ui_labelSysInit;
extern lv_obj_t *ui_labelFrameT;
extern lv_obj_t *ui_labelFrameOnT;
extern lv_obj_t *ui_labelPotValue;

static void platter_ui_timer_cb(lv_timer_t *timer)
{
    uint32_t period_ms = g_system_rx.platterRotationPeriod_ms;
    uint8_t sliceCount = g_system_rx.sliceCount;
    uint32_t frameTime_us;
    frameTime_us = (period_ms * 1000)/g_system_rx.sliceCount;

    if (period_ms == 0)
    {
        lv_label_set_text(ui_labelPlatterHz,  "--.- Hz");
        lv_label_set_text(ui_labelPlatterRPM, "-- RPM");
        return;
    }

    float hz  = 1000.0f / (float)period_ms;
    float rpm = hz * 60.0f;

    char buf[32];
    snprintf(buf, sizeof(buf), "%.2f Hz", hz);
    lv_label_set_text(ui_labelPlatterHz, buf);

    snprintf(buf, sizeof(buf), "%.0f RPM", rpm);
    lv_label_set_text(ui_labelPlatterRPM, buf);

    //update the frame count
    snprintf(buf, sizeof(buf), "%u", g_system_rx.sliceCount);
    lv_label_set_text(ui_labelFrameCount, buf);

    //update frame time
    snprintf(buf, sizeof(buf), "%u", frameTime_us);
    lv_label_set_text(ui_labelFrameT, buf);

    //update pot value
    //snprintf(buf, sizeof(buf), "%u", g_system_rx.motorSpeedPot);
    //lv_label_set_text(ui_labelPotValue, buf);
    ui_pot_value_update();

    //system initialized
    ui_sys_init_update();

    //Frame ON T
    lv_label_set_text(ui_labelFrameOnT, "1 sys cycle");


}

void platter_ui_init(void)
{
    lv_timer_create(platter_ui_timer_cb, 200, NULL);
}





//separate out updating ui code into individual functions
//in order that data is received

//use syste initialized data
void ui_sys_init_update(void){
    //test - update initialized value
    snprintf(buf, sizeof(buf), "%u", g_system_rx.initialized);
    lv_label_set_text(ui_labelSysInit, buf);

    /*
    if(g_system_rx.initialized == 1){
        lv_label_set_text(ui_labelSysInit, "YES");
        lv_obj_set_style_text_color(ui_labelSysInit, lv_color_make(21,139,37), 0); // green
    }else{
        lv_label_set_text(ui_labelSysInit, "NO");
        lv_obj_set_style_text_color(ui_labelSysInit, lv_color_make(155,26,26), 0); // red
    }
    */
}



//update potentiometer value
void ui_pot_value_update(void){
    char buf[32];
    snprintf(buf, sizeof(buf), "%u", g_system_rx.motorSpeedPot);
    lv_label_set_text(ui_labelPotValue, buf);
}