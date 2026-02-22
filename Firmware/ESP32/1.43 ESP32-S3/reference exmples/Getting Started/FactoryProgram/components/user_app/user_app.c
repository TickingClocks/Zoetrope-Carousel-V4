#include <stdio.h>
#include "user_app.h"

#include "touch_bsp.h"
#include "pcf85063.h"
#include "qmi8658c.h"
#include "adc_bsp.h"
#include "sd_card_bsp.h"
#include "ble_scan_bsp.h"
#include "esp_wifi_bsp.h"
#include "esp_timer.h"
#include "gpio_bsp.h"
#include "button_bsp.h"
TaskHandle_t pxBleTask = NULL;
TaskHandle_t pxWifiTask = NULL;
EventGroupHandle_t TaskEven;
void esp_wifi_ble_setscan(uint8_t mode);
void user_example(void *udata);
void color_user(void *arg);   //Detect screen black spot task
void esp_wifi_scan_w(void *arg);
void esp_ble_scan_w(void *arg);
static void screen_btn_event_handler (lv_event_t *e);
void lv_clear_list(lv_obj_t *obj,uint8_t value);
void out_time(ClockModule * clock);
void SetTheClock_start(lv_ui *ui);
void example_GPIO_task(void *arg);
void example_KEY_task(void *arg);
extern void setBrightnes(uint8_t brig);
ClockModule clock_iniput;
void user_Click_Event_Init(lv_ui *ui)
{
  lv_obj_add_event_cb(ui->screen_btn_1, screen_btn_event_handler, LV_EVENT_ALL, ui);   
  lv_obj_add_event_cb(ui->screen_btn_2, screen_btn_event_handler, LV_EVENT_ALL, ui);    
  lv_obj_add_event_cb(ui->screen_imgbtn_1, screen_btn_event_handler, LV_EVENT_ALL, ui); 
  lv_obj_add_event_cb(ui->screen_imgbtn_2, screen_btn_event_handler, LV_EVENT_ALL, ui); 
  lv_obj_add_event_cb(ui->screen_btn_5, screen_btn_event_handler, LV_EVENT_ALL, ui);    
  lv_obj_add_event_cb(ui->screen_btn_6, screen_btn_event_handler, LV_EVENT_ALL, ui);    
  lv_obj_add_event_cb(ui->screen_slider_1, screen_btn_event_handler, LV_EVENT_ALL, ui); 
}
void user_task_init(lv_ui *ui)
{
  xTaskCreate(user_example, "user_example", 3000, ui , 2, NULL);
  xTaskCreate(color_user, "color_user", 3000, ui , 2, NULL);
  xTaskCreate(esp_wifi_scan_w, "esp_wifi_scan_w", 3000, ui, 2, &pxWifiTask);
  xTaskCreate(esp_ble_scan_w, "esp_ble_scan_w", 3000, ui, 2, &pxBleTask);
  xTaskCreate(example_GPIO_task, "example_GPIO_task", 3000, ui, 2, NULL); //gpio Test
  xTaskCreate(example_KEY_task, "example_KEY_task", 3000, ui, 2, NULL);   //KEY Test
}
void user_Init(lv_ui *ui)
{
  TaskEven = xEventGroupCreate();
  xEventGroupSetBits( TaskEven,(0x01<<2) );    //wifi
  xEventGroupSetBits( TaskEven,(0x01<<1) );    //ble
  lv_clear_list(ui->screen_list_1,20);
  lv_clear_list(ui->screen_list_2,20);
  esp32_gpio_init();
  button_Init();
  nvs_flash_Init();
  ble_scan_class_init();
  ble_scan_Init();
  //espwifi_Init();
  user_Click_Event_Init(ui);                  
  user_task_init(ui);                          
  clock_iniput.Hours = 6;
  clock_iniput.minutes = 55;
  clock_iniput.seconds = 30;
  out_time(&clock_iniput);
  SetTheClock_start(&guider_ui);               //Start clock
}
void user_app_sd_read(lv_ui *obj)
{
  char sd_values[15] = {0};
  SD_card_Init();
  float sd_value = sd_cadr_get_value();
  if(sd_value)
  {
    sprintf(sd_values,"%.2fG",sd_value);
    lv_label_set_text(obj->screen_label_6, sd_values);
  }
  else
  {
    lv_label_set_text(obj->screen_label_6, "NULL");
  }
}
void example_KEY_task(void *arg)
{
  lv_ui *obj = (lv_ui *)arg;
  for(;;)
  {
    EventBits_t even = xEventGroupWaitBits(key_groups,(0x01<<1) | (0x01<<0),pdTRUE,pdFALSE,1000);
    if( (even >> 0) & 0x01 )
    {
      lv_label_set_text(obj->screen_label_21, "Click event.");
    }
    else if( (even >> 1) & 0x01 )
    {
      lv_label_set_text(obj->screen_label_21, "Double click event.");
    }
    else
    {
      lv_label_set_text(obj->screen_label_21, "No state.");
    }
  }
}
void example_GPIO_task(void *arg)
{
  lv_ui *obj = (lv_ui *)arg;
  for(;;)
  {
    uint8_t _add = 0;
    GPIO_SET(example_test_out1,0);
    GPIO_SET(example_test_out2,0);
    vTaskDelay(pdMS_TO_TICKS(200));
    if(GPIO_GET(example_test_in1)==0 && GPIO_GET(example_test_in2)==0)
    {
      _add++;
    }
    GPIO_SET(example_test_out1,1);
    GPIO_SET(example_test_out2,1);
    vTaskDelay(pdMS_TO_TICKS(200));
    if(GPIO_GET(example_test_in1)==1 && GPIO_GET(example_test_in2)==1)
    {
      _add++;
    }
    GPIO_SET(example_test_out1,1);
    GPIO_SET(example_test_out2,0);
    vTaskDelay(pdMS_TO_TICKS(200));
    if(GPIO_GET(example_test_in1)==1 && GPIO_GET(example_test_in2)==0)
    {
      _add++;
    }
    if(_add == 3)
    {
      //printf("GPIO test passed.\n");
      lv_label_set_text(obj->screen_label_17, "GPIO test passed.");
    }
    else
    {
      //printf("GPIO test failed.\n");
      lv_label_set_text(obj->screen_label_17, "GPIO test failed.");
    }
  }
}
void out_time(ClockModule * clock)
{
  clock->out_Hours = clock->Hours * 5;
  clock->out_minutes = clock->minutes;
  clock->out_seconds = clock->seconds;
  uint8_t bat = clock->out_minutes / 12;
  clock->out_Hours += bat;

  int16_t Hours_ars = clock_iniput.out_Hours * 6 - 90;
  int16_t Minutes_ars = clock_iniput.out_minutes * 6 - 90;
  int16_t Seconds_ars = clock_iniput.out_seconds * 6 - 90;
  lv_img_set_angle(guider_ui.screen_img_4, Hours_ars * 10);
  lv_img_set_angle(guider_ui.screen_img_5, Minutes_ars * 10);
  lv_img_set_angle(guider_ui.screen_img_6, Seconds_ars * 10);
}
static void clock_task_callback(void *arg)
{
  static uint8_t bat = 0;
  lv_ui *ui = (lv_ui *)arg;
  clock_iniput.out_seconds++;
  int16_t Seconds_ars = clock_iniput.out_seconds * 6 - 90;
  lv_img_set_angle(ui->screen_img_6, Seconds_ars * 10);
  if(clock_iniput.out_seconds == 60)
  {
    clock_iniput.out_seconds = 0;
    clock_iniput.out_minutes++;
    int16_t Minutes_ars = clock_iniput.out_minutes * 6 - 90;
    lv_img_set_angle(ui->screen_img_5, Minutes_ars * 10);
    if( (clock_iniput.out_minutes == 12) || (clock_iniput.out_minutes == 24) || (clock_iniput.out_minutes == 36) || (clock_iniput.out_minutes == 48) || (clock_iniput.out_minutes == 60))
    bat = 1;
    else
    bat = 0;
  }
  if(clock_iniput.out_minutes == 60)
  {
    clock_iniput.minutes = 0;
  }
  if( bat == 1 )
  {
    bat = 0;
    clock_iniput.out_Hours++;
    int16_t Hours_ars = clock_iniput.out_Hours * 6 - 90;
    lv_img_set_angle(ui->screen_img_4, Hours_ars * 10);
  }
  if(clock_iniput.out_Hours == 60)
  {
    clock_iniput.out_Hours = 0;
  }
}
void SetTheClock_start(lv_ui *ui)
{
  const esp_timer_create_args_t clock_tick_timer_args = 
  {
    .callback = &clock_task_callback,
    .name = "clock_task",
    .arg = ui,
  };
  esp_timer_handle_t clock_tick_timer = NULL;
  ESP_ERROR_CHECK(esp_timer_create(&clock_tick_timer_args, &clock_tick_timer));
  ESP_ERROR_CHECK(esp_timer_start_periodic(clock_tick_timer, 1000 * 1000));              //1s
}
void user_example(void *udata)
{
  lv_ui *obj = (lv_ui *)udata;
  char rtc_values[20] = {0};
  char qmi_values[30] = {0};
  char adc_values[15] = {0};
  float acc[3] = {0};
  float gyro[3] = {0};
  float adc_value;
  uint32_t stimes = 0;
  uint32_t rtc_test = 0;
  uint32_t qmi_test = 0;
  uint32_t adc_test = 0;
  qmi8658_init();
  PCF85063_set_tim_init();
  adc_bsp_init();
  user_app_sd_read(obj);
  for(;;)
  {
    if(stimes - rtc_test > 4) //5s
    {
      rtc_test = stimes;
      PCF85063_get_tim((uint8_t *)rtc_values);
      lv_label_set_text(obj->screen_label_10, rtc_values);
    }
    if(stimes - qmi_test > 1) //2s
    {
      qmi_test = stimes;
      qmi8658_read_xyz(acc,gyro);
      sprintf(qmi_values,"ax:%.2f ay:%.2f az:%.2f",acc[0],acc[1],acc[2]);
      lv_label_set_text(obj->screen_label_12, qmi_values);
      sprintf(qmi_values,"gx:%.2f gy:%.2f gz:%.2f",gyro[0],gyro[1],gyro[2]);
      lv_label_set_text(obj->screen_label_19, qmi_values);
    }
    if(stimes - adc_test > 1) //2s
    {
      adc_test = stimes;
      adc_get_value(&adc_value,NULL);
      if(adc_value)
      {
        sprintf(adc_values,"%.2fV",adc_value);
        lv_label_set_text(obj->screen_label_7, adc_values);
        //sprintf(adc_values,"%d",(uint16_t)adc_value[1]);
        //lv_label_set_text(obj->screen_label_8, adc_values);  
      }
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
    stimes++;
  }
}

void color_user(void *arg)
{
  lv_ui *ui = (lv_ui *)arg;
  lv_obj_clear_flag(ui->screen_carousel_1,LV_OBJ_FLAG_SCROLLABLE); //unmovable
  lv_obj_clear_flag(ui->screen_cont_2,LV_OBJ_FLAG_HIDDEN); 
  lv_obj_add_flag(ui->screen_cont_1, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(ui->screen_img_1,LV_OBJ_FLAG_HIDDEN); 
  lv_obj_add_flag(ui->screen_img_2, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(ui->screen_img_3, LV_OBJ_FLAG_HIDDEN);
  vTaskDelay(pdMS_TO_TICKS(1500));
  lv_obj_clear_flag(ui->screen_img_2,LV_OBJ_FLAG_HIDDEN); 
  lv_obj_add_flag(ui->screen_img_1, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(ui->screen_img_3, LV_OBJ_FLAG_HIDDEN);
  vTaskDelay(pdMS_TO_TICKS(1500));
  lv_obj_clear_flag(ui->screen_img_3,LV_OBJ_FLAG_HIDDEN); 
  lv_obj_add_flag(ui->screen_img_2, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(ui->screen_img_1, LV_OBJ_FLAG_HIDDEN);
  vTaskDelay(pdMS_TO_TICKS(1500));
  lv_obj_clear_flag(ui->screen_cont_1,LV_OBJ_FLAG_HIDDEN); 
  lv_obj_add_flag(ui->screen_cont_2, LV_OBJ_FLAG_HIDDEN);  
  lv_obj_add_flag(ui->screen_carousel_1,LV_OBJ_FLAG_SCROLLABLE); //removable
  vTaskDelete(NULL); 
}
static void screen_btn_event_handler (lv_event_t *e)
{
  lv_event_code_t code = lv_event_get_code(e);
  lv_ui *ui = (lv_ui *)e->user_data;
  lv_obj_t * module = e->current_target;
  switch (code)
  {
    case LV_EVENT_CLICKED:
    {
      EventBits_t even = xEventGroupWaitBits(TaskEven,(0x01<<1) | (0x01<<2),pdFALSE,pdFALSE,10);
      if(module == ui->screen_btn_1)
      {
        if( even & (0x01<<1) )
        {
          esp_wifi_ble_setscan(0); //ble
          xEventGroupClearBits( TaskEven,(0x01<<1) );
          ble_scan_setconf();
          xTaskNotifyGive(pxBleTask);
        }
      }
      else if(module == ui->screen_btn_6)    //wifi     
      {
        if( even & (0x01<<2) )
        {
          esp_wifi_ble_setscan(1); //wifi

          xEventGroupClearBits( TaskEven,(0x01<<2) );
          if(pxWifiTask != NULL)
          xTaskNotifyGive(pxWifiTask);
          else
          printf("ok\n");
        }
      }
      else if(module == ui->screen_imgbtn_1)
      {
        lv_obj_clear_flag(ui->screen_cont_4,LV_OBJ_FLAG_HIDDEN);  
        lv_obj_add_flag(ui->screen_cont_3, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ui->screen_cont_5, LV_OBJ_FLAG_HIDDEN);
      } 
      else if(module == ui->screen_imgbtn_2)     //ble
      {
        lv_obj_clear_flag(ui->screen_cont_5,LV_OBJ_FLAG_HIDDEN);  
        lv_obj_add_flag(ui->screen_cont_3, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ui->screen_cont_4, LV_OBJ_FLAG_HIDDEN);
      }
      else if( (module == ui->screen_btn_5) || (module == ui->screen_btn_2) )
      {
        lv_obj_clear_flag(ui->screen_cont_3,LV_OBJ_FLAG_HIDDEN);  
        lv_obj_add_flag(ui->screen_cont_5, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ui->screen_cont_4, LV_OBJ_FLAG_HIDDEN);
      }
      else if( module == ui->screen_slider_1 )
      {
        uint8_t value = lv_slider_get_value(module);
        setBrightnes(value);
      }
      break;
    }
    default:
      break;
  }
}
//wifi :1 ble:0
void esp_wifi_ble_setscan(uint8_t mode)
{
  static uint8_t wifi_ble_flag = 0;
  if(mode != wifi_ble_flag)
  {
    if(mode) //wifi Need to release ble
    {
      ble_scan_Deinit();
      espwifi_Init();
    }
    else
    {
      espwifi_Deinit();
      ble_scan_Init();
    }
    wifi_ble_flag = mode;
  }
}
void lv_clear_list(lv_obj_t *obj,uint8_t value) 
{
	for(signed char i = value-1; i>=0; i--)
	{
		lv_obj_t *imte = lv_obj_get_child(obj,i);
		lv_obj_add_flag(imte,LV_OBJ_FLAG_HIDDEN);
		vTaskDelay(pdMS_TO_TICKS(20));
	}
  vTaskDelay(pdMS_TO_TICKS(20));
  lv_obj_invalidate(obj); //Redraw the next cycle
}

void esp_wifi_scan_w(void *arg)
{
  lv_ui *wifi_obj = (lv_ui *)arg;
	static wifi_ap_record_t recdata;
  static uint16_t rec = 0;
	static const char *imgbox = NULL;
	static lv_obj_t *imte;
	static lv_obj_t *label;
  for(;;)
  {
    ulTaskNotifyTake(pdTRUE,portMAX_DELAY);                                      //Wait for task notification
		lv_clear_list(wifi_obj->screen_list_2,rec);
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_scan_start(NULL,true));               //Scan available AP
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_scan_get_ap_num(&rec));
    if(rec != 0)
    {
			if(rec > 19)
			{
				rec = 20;
				for(uint8_t i = 0; i<rec; i++)
      	{
      	  ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_scan_get_ap_record(&recdata));
					imgbox = (const char*)(&(recdata.ssid[0]));
					if (imgbox == NULL)
					break;
					imte = lv_obj_get_child(wifi_obj->screen_list_2,i);
					if (imte != NULL)
					{
						label = lv_obj_get_child(imte,1);
						if(label != NULL)
            {
    					lv_label_set_text(label,imgbox);
							lv_obj_clear_flag(imte,LV_OBJ_FLAG_HIDDEN);        
						}
					}
					imgbox = NULL;
					imte = NULL;
					label = NULL;
					vTaskDelay(pdMS_TO_TICKS(100));
      	}
				esp_wifi_clear_ap_list();
			}
			else
			{
      	for(uint8_t i = 0; i<rec; i++)
      	{
      	  ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_scan_get_ap_record(&recdata));
					imgbox = (const char*)(&(recdata.ssid[0]));
					if (imgbox == NULL)
					break;
					imte = lv_obj_get_child(wifi_obj->screen_list_2,i); //Get subobject
					if (imte != NULL)
					{
						label = lv_obj_get_child(imte,1); //Gets the child subobject
						if(label != NULL)
            {
    					lv_label_set_text(label,imgbox);
							lv_obj_clear_flag(imte,LV_OBJ_FLAG_HIDDEN);    
						}
					}
					imgbox = NULL;
					imte = NULL;
					label = NULL;
					vTaskDelay(pdMS_TO_TICKS(100));
      	}
			}
    }
    xEventGroupSetBits( TaskEven,(0x01<<2) );
  }
}
void esp_ble_scan_w(void *arg)
{
  lv_ui *wifi_obj = (lv_ui *)arg;
  static uint16_t rec = 0;
  uint8_t mac[6];
  static lv_obj_t *imte;
	static lv_obj_t *label;
  char imgbox[24] = {0};
  for(;;)
  {
    ulTaskNotifyTake(pdTRUE,portMAX_DELAY);     
    lv_clear_list(wifi_obj->screen_list_1,rec);
    rec = 0;
    for(;xQueueReceive(ble_Queue,mac,3500) == pdTRUE;)
    {
      imte = lv_obj_get_child(wifi_obj->screen_list_1,rec);
      if (imte != NULL)
			{
				label = lv_obj_get_child(imte,1);
				if(label != NULL)
        {
          sprintf(imgbox,"%d:%d:%d:%d:%d:%d",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
    			lv_label_set_text(label,imgbox);
					lv_obj_clear_flag(imte,LV_OBJ_FLAG_HIDDEN);    
				}
			}
			imte = NULL;
			label = NULL;
      rec++;
      vTaskDelay(pdMS_TO_TICKS(100));
      if(rec == 20)
      break;
    }
    xEventGroupSetBits( TaskEven,(0x01<<1) );
  }
}