#include "drivers/ESP_UART.h"
#include "app/machine interface.h"
#include "stm32f4xx_hal.h"   
#include "stm32f4xx_hal_def.h"
#include "stm32f4xx_hal_i2c.h"
#include <stdint.h>

#include "app/UART_packet.h"

extern ESPTX_instance esp32_1; //main rectangle ESP32-S3 screen
extern system_interface zoetrope; //main system instance
static uint8_t uart_tx_TEST_buffer[16+34]; //transmit buffer

void send_packet(system_interface *sys){
    HAL_StatusTypeDef HAL_Status_UART = HAL_OK;

    if (huart4.gState == HAL_UART_STATE_READY){
        //THIS IS JUST SENDING DATA WITHOUT WORRYING ABOUT A START BYTE
        //HAL_Status_UART = HAL_UART_Transmit_DMA(&huart4,(uint8_t *)&data,sizeof(data));
        HAL_Status_UART = HAL_UART_Transmit_DMA(&huart4,(uint8_t *)&sys->platterRotationPeriod_ms,sizeof(sys->platterRotationPeriod_ms));
        
        if(HAL_Status_UART != HAL_OK){
            // Transmission Error
            sys->redLED = 1; //turn on red LED to indicate error
        }
    }
}

//callback function
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart){
    
    if (huart->Instance == UART4){ //uart of the rectangle esp32
        // Transmission complete
        // Safe to queue next packet
    }
}

// Transmit packet via DMA
static uint8_t tx_buffer[PACKET_MAX_PAYLOAD + 5]; // SOF+TYPE+LEN+CRC+payload

HAL_StatusTypeDef packet_send(system_interface *sys, uint8_t type, uint16_t len) {
    if(len > PACKET_MAX_PAYLOAD) return HAL_ERROR;

    //while testing
    buildTestPacket(sys); //while testing - sets data

    tx_buffer[0] = PACKET_SOF;
    tx_buffer[1] = (uint8_t)type;
    tx_buffer[2] = (uint8_t)((16+34) & 0xFF);
    tx_buffer[3] = (uint8_t)(((16+34) >> 8) & 0xFF);

    for(uint8_t k=0; k<16+34; k++){
        //inserting in our testing data here
        tx_buffer[4+k] = uart_tx_TEST_buffer[k];
    }
    tx_buffer[4] = packet_crc(tx_buffer, 4+16+34); //4+16+34 -CRC byte - unsure if this is working - not implemented

    //return HAL_UART_Transmit_DMA(&huart4, tx_buffer, 5+len);
    return HAL_UART_Transmit_DMA(&huart4, tx_buffer, 5+16+34); //4+16+34 -static while testing
}

