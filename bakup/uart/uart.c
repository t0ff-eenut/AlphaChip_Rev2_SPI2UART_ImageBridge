////////////////////////////////////////////////////////////////////////////////////////////////
// 1. UART 구조체 초기화
// 2. UART 구조체 해제
// 3. Checksum Process
// 4. UART 수신 ESP <- PC
    // 반복
// 5. UART CMD 분류 Process
// 6. UART CMD 송신 Transaction ESP -> PC
// 7. UART 전송 Thread [Image, CMD]
// 8. UART 초기화
////////////////////////////////////////////////////////////////////////////////////////////////

#include "uart/uart.h"

// from queue.c
extern esp_err_t esp_err;
extern QueueHandle_t Q_ui8_frame_32768bit;
extern QueueHandle_t Q_ui8_uart_recive_8bit;
extern QueueHandle_t Q_ui32_cmd_128bit;
extern QueueHandle_t Q_ui32_spi_recive_reg_64bit;

// 1. UART 구조체 초기화
bool init_uart_structer(uart_structer* buf) {
    bool b_success = true;

    buf->ui8_signal      = (uint8_t*)malloc(sizeof(uint8_t));
    buf->ui8_cmd         = (uint8_t*)malloc(sizeof(uint8_t));
    buf->ui8_addr_length = (uint8_t*)malloc(sizeof(uint8_t));
    buf->ui8_addr        = (uint8_t*)malloc(sizeof(uint8_t));  // 초기값 1바이트
    buf->ui8_data_length = (uint8_t*)malloc(sizeof(uint8_t));
    buf->ui8_data        = (uint8_t*)malloc(sizeof(uint8_t));  // 초기값 1바이트
    buf->ui8_chksum      = (uint8_t*)malloc(sizeof(uint8_t));
    buf->ui8_dummy       = (uint8_t*)malloc(sizeof(uint8_t));

    b_success &= (buf->ui8_signal      != NULL);
    b_success &= (buf->ui8_cmd         != NULL);
    b_success &= (buf->ui8_addr_length != NULL);
    b_success &= (buf->ui8_addr        != NULL);
    b_success &= (buf->ui8_data_length != NULL);
    b_success &= (buf->ui8_data        != NULL);
    b_success &= (buf->ui8_chksum      != NULL);
    b_success &= (buf->ui8_dummy       != NULL);

    if (b_success) {
        memset(buf->ui8_signal,      0, sizeof(uint8_t));
        memset(buf->ui8_cmd,         0, sizeof(uint8_t));
        memset(buf->ui8_addr_length, 0, sizeof(uint8_t));
        memset(buf->ui8_addr,        0, sizeof(uint8_t));
        memset(buf->ui8_data_length, 0, sizeof(uint8_t));
        memset(buf->ui8_data,        0, sizeof(uint8_t));
        memset(buf->ui8_chksum,      0, sizeof(uint8_t));
        memset(buf->ui8_dummy,       0, sizeof(uint8_t));
    }
    return b_success;
}

// 2. UART 구조체 해제
void free_uart_structer(uart_structer* buf) {
    free(buf->ui8_signal);
    free(buf->ui8_cmd);
    free(buf->ui8_addr_length);
    free(buf->ui8_addr);
    free(buf->ui8_data_length);
    free(buf->ui8_data);
    free(buf->ui8_chksum);
    free(buf->ui8_dummy);
}

// 3. Checksum Process
uint8_t checksum(uart_structer* uart_recive_data){
    uint8_t ui8_checksum = 0;

    ui8_checksum ^= *uart_recive_data->ui8_signal; // XOR 체크섬

    ui8_checksum ^= *uart_recive_data->ui8_cmd; // XOR 체크섬

    ui8_checksum ^= *uart_recive_data->ui8_addr_length; // XOR 체크섬
    for(uint8_t i = 0; i < *uart_recive_data->ui8_addr_length; i++) {
        ui8_checksum ^= uart_recive_data->ui8_addr[i]; // XOR 체크섬
    }

    ui8_checksum ^= *uart_recive_data->ui8_data_length; // XOR 체크섬
    for(uint8_t i = 0; i < *uart_recive_data->ui8_data_length; i++) {
        ui8_checksum ^= uart_recive_data->ui8_data[i]; // XOR 체크섬
    }

    return ui8_checksum;
}

