#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"
#include "sd_card_bsp.h"


#define PIN_NUM_MISO  40
#define PIN_NUM_MOSI  39
#define PIN_NUM_CLK   41
#define PIN_NUM_CS    -1
#define EXAMPLE_MAX_CHAR_SIZE    64  //The maximum value of read data
#define SDlist "/sd_card" //Directory, similar to a standard

esp_err_t s_example_write_file(const char *path, char *data);
sdmmc_card_t *card = NULL;

void SD_card_Init(void)
{
  esp_vfs_fat_sdmmc_mount_config_t mount_config = 
  {
    .format_if_mount_failed = true,       //If the hook fails, create a partition table and format the SD card
    .max_files = 5,                       //Maximum number of open files
    .allocation_unit_size = 16 * 1024 *3  //Similar to sector size
  };

  sdmmc_host_t host = SDMMC_HOST_DEFAULT();
  host.max_freq_khz = SDMMC_FREQ_HIGHSPEED;//high speed

  sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
  slot_config.width = 1;           //1-wire
  slot_config.clk = PIN_NUM_CLK;
  slot_config.cmd = PIN_NUM_MOSI;
  slot_config.d0 = PIN_NUM_MISO;
  ESP_ERROR_CHECK_WITHOUT_ABORT(esp_vfs_fat_sdmmc_mount(SDlist, &host, &slot_config, &mount_config, &card));

  if(card != NULL)
  {
    sdmmc_card_print_info(stdout, card); //Print out the card information
    //printf("sios:%.2f\n",(float)(card->csd.capacity)/2048/1024);//g
  }
}
float sd_cadr_get_value(void)
{
  if(card != NULL)
  {
    return (float)(card->csd.capacity)/2048/1024; //G
  }
  else
  return 0;
}

/*write data
path:path
data:data
*/
esp_err_t s_example_write_file(const char *path, char *data)
{
  esp_err_t err;
  if(card == NULL)
  {
    return ESP_ERR_NOT_FOUND;
  }
  err = sdmmc_get_status(card); //First check if there is an SD card
  if(err != ESP_OK)
  {
    return err;
  }
  FILE *f = fopen(path, "w"); //Get path address
  if(f == NULL)
  {
    printf("path:Write Wrong path\n");
    return ESP_ERR_NOT_FOUND;
  }
  fprintf(f, data); //write in
  fclose(f);
  return ESP_OK;
}
/*
read data
path:path
*/
esp_err_t s_example_read_file(const char *path,uint8_t *pxbuf,uint32_t *outLen)
{
  esp_err_t err;
  if(card == NULL)
  {
    printf("path:card == NULL\n");
    return ESP_ERR_NOT_FOUND;
  }
  err = sdmmc_get_status(card); //First check if there is an SD card
  if(err != ESP_OK)
  {
    printf("path:card == NO\n");
    return err;
  }
  FILE *f = fopen(path, "rb");
  if (f == NULL)
  {
    printf("path:Read Wrong path\n");
    return ESP_ERR_NOT_FOUND;
  }
  fseek(f, 0, SEEK_END);     //Move the pointer to the back
  uint32_t unlen = ftell(f);
  //fgets(pxbuf, unlen, f); //Read text
  fseek(f, 0, SEEK_SET); //Move the pointer to the front
  uint32_t poutLen = fread((void *)pxbuf,1,unlen,f);
  printf("pxlen: %ld,outLen: %ld\n",unlen,poutLen);
  *outLen = poutLen;
  fclose(f);
  return ESP_OK;
}
/*
struct stat st;
stat(file_foo, &st);//file foo is a string. The file name needs a suffix to indicate whether the file exists
unlink(file_foo);//Return 0 after successfully deleting the file
rename(char*,char*);//rename file
esp_vfs_fat_sdcard_format();//formatting
esp_vfs_fat_sdcard_unmount(mount_point, card);//Uninstalling the vfs
FILE *fopen(const char *filename, const char *mode);
"w": Open the file in write mode, and if the file exists, empty the file contents; If the file does not exist, a new file is created.
"a": Open the file in append mode and create a new file if it does not exist.
mkdir(dirname, mode);createFolder

Read other non-text type data "rb" mode is used to open files in read-only and binary mode, suitable for binary files such as images.
If you only use "r", the file will be opened in text mode, which can cause data corruption or errors when reading the binary file.
Therefore, for image files (such as JPEG, PNG, etc.), you should use the "rb" mode to ensure that the file contents are read correctly.
b converts to binary
The following two functions enable file size returns
  fseek(file, 0, SEEK_END)：Move the file pointer to the end of the file.
  ftell(file)；Returns the pointer position of the current file, which is the file size, in bytes
*/