void buildTestPacket(system_interface *sys){
    //the test buffer is a pre-set sequence of bytes for testing - it is sent into other functions as the sent data
    uart_tx_TEST_buffer[0] = sys->sliceCount; //THIS LINE ONLY, THE VALUE CONSTANTLY CHANGES - DONT REMOVE, BUT DONT USE ON ESP SIDE??
    uart_tx_TEST_buffer[1] = sys->system_state; //system state
    sys->platterRotationPeriod_ms = sys->platterRotationPeriod_us / 1000; //convert to ms for sending
    uart_tx_TEST_buffer[2] = (uint8_t)(sys->platterRotationPeriod_ms & 0xFF); //platter rotation period LSB
    uart_tx_TEST_buffer[3] = (uint8_t)((sys->platterRotationPeriod_ms >> 8) & 0xFF); //platter rotation period
    uart_tx_TEST_buffer[4] = (uint8_t)((sys->platterRotationPeriod_ms >> 16) & 0xFF); //platter rotation period
    uart_tx_TEST_buffer[5] = (uint8_t)((sys->platterRotationPeriod_ms >> 24) & 0xFF); //platter rotation period MSB
    uart_tx_TEST_buffer[6] = sys->sliceCount; //number of frames(slices) on platter
    uart_tx_TEST_buffer[7] = (uint8_t)(sys->motorSpeedPot & 0xFF); //motor speed potentiometer LSB
    uart_tx_TEST_buffer[8] = (uint8_t)((sys->motorSpeedPot >> 8) & 0xFF); //motor speed potentiometer MSB


    uart_tx_TEST_buffer[9] = sys->strobeEnabled; //strobe enabled flag
    uart_tx_TEST_buffer[10] = sys->motorEnabled; //motor enabled flag
    uart_tx_TEST_buffer[11] = sys->motorDirection; //motor direction flag
    uart_tx_TEST_buffer[12] = sys->esp32_1_verticalLineHighlight; //ui vertical line highight (0 = non selected/highlighted)
    uart_tx_TEST_buffer[13] = sys->esp32_1_horizontalLineHighlight; //ui horizontal line highight (0 = non selected/highlighted)
    uart_tx_TEST_buffer[14] = sys->esp32_1_highlightFlag; //ui highlight flag (0 = non selected/highlighted)
    uart_tx_TEST_buffer[15] = sys->esp32_1_selectFlag; //ui selection change flag (0 = no selection, possible highlight)
    
    
    //need to add the below into the esp32 receiving side
    uart_tx_TEST_buffer[16] = sys->strobeEnabled; //strobe enabled flag
    uart_tx_TEST_buffer[17] = (uint8_t)(sys->tempSensor1 & 0xFF); //temperature 1 LSB
    uart_tx_TEST_buffer[18] = (uint8_t)((sys->tempSensor1 >> 8) & 0xFF); //temperature 1 MSB
    uart_tx_TEST_buffer[19] = (uint8_t)(sys->tempSensor2 & 0xFF); //temperature 2 LSB
    uart_tx_TEST_buffer[20] = (uint8_t)((sys->tempSensor2 >> 8) & 0xFF); //temperature 2 MSB
    //uart_tx_TEST_buffer[21] = (uint8_t)(sys->ambientLightLevel1 & 0xFF); //ambient light level 1 LSB
    //uart_tx_TEST_buffer[22] = (uint8_t)((sys->ambientLightLevel1 >> 8) & 0xFF); //ambient light level 1 MSB
    uart_tx_TEST_buffer[23] = (uint8_t)(sys->ambientLightLevel2 & 0xFF); //ambient light level 2 LSB
    uart_tx_TEST_buffer[24] = (uint8_t)((sys->ambientLightLevel2 >> 8) & 0xFF); //ambient light level 2 MSB
    uart_tx_TEST_buffer[25] = sys->strobeMode; //strobeMode setting
    uart_tx_TEST_buffer[26] = sys->motorMode; //motorMode setting
    uart_tx_TEST_buffer[27] = sys->motorMicrostepSetting; //stepper motor step configuration
    uart_tx_TEST_buffer[28] = sys->motor_spreadCycle; //spreadcycle/stealthchop setting
    uart_tx_TEST_buffer[29] = sys->platterSensor; //sensor detecting platter installed flag
    uart_tx_TEST_buffer[30] = (uint8_t)(sys->heartbeatTime_ms & 0xFF); //LSB
    uart_tx_TEST_buffer[31] = (uint8_t)((sys->heartbeatTime_ms >> 8) & 0xFF);
    uart_tx_TEST_buffer[32] = (uint8_t)((sys->heartbeatTime_ms >> 16) & 0xFF);
    uart_tx_TEST_buffer[33] = (uint8_t)((sys->heartbeatTime_ms >> 24) & 0xFF); //MSB

    uart_tx_TEST_buffer[34] = sys->fan1_enable; //state of the motor fan
    uart_tx_TEST_buffer[35] = sys->fan2_enable; //state of the PCB fan - not implemented

    //we need to pass over the strobe on time!!
    //NEED TO WRITE THE ESP32 RECEIVING SIDE OF THIS DATA
    uart_tx_TEST_buffer[36] = (uint8_t)(sys->strobeCCRValue & 0xFF); //strobe CCR value LSB
    uart_tx_TEST_buffer[37] = (uint8_t)((sys->strobeCCRValue >> 8) & 0xFF); //strobe CCR value MSB

    //the below is arleady being sent, reserved
    uart_tx_TEST_buffer[38] = 0; //reserved for future use
    uart_tx_TEST_buffer[39] = 0; //reserved for future use
    uart_tx_TEST_buffer[40] = 0; //reserved for future use  
    uart_tx_TEST_buffer[41] = 0; //reserved for future use
    uart_tx_TEST_buffer[42] = 0; //reserved for future use
    uart_tx_TEST_buffer[43] = 0; //reserved for future use
    uart_tx_TEST_buffer[44] = 0; //reserved for future use
    uart_tx_TEST_buffer[45] = 0; //reserved for future use
    uart_tx_TEST_buffer[46] = 0; //reserved for future use
    uart_tx_TEST_buffer[47] = 0; //reserved for future use
    uart_tx_TEST_buffer[48] = 0; //reserved for future use
    uart_tx_TEST_buffer[49] = 0; //reserved for future use

    //next I will add the below into the data flow
    //uint32_t rotation period
    //uint8_t strobe enabled
    //uint16_t temperature 1
    //uint16_t temperature 2
    //uint16_t light sensor 1
    //uint16_t light sensor 2
    //uint8_t strobe mode
    //uint8_t motor mode
    //uint8_t stepper motor step config
    //uint8_t stepper motor stealthchop
    //uint8_t platter installed
    //uint32_t heartbeat pulse time
    //uint8_t fan 1 state
    //uint8_t fan 2 state
}