// 4. UART 수신 ESP <- PC
void uart_rx_thread(void)
{
    uint8_t* A_ui8_uart_recive_8bit = (uint8_t*)malloc(sizeof(uint8_t));
    // 공간 생성 실패시
    if(A_ui8_uart_recive_8bit == NULL) {
        printf("malloc failed: A_ui8_uart_recive\n");
        free(A_ui8_uart_recive_8bit);
        A_ui8_uart_recive_8bit = NULL;
        return ERROR_MALLOC_FAIL;
    }

    // 반복
    while (1) {
        // int len = uart_read_bytes(UART_NUM_1, A_ui8_uart_recive, sizeof(uint8_t), 20 / portTICK_PERIOD_MS);
        if (uart_read_bytes(UART_NUM_1, A_ui8_uart_recive_8bit, sizeof(uint8_t), 0) > 0){
            xQueueSend(Q_ui8_uart_recive_8bit, A_ui8_uart_recive_8bit, 0);
        }
        else{
            vTaskDelay(1);
        }
        // UBaseType_t stack_remaining = uxTaskGetStackHighWaterMark(NULL);
        // printf("uart_rx_thread : 남은 stack : %u word ( = %u byte)\n", stack_remaining, stack_remaining * 4); // spi_image_rx_thread : 남은 stack : 1988 ( = 7952)
    }
    // 반복

    free(A_ui8_uart_recive_8bit);
    A_ui8_uart_recive_8bit = NULL;
    return 0;
}

