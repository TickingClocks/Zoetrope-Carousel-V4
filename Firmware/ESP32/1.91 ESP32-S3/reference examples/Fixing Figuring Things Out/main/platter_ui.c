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
extern lv_obj_t *ui_labelState;
extern lv_obj_t *ui_labelRotPeriod;
extern lv_obj_t *ui_labelMotorEnabled;

static void platter_ui_timer_cb(lv_timer_t *timer){

    //update platter speed values
    ui_update_platter_speed_values();

    //update the frame count
    ui_update_frame_count();

    //update pot value
    ui_pot_value_update();

    //system initialized
    ui_sys_init_update();

    //system state
    ui_system_state_update();

    //Frame ON T
    ui_frame_on_time_update();

    //motor enabled
    //THIS ONE IS NOT WORKING - VALUE NOT CHANGING FROM 0
    ui_update_motor_enabled();

}

void platter_ui_init(void){
    lv_timer_create(platter_ui_timer_cb, 200, NULL);
}

//separate out updating ui code into individual functions
//in order that data is received

//THIS VALUE IS BROKEN FOR SOME REASON - I SUSPECT THE STM32 SIDE
//use syste initialized data
void ui_sys_init_update(void){
    char buf[32];

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

//update system state value
void ui_system_state_update(void){
    char buf[32];
    snprintf(buf, sizeof(buf), "%u", g_system_rx.system_state);
    lv_label_set_text(ui_labelState, buf);
}

//update frame count value
void ui_update_frame_count(void){
    char buf[32];
    snprintf(buf, sizeof(buf), "%u", g_system_rx.sliceCount);
    lv_label_set_text(ui_labelFrameCount, buf);
}

//motor enabled value
void ui_update_motor_enabled(void){
    char buf[32];
    snprintf(buf, sizeof(buf), "%u", g_system_rx.motorEnabled);
    lv_label_set_text(ui_labelMotorEnabled, buf);

    //if(g_system_rx.motorEnabled == 1){
      //  lv_label_set_text(ui_labelMotorEnabled, "YES");
        //lv_obj_set_style_text_color(ui_labelMotorEnabled, lv_color_make(21,139,37), 0); // green
    //}else{
      //  lv_label_set_text(ui_labelMotorEnabled, "NO");
        //lv_obj_set_style_text_color(ui_labelMotorEnabled, lv_color_make(155,26,26), 0); // red
    //}
}

//calculate and update speed variables
void ui_update_platter_speed_values(void){
    
    //there is probably a clock issue somewhere in the STM32 code. The speed values being sent are actually off by a factor of two. 
    //This has caused the displayed speed to be half of what it should be.
    //Until that is resolved, we can just double the period here to get correct speed readout.
    char buf[32];
    uint32_t period_ms = g_system_rx.platterRotationPeriod_ms;
    
    //compensate for clock issue by halving period
    period_ms = period_ms / 2; //*** TEMPORARY FIX ***/

    uint8_t sliceCount = g_system_rx.sliceCount;
    uint32_t frameTime_us;
    frameTime_us = (period_ms * 1000)/g_system_rx.sliceCount;

    if (period_ms == 0)
    {
        lv_label_set_text(ui_labelPlatterHz,  "--.-");
        lv_label_set_text(ui_labelPlatterRPM, "--");
        return;
    }

    float hz  = 1000.0f / (float)period_ms;
    float rpm = hz * 60.0f;

    //platter rotation period
    snprintf(buf, sizeof(buf), "%u", period_ms);
    lv_label_set_text(ui_labelRotPeriod, buf);

    //hz
    snprintf(buf, sizeof(buf), "%.2f", hz);
    lv_label_set_text(ui_labelPlatterHz, buf);

    //rpm
    snprintf(buf, sizeof(buf), "%.0f", rpm);
    lv_label_set_text(ui_labelPlatterRPM, buf);

    //frame time
    snprintf(buf, sizeof(buf), "%u", frameTime_us);
    lv_label_set_text(ui_labelFrameT, buf);

}

void ui_frame_on_time_update(void){
    char buf[32];
    float frameOnPercent = g_system_rx.strobeCCRValue / 100.0; //this is the right one
    //float frameOnPercent = g_system_rx.strobeCCRValue / 10.0f;

    //float frameOnPercent = 0.1;

    snprintf(buf, sizeof(buf), "%.2f%%", frameOnPercent);
    lv_label_set_text(ui_labelFrameOnT, buf);
    //lv_label_set_text(ui_labelFrameOnT, "1.0%"); old static placeholder
}