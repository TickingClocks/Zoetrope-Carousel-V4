#include "button_bsp.h"
#include "multi_button.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "gpio_bsp.h"
EventGroupHandle_t key_groups;
struct Button button1; //Request button
#define button1_id 0   //button1_id
#define button1_active 0 //active level
static void clock_task_callback(void *arg)
{
  button_ticks();              //Status callback
}
uint8_t read_button_GPIO(uint8_t button_id)   //Return level
{
	switch(button_id)
	{
		case button1_id:
      return GPIO_GET(example_key);
			break;
		default:
			break;
	}
  return 0;
}
void Button_SINGLE_CLICK_Callback(void* btn) //Click event
{
  struct Button *user_button = (struct Button *)btn;
	if(user_button == &button1)
  {
    xEventGroupSetBits( key_groups,(0x01<<0) ); 
  }
}

void Button_DOUBLE_CLICK_Callback(void* btn) //Double click event
{
  struct Button *user_button = (struct Button *)btn;
	if(user_button == &button1)
  {
    xEventGroupSetBits( key_groups,(0x01<<1) );
  }
}
void Button_PRESS_DOWN_Callback(void* btn) //Press event
{
  struct Button *user_button = (struct Button *)btn;
	if(user_button == &button1)
  {
    printf("DOWN\n");
  }
}
void Button_PRESS_UP_Callback(void* btn) //Bouncing event
{
  struct Button *user_button = (struct Button *)btn;
	if(user_button == &button1)
  {
    printf("UP\n");
  }
}
void Button_PRESS_REPEAT_Callback(void* btn) //Press the event repeatedly
{
  struct Button *user_button = (struct Button *)btn;
	if(user_button == &button1)
  {
    printf("PRESS_REPEAT : %d\n",user_button->repeat);
  }
}
void Button_LONG_PRESS_START_Callback(void* btn) //A long press triggers an event
{
  struct Button *user_button = (struct Button *)btn;
	if(user_button == &button1)
  {
    printf("LONG_PRESS_START\n");
  }
}
void Button_LONG_PRESS_HOLD_Callback(void* btn) //The long press event keeps triggering
{
  struct Button *user_button = (struct Button *)btn;
	if(user_button == &button1)
  {
    printf("LONG_PRESS_HOLD\n");
  }
}
void button_Init(void)
{
  key_groups = xEventGroupCreate();
  //xEventGroupSetBits( TaskEven,(0x01<<2) ); 
  button_init(&button1, read_button_GPIO, button1_active , button1_id);      // Initialize Initialize object callback function trigger level key ID
  button_attach(&button1, SINGLE_CLICK, Button_SINGLE_CLICK_Callback);       //Click Register callback function
  //button_attach(&button1, LONG_PRESS_START, Button_LONG_PRESS_START_Callback);       //Click Register callback function
  button_attach(&button1, DOUBLE_CLICK, Button_DOUBLE_CLICK_Callback);       //Double-click to register the callback function
  //button_attach(&button1, PRESS_REPEAT, Button_PRESS_REPEAT_Callback);       //Click Register callback function
  const esp_timer_create_args_t clock_tick_timer_args = 
  {
    .callback = &clock_task_callback,
    .name = "clock_task",
    .arg = NULL,
  };
  esp_timer_handle_t clock_tick_timer = NULL;
  ESP_ERROR_CHECK(esp_timer_create(&clock_tick_timer_args, &clock_tick_timer));
  ESP_ERROR_CHECK(esp_timer_start_periodic(clock_tick_timer, 1000 * 5));  //5ms
  button_start(&button1); //Start button
}