// 5. UART CMD 분류 Process
uint8_t uart_rx_data_process(void){
    uint8_t             ui8_return_value                    = 0;
    uint8_t*            ui8_uart_recv_q_recv_8bit;
    uart_structer       uartstructer_uart_recive_buf;
    uint32_t*           ui32_cmd_q_insert_128bit;
    uart_recive_state   e_uart_recive_state         = UART_STATE_SIGNAL;
    int                 i_cmd_recive_count          = 0;

    ui8_uart_recv_q_recv_8bit = (uint8_t*)malloc(sizeof(uint8_t));
    if (ui8_uart_recv_q_recv_8bit == NULL) {
        printf("malloc failed: A_ui8_uart_recive\n");
        free(ui8_uart_recv_q_recv_8bit);
        return ERROR_MALLOC_FAIL;
    }
    memset(ui8_uart_recv_q_recv_8bit, 0, sizeof(uint8_t));

    if(!init_uart_structer(&uartstructer_uart_recive_buf)){
        printf("malloc failed: uartstructer_uart_recive_buf\n");
        free_uart_structer(&uartstructer_uart_recive_buf);
        return ERROR_MALLOC_FAIL;
    }

    ////////////////////////////////////////////////
    // ADDR_length // ADDR // DATA_length // DATA //
    ////////////////////////////////////////////////
    ui32_cmd_q_insert_128bit = (uint32_t*)malloc(sizeof(uint32_t) * 4);
    if (ui32_cmd_q_insert_128bit == NULL) {
        printf("malloc failed: ui32_cmd_q_insert_128bit\n");
        free(ui32_cmd_q_insert_128bit);
        return ERROR_MALLOC_FAIL;
    }
    memset(ui32_cmd_q_insert_128bit, 0, sizeof(uint32_t) * 4);

    // Tread 반복
    while (1) {
        if (uxQueueMessagesWaiting(Q_ui8_uart_recive_8bit) > 0) {
            xQueueReceive(Q_ui8_uart_recive_8bit, ui8_uart_recv_q_recv_8bit, portMAX_DELAY);
            // printf("uart read_byte : %d %02X \n", *ui8_uart_recv_q_recv_8bit, *ui8_uart_recv_q_recv_8bit);

            switch (e_uart_recive_state) {
                case UART_STATE_SIGNAL:
                    if (*ui8_uart_recv_q_recv_8bit == IMAGE_SIGNAL || *ui8_uart_recv_q_recv_8bit == CMD_SIGNAL) {
                        *uartstructer_uart_recive_buf.ui8_signal = *ui8_uart_recv_q_recv_8bit;
                        if(*ui8_uart_recv_q_recv_8bit == IMAGE_SIGNAL){
                            // *uartstructer_uart_recive_buf.ui8_addr_length = 1;
                            // printf("UART_RECIVE_IMAGE_SIGNAL\n");
                        }
                        else if(*ui8_uart_recv_q_recv_8bit == CMD_SIGNAL){
                            // *uartstructer_uart_recive_buf.ui8_addr_length = 4;
                            // printf("UART_RECIVE_CMD_SIGNAL\n");
                        }
                        // *uartstructer_uart_recive_buf.ui8_data_length = 4;
                        // memset(uartstructer_uart_recive_buf.ui8_addr, 0, sizeof(uint8_t) * *uartstructer_uart_recive_buf.ui8_addr_length);
                        // memset(uartstructer_uart_recive_buf.ui8_data, 0, sizeof(uint8_t) * *uartstructer_uart_recive_buf.ui8_data_length);
                        i_cmd_recive_count = 0;
                        e_uart_recive_state = UART_STATE_CMD;
                    }
                    break;

                case UART_STATE_CMD:
                    // printf("UART_RECIVE_UART_STATE_CMD\n");
                    *uartstructer_uart_recive_buf.ui8_cmd = *ui8_uart_recv_q_recv_8bit;
                    i_cmd_recive_count = 0;
                    e_uart_recive_state = UART_STATE_ADDR;
                    break;

                case UART_STATE_ADDR:
                    // printf("UART_RECIVE_UART_STATE_ADDR\n");
                    if(i_cmd_recive_count == 0) {
                        *uartstructer_uart_recive_buf.ui8_addr_length = *ui8_uart_recv_q_recv_8bit;
                        if(uartstructer_uart_recive_buf.ui8_addr != NULL){
                            free(uartstructer_uart_recive_buf.ui8_addr);
                            uartstructer_uart_recive_buf.ui8_addr = NULL;
                        }
                        uartstructer_uart_recive_buf.ui8_addr = (uint8_t*)malloc(sizeof(uint8_t) * (*uartstructer_uart_recive_buf.ui8_addr_length));
                        if(uartstructer_uart_recive_buf.ui8_addr == NULL){
                            printf("malloc failed: uartstructer_uart_recive_buf.ui8_addr\n");
                            free(uartstructer_uart_recive_buf.ui8_addr);
                            return ERROR_MALLOC_FAIL;
                        }
                        memset(uartstructer_uart_recive_buf.ui8_addr, 0, sizeof(uint8_t) * (*uartstructer_uart_recive_buf.ui8_addr_length));
                    }
                    else{
                        uartstructer_uart_recive_buf.ui8_addr[i_cmd_recive_count - 1] = *ui8_uart_recv_q_recv_8bit;
                    }
                    i_cmd_recive_count++;
                    if (i_cmd_recive_count == *uartstructer_uart_recive_buf.ui8_addr_length + 1) {
                        e_uart_recive_state = UART_STATE_DATA;
                        i_cmd_recive_count = 0;
                    }
                    break;

                case UART_STATE_DATA:
                    // printf("UART_RECIVE_UART_STATE_DATA\n");
                    if (i_cmd_recive_count == 0) {
                        *uartstructer_uart_recive_buf.ui8_data_length = *ui8_uart_recv_q_recv_8bit;
                        if(uartstructer_uart_recive_buf.ui8_data != NULL){
                            free(uartstructer_uart_recive_buf.ui8_data);
                            uartstructer_uart_recive_buf.ui8_data = NULL;
                        }
                        uartstructer_uart_recive_buf.ui8_data = (uint8_t*)malloc(sizeof(uint8_t) * (*uartstructer_uart_recive_buf.ui8_data_length));
                        if(uartstructer_uart_recive_buf.ui8_data == NULL){
                            printf("malloc failed: uartstructer_uart_recive_buf.ui8_data\n");
                            free(uartstructer_uart_recive_buf.ui8_data);
                            return ERROR_MALLOC_FAIL;
                        }
                        memset(uartstructer_uart_recive_buf.ui8_data, 0, sizeof(uint8_t) * (*uartstructer_uart_recive_buf.ui8_data_length));
                    }
                    else {
                        uartstructer_uart_recive_buf.ui8_data[i_cmd_recive_count - 1] = *ui8_uart_recv_q_recv_8bit;
                    }
                    i_cmd_recive_count++;
                    if (i_cmd_recive_count == *uartstructer_uart_recive_buf.ui8_data_length + 1) {
                        e_uart_recive_state = UART_STATE_CHKSUM;
                        i_cmd_recive_count = 0;
                    }
                    break;

                case UART_STATE_CHKSUM:
                    // printf("UART_RECIVE_UART_STATE_CHKSUM\n");
                    *uartstructer_uart_recive_buf.ui8_chksum = *ui8_uart_recv_q_recv_8bit;

                    if(*uartstructer_uart_recive_buf.ui8_chksum == checksum(&uartstructer_uart_recive_buf)){
                        // printf("UART_RECIVE_Checksum ok\n");
                        if (*uartstructer_uart_recive_buf.ui8_signal == CMD_SIGNAL) {
                            // printf("UART_RECIVE : CMD -> Queue\n");
                            ui32_cmd_q_insert_128bit[0] = *uartstructer_uart_recive_buf.ui8_addr_length;
                            for (int i = 0; i < *uartstructer_uart_recive_buf.ui8_addr_length; i++) {
                                ui32_cmd_q_insert_128bit[1] |= (uartstructer_uart_recive_buf.ui8_addr[i] << (8 * ((*uartstructer_uart_recive_buf.ui8_addr_length - i) - 1)));
                            }
                            ui32_cmd_q_insert_128bit[2] = *uartstructer_uart_recive_buf.ui8_data_length;
                            for (int i = 0; i < *uartstructer_uart_recive_buf.ui8_data_length; i++) {
                                ui32_cmd_q_insert_128bit[3] |= (uartstructer_uart_recive_buf.ui8_data[i] << (8 * ((*uartstructer_uart_recive_buf.ui8_data_length - i) - 1)));
                            }
                            xQueueSend(Q_ui32_cmd_128bit, ui32_cmd_q_insert_128bit, 0);
                            memset(ui32_cmd_q_insert_128bit, 0, sizeof(uint32_t) * 4);
                        } else if (*uartstructer_uart_recive_buf.ui8_signal == IMAGE_SIGNAL) {
                            // printf("UART_RECIVE : IMAGE -> Queue[NC]\n");
                        }
                    }
                    else {
                        printf("UART_RECIVE_Checksum Error\n");
                    }
                    // 상태 초기화
                    e_uart_recive_state = UART_STATE_SIGNAL;
                    break;
            }
        }
        vTaskDelay(1);
    }
    // Tread 반복

    free_uart_structer(&uartstructer_uart_recive_buf);
    free(ui8_uart_recv_q_recv_8bit);
    ui8_uart_recv_q_recv_8bit = NULL;
    free(ui32_cmd_q_insert_128bit);
    ui32_cmd_q_insert_128bit = NULL;
    return ui8_return_value;
}

// 6. UART CMD 송신 Transaction ESP -> PC
uint8_t uart_tx_transaction(uart_structer* uart_send_data){
    uint8_t     ui8_return_value               = 0;

    uint8_t*    ui8_uart_image_recive_chk_8bit;        // UART 전송 검증
    // 에러 검출을 위한 공간 확보
    ui8_uart_image_recive_chk_8bit = (uint8_t*)malloc(sizeof(uint8_t));
    if (ui8_uart_image_recive_chk_8bit == NULL){
        printf("malloc failed: ui8_uart_image_recive_chk_8bit\n");
        free(ui8_uart_image_recive_chk_8bit);
        return ERROR_MALLOC_FAIL;
    }
    memset(ui8_uart_image_recive_chk_8bit, 0, sizeof(uint8_t));

    uart_write_bytes(UART_NUM_1, uart_send_data->ui8_signal, 1);                                // 0xCC
    uart_write_bytes(UART_NUM_1, uart_send_data->ui8_cmd, 1);                                   // 0x80
    uart_write_bytes(UART_NUM_1, uart_send_data->ui8_addr_length, 1);                           // 0x01
    uart_write_bytes(UART_NUM_1, uart_send_data->ui8_addr, *uart_send_data->ui8_addr_length);
    uart_write_bytes(UART_NUM_1, uart_send_data->ui8_data_length, 1);
    uart_write_bytes(UART_NUM_1, uart_send_data->ui8_data, *uart_send_data->ui8_data_length);
    uart_write_bytes(UART_NUM_1, uart_send_data->ui8_chksum, 1);
    uart_write_bytes(UART_NUM_1, uart_send_data->ui8_dummy, 1);

    free(ui8_uart_image_recive_chk_8bit);
    ui8_uart_image_recive_chk_8bit = NULL;
}

// 7. Uart 전송 Thread [Image, CMD]
uint8_t uart_tx_thread(void *arg)
{   
    //// 이미지 전송 /////
    ///////////////////////
    //  CMD    //  ADDR  //
    //  8bit   //  8bit  //
    ///////////////////////

    // 8bit == 1 Pixel
    //////////////////////////////////////////////////////////////////////
    //                              64개                                //
    // / 8bit / 8bit / 8bit / 8bit / ... / 8bit / 8bit / 8bit / 8bit /  //          1
    // / 8bit / 8bit / 8bit / 8bit / ... / 8bit / 8bit / 8bit / 8bit /  //          2
    //                                .                                 //          
    //                                .                                 //  64개    
    //                                .                                 //
    // / 8bit / 8bit / 8bit / 8bit / ... / 8bit / 8bit / 8bit / 8bit /  //          63
    // / 8bit / 8bit / 8bit / 8bit / ... / 8bit / 8bit / 8bit / 8bit /  //          64
    //////////////////////////////////////////////////////////////////////

    /////////////////////
    // 프로세스 진행 방향 //
    /////////////////////
    /////////////////////
    //     공간 확보    //
    //    FRAME 전송   //
    //     CMD 전송    //
    /////////////////////

    uart_structer uartstructer_uart_send_buf;       // UART 전송 규격
    uint8_t** ui8_frame_q_recv_32768bit;            // Queue -> Frame 공간
    uint8_t* ui8_spi_reg_q_recv_32bit;              // Queue -> REG 공간
    uint32_t* A_ui32_spi_recive_reg_q_recv_64bit;

    // 공간 확보  /////////////////////////////////////////
    ui8_frame_q_recv_32768bit = (uint8_t**)malloc(sizeof(uint8_t*) * 64);
    if (ui8_frame_q_recv_32768bit == NULL) {
        printf("malloc failed: ui8_frame_q_recv_32768bit\n");
        free(ui8_frame_q_recv_32768bit);
        return ERROR_MALLOC_FAIL;
    }
    for (int i_y = 0; i_y < 64; i_y++) {
        ui8_frame_q_recv_32768bit[i_y] = (uint8_t*)malloc(sizeof(uint8_t) * 64);
        // 공간 확보 실패
        if (ui8_frame_q_recv_32768bit[i_y] == NULL) {
            printf("malloc failed: ui8_frame_q_recv_32768bit[%d]\n", i_y);
            for (int i_y_del = 0; i_y_del < i_y; i_y_del++) {
                if (ui8_frame_q_recv_32768bit[i_y_del]) {
                    free(ui8_frame_q_recv_32768bit[i_y_del]);
                    ui8_frame_q_recv_32768bit[i_y_del] = NULL;
                }
            }
            free(ui8_frame_q_recv_32768bit);
            ui8_frame_q_recv_32768bit = NULL;
            return ERROR_MALLOC_FAIL;
        }
        memset(ui8_frame_q_recv_32768bit[i_y], 0, sizeof(uint8_t) * 64);
    }

    ui8_spi_reg_q_recv_32bit = (uint8_t*)malloc(sizeof(uint8_t) * 4);
    if (ui8_spi_reg_q_recv_32bit == NULL) {
        printf("malloc failed: ui8_spi_reg_q_recv_32bit\n");
        free(ui8_spi_reg_q_recv_32bit);
        return ERROR_MALLOC_FAIL;
    }
    memset(ui8_spi_reg_q_recv_32bit, 0, sizeof(uint8_t) * 4);

    A_ui32_spi_recive_reg_q_recv_64bit = (uint32_t*)malloc(sizeof(uint32_t) * 2);
    if (A_ui32_spi_recive_reg_q_recv_64bit == NULL) {
        printf("malloc failed: A_ui32_spi_recive_reg_q_recv_64bit\n");
        free(A_ui32_spi_recive_reg_q_recv_64bit);
        return ERROR_MALLOC_FAIL;
    }
    memset(A_ui32_spi_recive_reg_q_recv_64bit, 0, sizeof(uint32_t) * 2);

    // Uart 구조체 공간 확보 실패 시
    if(!init_uart_structer(&uartstructer_uart_send_buf)){
        printf("malloc failed: uartstructer_uart_send_buf\n");
        free_uart_structer(&uartstructer_uart_send_buf);
        return ERROR_MALLOC_FAIL;
    }
    // 공간 확보  /////////////////////////////////////////


    // Tread 반복
    while(1){
        
        // Frame 전송 ////////////////////////////////////////
        // 이미지 전송 (Chip -> ESP32 -> PC)
        // printf("Q_ui8_frame_32768bit Size : [%d]\n", uxQueueMessagesWaiting(Q_ui8_frame_32768bit));
        if(uxQueueMessagesWaiting(Q_ui8_frame_32768bit) > 0){
            // 이미지 1장 꺼내기
            xQueueReceive(Q_ui8_frame_32768bit, ui8_frame_q_recv_32768bit, portMAX_DELAY);
            *uartstructer_uart_send_buf.ui8_signal         = IMAGE_SIGNAL;                         // 이미지 Signal
            *uartstructer_uart_send_buf.ui8_cmd            = WRITE_CMD;                            // Read CMD
            *uartstructer_uart_send_buf.ui8_addr_length    = 1;                                    // 1 줄
            *uartstructer_uart_send_buf.ui8_data_length    = 64;                                   // 64 줄
            if(uartstructer_uart_send_buf.ui8_addr != NULL){
                printf("ui8_addr not NULL\n");
                free(uartstructer_uart_send_buf.ui8_addr);
                uartstructer_uart_send_buf.ui8_addr = NULL;
            }
            if(uartstructer_uart_send_buf.ui8_data != NULL){
                printf("ui8_data not NULL\n");
                free(uartstructer_uart_send_buf.ui8_data);
                uartstructer_uart_send_buf.ui8_data = NULL;
            }
            if(uartstructer_uart_send_buf.ui8_addr == NULL){
                uartstructer_uart_send_buf.ui8_addr = (uint8_t*)malloc(sizeof(uint8_t) * (*uartstructer_uart_send_buf.ui8_addr_length));
                if(uartstructer_uart_send_buf.ui8_addr == NULL){
                    printf("malloc failed: uartstructer_uart_send_buf.ui8_addr\n");
                    free(uartstructer_uart_send_buf.ui8_addr);
                    return ERROR_MALLOC_FAIL;
                }
                memset(uartstructer_uart_send_buf.ui8_addr, 0, sizeof(uint8_t) * (*uartstructer_uart_send_buf.ui8_addr_length));
            }
            if(uartstructer_uart_send_buf.ui8_data == NULL){
                uartstructer_uart_send_buf.ui8_data = (uint8_t*)malloc(sizeof(uint8_t) * (*uartstructer_uart_send_buf.ui8_data_length));
                if(uartstructer_uart_send_buf.ui8_data == NULL){
                    printf("malloc failed: uartstructer_uart_send_buf.ui8_data\n");
                    free(uartstructer_uart_send_buf.ui8_data);
                    return ERROR_MALLOC_FAIL;
                }
                memset(uartstructer_uart_send_buf.ui8_data, 0, sizeof(uint8_t) * (*uartstructer_uart_send_buf.ui8_data_length));
            }

            // // 64줄 //////////////////////////////////
            // printf("Start Frame - %d\n", uxQueueMessagesWaiting(Q_ui8_frame_32768bit));
            // for(uint8_t i_y = 0; i_y < *uartstructer_uart_send_buf.ui8_data_length; i_y++){
            //     for(uint8_t i_x = 0; i_x < *uartstructer_uart_send_buf.ui8_data_length; i_x++){
            //         printf("%02X ",ui8_frame_q_recv_32768bit[i_y][i_x]);
            //     }
            //     printf("\n");
            // }
            // printf("\n");
            // // 64줄 //////////////////////////////////

            // 64줄 //////////////////////////////////
            // //////
            // printf("Start Frame - %d\n", uxQueueMessagesWaiting(Q_ui8_frame_32768bit));
            // //////

            for(uint8_t i_y = 0; i_y < *uartstructer_uart_send_buf.ui8_data_length; i_y++){
                memcpy(uartstructer_uart_send_buf.ui8_addr, &i_y, *uartstructer_uart_send_buf.ui8_addr_length);
                memcpy(uartstructer_uart_send_buf.ui8_data, ui8_frame_q_recv_32768bit[i_y], *uartstructer_uart_send_buf.ui8_data_length);
                *uartstructer_uart_send_buf.ui8_chksum = checksum(&uartstructer_uart_send_buf);

                // //////   
                // printf("ui8_signal : %02X\n",*uartstructer_uart_send_buf.ui8_signal);
                // printf("ui8_cmd : %02X\n",*uartstructer_uart_send_buf.ui8_cmd);
                // printf("ui8_addr_length : %d\n",*uartstructer_uart_send_buf.ui8_addr_length);
                // printf("ui8_addr : %d\n",*uartstructer_uart_send_buf.ui8_addr);
                // printf("ui8_data_length : %d\n",*uartstructer_uart_send_buf.ui8_data_length);
                // printf("ui8_data : ");
                // for(uint8_t i_x = 0; i_x < *uartstructer_uart_send_buf.ui8_data_length; i_x++){
                //     printf("%02X ",ui8_frame_q_recv_32768bit[i_y][i_x]);
                // }
                // printf("\n");
                // printf("ui8_chksum : %02X\n",*uartstructer_uart_send_buf.ui8_chksum);
                // printf("ui8_dummy : %02X\n",*uartstructer_uart_send_buf.ui8_dummy);
                // //////


                uart_tx_transaction(&uartstructer_uart_send_buf);
            }
            // 64줄 //////////////////////////////////
            free(uartstructer_uart_send_buf.ui8_addr);
            uartstructer_uart_send_buf.ui8_addr = NULL;
            free(uartstructer_uart_send_buf.ui8_data);
            uartstructer_uart_send_buf.ui8_data = NULL;
        }
        vTaskDelay(1);
        // Frame 전송 ////////////////////////////////////////

        // REG 전송 ////////////////////////////////////////
        // REGISTER 정보 전송 (Chip -> ESP32 -> PC)
        if(uxQueueMessagesWaiting(Q_ui32_spi_recive_reg_64bit) > 0){

            printf("NO NO NO - %d\n", uxQueueMessagesWaiting(Q_ui32_spi_recive_reg_64bit));

            xQueueReceive(Q_ui32_spi_recive_reg_64bit, A_ui32_spi_recive_reg_q_recv_64bit, portMAX_DELAY); 
            *uartstructer_uart_send_buf.ui8_signal         = CMD_SIGNAL;                         // CMD Signal
            *uartstructer_uart_send_buf.ui8_cmd            = WRITE_CMD;                          // Write CMD
            *uartstructer_uart_send_buf.ui8_addr_length    = 4;                                  // 4 줄
            *uartstructer_uart_send_buf.ui8_data_length    = 4;                                  // 4 줄
            if(uartstructer_uart_send_buf.ui8_addr != NULL){
                free(uartstructer_uart_send_buf.ui8_addr);
                uartstructer_uart_send_buf.ui8_addr = NULL;
            }
            if(uartstructer_uart_send_buf.ui8_data != NULL){
                free(uartstructer_uart_send_buf.ui8_data);
                uartstructer_uart_send_buf.ui8_data = NULL;
            }

            if(uartstructer_uart_send_buf.ui8_addr == NULL){
                uartstructer_uart_send_buf.ui8_addr = (uint8_t*)malloc(sizeof(uint8_t) * (*uartstructer_uart_send_buf.ui8_addr_length));
                if(uartstructer_uart_send_buf.ui8_addr == NULL){
                    printf("malloc failed: uartstructer_uart_send_buf.ui8_addr\n");
                    free(uartstructer_uart_send_buf.ui8_addr);
                    return ERROR_MALLOC_FAIL;
                }
                memset(uartstructer_uart_send_buf.ui8_addr, 0, sizeof(uint8_t) * (*uartstructer_uart_send_buf.ui8_addr_length));
            }
            if(uartstructer_uart_send_buf.ui8_data == NULL){
                uartstructer_uart_send_buf.ui8_data = (uint8_t*)malloc(sizeof(uint8_t) * (*uartstructer_uart_send_buf.ui8_data_length));
                if(uartstructer_uart_send_buf.ui8_data == NULL){
                    printf("malloc failed: uartstructer_uart_send_buf.ui8_data\n");
                    free(uartstructer_uart_send_buf.ui8_data);
                    return ERROR_MALLOC_FAIL;
                }
                memset(uartstructer_uart_send_buf.ui8_data, 0, sizeof(uint8_t) * (*uartstructer_uart_send_buf.ui8_data_length));
            }

            /// 8bit로 쪼개기
            for(uint8_t i = 0; i < *uartstructer_uart_send_buf.ui8_addr_length; i++){
                uartstructer_uart_send_buf.ui8_addr[i] = ((A_ui32_spi_recive_reg_q_recv_64bit[0] & (0xFF << (8 * i))) >> (8 * i));
            }
            for(uint8_t i = 0; i < *uartstructer_uart_send_buf.ui8_data_length; i++){
                uartstructer_uart_send_buf.ui8_data[i] = ((A_ui32_spi_recive_reg_q_recv_64bit[1] & (0xFF << (8 * i))) >> (8 * i));
            }

            *uartstructer_uart_send_buf.ui8_chksum  = checksum(&uartstructer_uart_send_buf);
            uart_tx_transaction(&uartstructer_uart_send_buf);

            free(uartstructer_uart_send_buf.ui8_addr);
            uartstructer_uart_send_buf.ui8_addr = NULL;
            free(uartstructer_uart_send_buf.ui8_data);
            uartstructer_uart_send_buf.ui8_data = NULL;
        }
        vTaskDelay(1);
    }
    // Tread 반복

    // 자원 반환
    free_uart_structer(&uartstructer_uart_send_buf);
    for (int j = 0; j < 64; j++) {
        if (ui8_frame_q_recv_32768bit[j]) {
            free(ui8_frame_q_recv_32768bit[j]);
            ui8_frame_q_recv_32768bit[j] = NULL;
        }
    }
    free(ui8_frame_q_recv_32768bit);
    ui8_frame_q_recv_32768bit = NULL;
    free(ui8_spi_reg_q_recv_32bit);
    ui8_spi_reg_q_recv_32bit = NULL;
}

int uart_init(void){

    view_heap_stack("uart_init start");
// uart_init start - 사용가능 heap : 317660         차이 : 0
// uart_init start - 연속 heap : 253952     차이 : 0
// uart_init start - 사용 stack : 1848      차이 : 0

    // UART 초기 세팅
    const uart_config_t uart_config = {
        .baud_rate = BAUD_RATE_SEL,             // 보레잇 설정
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    // UART
    // uart_driver_install(UART_NUM_1, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_driver_install(UART_NUM_1, UART_BUFFER_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

//     // USB
//     const tinyusb_config_t tusb_cfg = {
//         .device_descriptor  = NULL,
//         .string_descriptor  = NULL,
//         .external_phy       = false, // In the most cases you need to use a `false` value
// #if (TUD_OPT_HIGH_SPEED)
//         .fs_configuration_descriptor = NULL,
//         .hs_configuration_descriptor = NULL,
//         .qualifier_descriptor = NULL,
// #else
//         .configuration_descriptor = NULL,
// #endif // TUD_OPT_HIGH_SPEED
//     };
//     ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));
//     tinyusb_config_cdcacm_t acm_cfg = { 0 }; // the configuration uses default values
//     ESP_ERROR_CHECK(tusb_cdc_acm_init(&acm_cfg));

    xTaskCreate(uart_rx_thread, "UART_Recive_TASK_THREAD", UART_RX_STACK_SIZE, NULL, configMAX_PRIORITIES - 1, NULL); // 5
    // UART 수신 데이터 재구성 스레드 시작
    xTaskCreate(uart_rx_data_process, "UART_DATA_PROCESS_THREAD", UART_RX_DATA_PROCESS_STACK_SIZE, NULL, configMAX_PRIORITIES - 1, NULL);
    xTaskCreate(uart_tx_thread, "UART_Send_TASK_THREAD", UART_TX_STACK_SIZE, NULL, configMAX_PRIORITIES - 1, NULL);   // 8


    view_heap_stack("uart_init end");
// uart_init end - 사용가능 heap : 289864   차이 : 27796
// uart_init end - 연속 heap : 225280       차이 : 28672
// uart_init end - 사용 stack : 1848        차이 : 0

    return 0;
}