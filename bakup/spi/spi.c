////////////////////////////////////////////////////////////////////////////////////////////////
// 1. SPI 구조체 초기화
// 2. SPI 구조체 해제
// 3. CRC8 Transaction
// 4. CRC8 Process

// 5. SPI Frame 수신 [ISP] Chip -> ESP
// 6. Frame 재구성
    // 반복

// 7. SPI 1Byte 수신 [CPU] Chip -> ESP
// 8. SPI 1Byte 송신 [CPU] Chip <- ESP
// 9. SPI CMD 수신 Transaction [CPU] Chip -> ESP
    // a. Sync Process -> b. Signal -> c. CMD -> d. Addr_length -> e. ADDR -> f. Data_length -> g. Data -> h. CRC8 -> i. Dummy
// 10. SPI CMD 수신 Data Setting (수신 공간 할당)
// 11. SPI CMD 송신 Transaction [CPU] Chip <- ESP
    // a. Sync Process -> b. Signal -> c. CMD -> d. Addr_length -> e. ADDR -> f. Data_length -> g. Data -> h. CRC8 -> i. Dummy
// 12. SPI CMD 송신 Data Setting (송신 공간 할당 및 데이터 세팅)

// 13. Uart Recive CMD Data Process
    // 반복

// 14. SPI 초기화
////////////////////////////////////////////////////////////////////////////////////////////////

#include "spi/spi.h"

// from queue.c
extern esp_err_t esp_err;
extern QueueHandle_t Q_ui64_spi_recive_image_320bit;
extern QueueHandle_t Q_ui8_frame_32768bit;
extern QueueHandle_t Q_ui32_cmd_128bit;
extern QueueHandle_t Q_ui32_spi_recive_reg_64bit;

// 1. SPI 구조체 초기화
static bool init_spi_structer(spi_structer* buf) {
    bool success = true;

    buf->ui8_signal      = (uint8_t*)malloc(sizeof(uint8_t));
    buf->ui8_cmd         = (uint8_t*)malloc(sizeof(uint8_t));
    buf->ui8_addr_length = (uint8_t*)malloc(sizeof(uint8_t));
    buf->ui8_addr        = (uint8_t*)malloc(sizeof(uint8_t));  // 초기값 1바이트
    buf->ui8_data_length = (uint8_t*)malloc(sizeof(uint8_t));
    buf->ui8_data        = (uint8_t*)malloc(sizeof(uint8_t));  // 초기값 1바이트
    buf->ui8_crc8        = (uint8_t*)malloc(sizeof(uint8_t));
    buf->ui8_dummy       = (uint8_t*)malloc(sizeof(uint8_t));

    success &= (buf->ui8_signal      != NULL);
    success &= (buf->ui8_cmd         != NULL);
    success &= (buf->ui8_addr_length != NULL);
    success &= (buf->ui8_addr        != NULL);
    success &= (buf->ui8_data_length != NULL);
    success &= (buf->ui8_data        != NULL);
    success &= (buf->ui8_crc8        != NULL);
    success &= (buf->ui8_dummy       != NULL);

    if (success) {
        memset(buf->ui8_signal,      0, sizeof(uint8_t));
        memset(buf->ui8_cmd,         0, sizeof(uint8_t));
        memset(buf->ui8_addr_length, 0, sizeof(uint8_t));
        memset(buf->ui8_addr,        0, sizeof(uint8_t));
        memset(buf->ui8_data_length, 0, sizeof(uint8_t));
        memset(buf->ui8_data,        0, sizeof(uint8_t));
        memset(buf->ui8_crc8,        0, sizeof(uint8_t));
        memset(buf->ui8_dummy,       0, sizeof(uint8_t));
    }
    return success;
}

// 2. SPI 구조체 해제
static void free_spi_structer(spi_structer* buf) {
    free(buf->ui8_signal);
    free(buf->ui8_cmd);
    free(buf->ui8_addr_length);
    free(buf->ui8_addr);
    free(buf->ui8_data_length);
    free(buf->ui8_data);
    free(buf->ui8_crc8);
    free(buf->ui8_dummy);
}

// 3. CRC8 Transaction
    // 체크섬, CRC
    // CRC-8 다항식 (x^8 + x^2 + x^1 + 1, 0x07)
static uint8_t crc8_proc(uint8_t ui8_crc8_value){
    for (int i = 0; i < 8; i++) {
        if (ui8_crc8_value & 0x80){
            ui8_crc8_value = (ui8_crc8_value << 1) ^ 0x07;
        }
        else{
            ui8_crc8_value <<= 1;
        }
    }
    return ui8_crc8_value;
}

// 4. CRC8 Process
static uint8_t crc8(spi_structer* spistructer_spi_buf){
    uint8_t ui8_crc = 0;
    ui8_crc = crc8_proc(ui8_crc ^ *spistructer_spi_buf->ui8_signal);
    ui8_crc = crc8_proc(ui8_crc ^ *spistructer_spi_buf->ui8_cmd);
    ui8_crc = crc8_proc(ui8_crc ^ *spistructer_spi_buf->ui8_addr_length);
    for(size_t i = 0; i < *spistructer_spi_buf->ui8_addr_length; i++){
        ui8_crc = crc8_proc(ui8_crc ^ spistructer_spi_buf->ui8_addr[i]);
    }
    ui8_crc = crc8_proc(ui8_crc ^ *spistructer_spi_buf->ui8_data_length);
    for(size_t i = 0; i < *spistructer_spi_buf->ui8_data_length; i++) {
        ui8_crc = crc8_proc(ui8_crc ^ spistructer_spi_buf->ui8_data[i]);
    }
    return ui8_crc;
}

// 5. SPI Frame 수신(320bit) [ISP] Chip -> ESP
    // SPI 이미지 수신 데이터 Queue 입력 -> 재구성
    // spi_image_rx_thread -> spi_image_rx_data_process
static void spi_image_rx_thread(void *arg)
{
    uint64_t A_ui64_spi_sendbuf[5] = {0};                                                           // SPI 전송 버퍼(64 * 5) = 320bit    
    uint64_t A_ui64_spi_recvbuf[5] = {0};                                                           // SPI 수신 버퍼(64 * 5) = 320bit

    // SPI Slave 트랜잭션 생성
    spi_slave_transaction_t spi_image_transaction;
    spi_image_transaction.length    = SPI_IMAGE_SPI_BUSTER_SIZE;                                    // (8 * 8 * 5) = 320bit
    spi_image_transaction.tx_buffer = A_ui64_spi_sendbuf;                                           // 전송 버퍼 연결
    spi_image_transaction.rx_buffer = A_ui64_spi_recvbuf;                                           // 수신 버퍼 연결

    // SPI 수신 데이터(320bit) 큐에 입력
    while(1){   // Thread 반복
        // SPI 이미지 데이터 수신 공간 초기화
        memset(A_ui64_spi_recvbuf, 0, sizeof(uint64_t) * 5);                                        // Read Buffer Clear
        // SPI 통신 시작
        esp_err = spi_slave_transmit(SPI_IMAGE_RCV_HOST, &spi_image_transaction, portMAX_DELAY);    // 64bit * 5 Recive = 320bit
        assert(esp_err == ESP_OK);
        // SPI 수신 데이터 Queue 입력
        xQueueSend(Q_ui64_spi_recive_image_320bit, A_ui64_spi_recvbuf, 0);
        // view_heap_stack("spi_image_rx_thread");
        
///////////////////
        // for(int i_count = 0; i_count < 5; i_count++){
        //     printf("A_ui64_spi_recvbuf[%d] : %16lli\n", i_count, A_ui64_spi_recvbuf[i_count]);
        // }
        //  printf("\n");
///////////////////
    }
}



// 6. Frame 재구성
    // SPI 이미지 수신 데이터 재구성 -> 1Frame -> Queue 입력
    // 반복
uint64_t reverse_bytes_uint64(uint64_t val) {
    uint64_t result = 0;
    for (int i = 0; i < 8; i++) {
        result <<= 8;
        result |= (val & 0xFF);
        val >>= 8;
    }
    return result;
}

static void spi_image_rx_data_process(void){

    // SPI 수신 데이터 버퍼(128 * 5) = 1Frame
    //////////          //
    // 64bit            //
    // 64bit            //
    // 64bit    320bit  //
    // 64bit            //
    // 64bit            //
    //////////          //
    //  .               //
    //  .               //  128개   =   1Frame
    //  .               //
    //////////          //
    // 64bit            //
    // 64bit            //
    // 64bit    320bit  //
    // 64bit            //
    // 64bit            //
    //////////          //

    uint8_t**   ui8_frame_q_send_32768bit;                  // 만들어진 Image Queue 송신 버퍼(1 Frame)
    uint64_t    ui64_spi_recv_q_recv_320bit[5]  = {0,};
    int         i_recive_image_addr             = 0;
    bool        b_frame_start_signal            = false;
    int         i_frame_addr_guide              = 0;

    // 이미지 수신 공간 확보 ///////////////////////////////
    // SPI 이미지 수신 데이터 버퍼 생성
    ui8_frame_q_send_32768bit = (uint8_t**)malloc(sizeof(uint8_t*) * 64);
    if (ui8_frame_q_send_32768bit == NULL) {
        printf("malloc failed: ui8_frame_q_send_32768bit\n");
        free(ui8_frame_q_send_32768bit);
        return ERROR_MALLOC_FAIL;
    }
    for (int i_low = 0; i_low < 64; i_low++){
        ui8_frame_q_send_32768bit[i_low] = (uint8_t*)malloc(sizeof(uint8_t) * 64);
        // 공간 확보 실패
        if (ui8_frame_q_send_32768bit[i_low] == NULL) {
            printf("malloc failed: ui8_frame_q_send_32768bit[%d]\n", i_low);
            for (int i_low_del = 0; i_low_del <= i_low; i_low_del++) {
                if (ui8_frame_q_send_32768bit[i_low_del]) {
                    free(ui8_frame_q_send_32768bit[i_low_del]);
                }
            }
            free(ui8_frame_q_send_32768bit);
            ui8_frame_q_send_32768bit = NULL;
            return ERROR_MALLOC_FAIL;
        }
        memset(ui8_frame_q_send_32768bit[i_low], 0, sizeof(uint8_t) * 64);
    }
    // 이미지 수신 공간 확보 ///////////////////////////////
    // SPI 수신 이미지 데이터 확인 및 Frame Queue 전송 //////
    // Thread 반복
    while(1){
        // SPI 이미지 수신 데이터가 있다면(320bit)
        if(uxQueueMessagesWaiting(Q_ui64_spi_recive_image_320bit) > 0){
            // Frame[i] 수신 ////////////////////////
            memset(ui64_spi_recv_q_recv_320bit, 0, sizeof(uint64_t) * 5);                               // Queue 데이터 수신 버퍼 초기화
            xQueueReceive(Q_ui64_spi_recive_image_320bit, ui64_spi_recv_q_recv_320bit, portMAX_DELAY);  // Queue 데이터 꺼내기
            
///////////////
            // for(int i_count = 0; i_count < 5; i_count++){
            //     printf("ui64_spi_recv_q_recv_320bit[%d] : %16llX\n", i_count, ui64_spi_recv_q_recv_320bit[i_count]);
            // }
            // printf("ui64_spi_recv_q_recv_320bit[0] : %016llX :: \n", ui64_spi_recv_q_recv_320bit[0]);
            // printf("\n");


            // 자리 바꿔야함
            // 현재 250722
            // ui64_spi_recv_q_recv_320bit[0] : 00000000000000FD :: 
            // ui64_spi_recv_q_recv_320bit[0] : 00000000000000FE ::
            // ui64_spi_recv_q_recv_320bit[0] : 00000000000000FF ::
            // ui64_spi_recv_q_recv_320bit[0] : 0000000000000080 ::
            // ui64_spi_recv_q_recv_320bit[0] : 0000000000000081 :: 
            // 타겟
            // ui64_spi_recv_q_recv_320bit[0] : 8000000000000000 ::
            // ui64_spi_recv_q_recv_320bit[0] : 8100000000000000 :: 
            for (int i = 0; i < 5; i++) {
                ui64_spi_recv_q_recv_320bit[i] = reverse_bytes_uint64(ui64_spi_recv_q_recv_320bit[i]);
            }
            // printf("ui64_spi_recv_q_recv_320bit[0] : %016llX :: \n", ui64_spi_recv_q_recv_320bit[0]);
            // printf("Queue Use : %d\n", uxQueueMessagesWaiting(Q_ui64_spi_recive_image_320bit));
///////////////

            // CMD가 1이라면 -> WRITE_SIGNAL
            // (1XXXXXXX_XXXXXXXX) (NNNNNNNN_NNNNNNNN) (NNNNNNNN_NNNNNNN) (NNNNNNNN_NNNNNNNN) (NNNNNNNN_NNNNNNNN)
            if(ui64_spi_recv_q_recv_320bit[0] & 1ULL << 63){

                // ADDR 확인
                // (X1111111_XXXXXXXX) (NNNNNNNN_NNNNNNNN) (NNNNNNNN_NNNNNNN) (NNNNNNNN_NNNNNNNN) (NNNNNNNN_NNNNNNNN)
                i_recive_image_addr = (ui64_spi_recv_q_recv_320bit[0] & 127ULL << 56) >> 56;

                // printf("i_recive_image_addr : [%d]\n", i_recive_image_addr);

                if((i_recive_image_addr == 0) && (b_frame_start_signal)){   // Frame 수신은 시작했는데, 마지막 ADDR(127)을 수신받지 못하였을 때
                    // 현재 Frame 마무리 후 Queue Push
                    b_frame_start_signal = false;
                    xQueueSend(Q_ui8_frame_32768bit, ui8_frame_q_send_32768bit, 0);
                    // printf("Already ui8_frame_q_send_32768bit Send Queue\n");
                }
                if(i_recive_image_addr == 0){                               // ADDR이 0이라면
                    // 새로운 Frame 시작
                    b_frame_start_signal = true;
                    i_frame_addr_guide = i_recive_image_addr;
                }
                if(i_recive_image_addr != i_frame_addr_guide){              // 가이드 주소랑 ADDR이 다르다면
                    printf("not match ADDR[Guid : %d, Addr : %d]\n", i_frame_addr_guide, i_recive_image_addr);
                    i_frame_addr_guide = i_recive_image_addr;
                }
                for(int i_sel_64bit = 1; i_sel_64bit < 5; i_sel_64bit++){   // ADDR 위치에 맞게 Frame 재구성
                    for(int i_sel_8bit = 0; i_sel_8bit < 8; i_sel_8bit++){
                        ui8_frame_q_send_32768bit[i_recive_image_addr / 2][((i_recive_image_addr % 2) * 32) + ((i_sel_64bit - 1) * 8) + i_sel_8bit] = ((ui64_spi_recv_q_recv_320bit[i_sel_64bit] & (0xFFULL << (8 * (7 - i_sel_8bit)))) >> (8 * (7 - i_sel_8bit)));
                    }
                }
                if (i_recive_image_addr == 127){                            // ADDR이 127이라면
                    // 현재 Frame 마무리 후 Queue Push
                    b_frame_start_signal = false;
                    xQueueSend(Q_ui8_frame_32768bit, ui8_frame_q_send_32768bit, 0);
                    // printf("ui8_frame_q_send_32768bit Send Queue\n");
                }
            }   // CMD가 1이라면 -> WRITE_SIGNAL
            i_frame_addr_guide++;
        }else{
            vTaskDelay(1);
        }
    }
    // Thread 반복
    // SPI 수신 이미지 데이터 확인 및 Frame Queue 전송 //////

    // 공간 삭제
    for (int i_low_del = 0; i_low_del < 64; i_low_del++) {
        if (ui8_frame_q_send_32768bit[i_low_del]) {
            free(ui8_frame_q_send_32768bit[i_low_del]);
            ui8_frame_q_send_32768bit[i_low_del] = NULL;
        }
    }
    free(ui8_frame_q_send_32768bit);
    ui8_frame_q_send_32768bit = NULL;
    // 공간 삭제
}

// 7. SPI 1Byte 수신 [CPU] Chip -> ESP
static void spi_receive_single_byte(spi_slave_transaction_t* spi_transaction, uint8_t* ui8_buf_value){
    esp_err = spi_slave_transmit(SPI_CMD_RCV_HOST, spi_transaction, portMAX_DELAY);
    assert(esp_err == ESP_OK);
    *ui8_buf_value = *(uint8_t*)spi_transaction->rx_buffer;
}

// 8. SPI 1Byte 송신 [CPU] Chip <- ESP
static void spi_send_single_byte(spi_slave_transaction_t* spi_transaction, uint8_t* ui8_buf_value){
    *(uint8_t*)spi_transaction->tx_buffer = *ui8_buf_value;
    esp_err = spi_slave_transmit(SPI_CMD_RCV_HOST, spi_transaction, portMAX_DELAY);
    assert(esp_err == ESP_OK);
}

// 9. SPI CMD 수신 Transaction [CPU] Chip -> ESP
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // a. Sync Process -> b. Signal -> c. CMD -> d. ADDR_length -> e. ADDR -> f. Data_length -> g. Data -> h. CRC8 -> i. Dummy  //
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static uint8_t spi_cmd_rx_transaction(spi_slave_transaction_t* spi_transaction, spi_structer* spistructer_spi_recv_buf){
    uint8_t ui8_send_return = 0;

    // a. Sync Process
    while(*(uint8_t*)spi_transaction->rx_buffer != SPI_FIRST_WORD){
        memset(spi_transaction->rx_buffer, 0, sizeof(uint8_t));             // Write Buffer Clear
        spi_send_single_byte(spi_transaction, spi_transaction->rx_buffer);
    }
    spi_send_single_byte(spi_transaction, spi_transaction->rx_buffer);
    // printf("SPI CMD RX Sync Done\n");
    // re spi_transaction->rx_buffer : A5
    // re spi_transaction->tx_buffer : A5

    // b. Signal
    spi_receive_single_byte(spi_transaction, spistructer_spi_recv_buf->ui8_signal);
    // printf("SPI Signal Recive\n");
    // re spi_transaction->rx_buffer : DD
    // re spi_transaction->tx_buffer : A5

    // c. CMD
    spi_receive_single_byte(spi_transaction, spistructer_spi_recv_buf->ui8_cmd);
    // printf("SPI CMD Recive\n");
    // re spi_transaction->rx_buffer : 00
    // re spi_transaction->tx_buffer : A5

    // d. ADDR_length
    spi_receive_single_byte(spi_transaction, spistructer_spi_recv_buf->ui8_addr_length);
    // printf("SPI ADDR_length Recive\n");
    // re spi_transaction->rx_buffer : 01
    // re spi_transaction->tx_buffer : A5

    // e. ADDR
    if(spistructer_spi_recv_buf->ui8_addr != NULL){
        // printf("malloc ui8_addr\n");
        free(spistructer_spi_recv_buf->ui8_addr);
        spistructer_spi_recv_buf->ui8_addr = NULL;
    }
    spistructer_spi_recv_buf->ui8_addr = (uint8_t*)malloc(sizeof(uint8_t) * (*(uint8_t*)spistructer_spi_recv_buf->ui8_addr_length));
    if (spistructer_spi_recv_buf->ui8_addr == NULL){
        printf("malloc failed: spistructer_spi_recv_buf.ui8_addr\n");
        free(spistructer_spi_recv_buf->ui8_addr);
        return ERROR_MALLOC_FAIL;
    }
    for(int i = 0; i < *spistructer_spi_recv_buf->ui8_addr_length; i++){
        spi_receive_single_byte(spi_transaction, spistructer_spi_recv_buf->ui8_addr + i);
        // printf("addr spi_transaction->rx_buffer : %02X\n", *(uint8_t*)spi_transaction->rx_buffer);
        // printf("addr spi_transaction->tx_buffer : %02X\n", *(uint8_t*)spi_transaction->tx_buffer);
    }
    // printf("SPI ADDR Recive\n");
    // re spi_transaction->rx_buffer : 00
    // re spi_transaction->tx_buffer : A5

    // f. Data_length
    spi_receive_single_byte(spi_transaction, spistructer_spi_recv_buf->ui8_data_length);
    // printf("SPI Data_length Recive\n");
    // re spi_transaction->rx_buffer : 04
    // re spi_transaction->tx_buffer : A5

    // g. Data
    if(spistructer_spi_recv_buf->ui8_data != NULL){
        // printf("malloc ui8_data\n");
        free(spistructer_spi_recv_buf->ui8_data);
        spistructer_spi_recv_buf->ui8_data = NULL;
    }
    spistructer_spi_recv_buf->ui8_data = (uint8_t*)malloc(sizeof(uint8_t) * (*(uint8_t*)spistructer_spi_recv_buf->ui8_data_length));
    if (spistructer_spi_recv_buf->ui8_data == NULL){
        printf("malloc failed: spistructer_spi_recv_buf.ui8_data\n");
        free(spistructer_spi_recv_buf->ui8_data);
        spistructer_spi_recv_buf->ui8_data = NULL;
        return ERROR_MALLOC_FAIL;
    }
    for(int i = 0; i < *spistructer_spi_recv_buf->ui8_data_length; i++){
        spi_receive_single_byte(spi_transaction, spistructer_spi_recv_buf->ui8_data + i);
        // printf("data spi_transaction->rx_buffer : %02X\n", *(uint8_t*)spi_transaction->rx_buffer);
        // printf("data spi_transaction->tx_buffer : %02X\n", *(uint8_t*)spi_transaction->tx_buffer);
    }
    // printf("SPI Data Recive\n");
    // re spi_transaction->rx_buffer : 03
    // re spi_transaction->tx_buffer : A5

    // h. CRC8
    spi_receive_single_byte(spi_transaction, spistructer_spi_recv_buf->ui8_crc8);
    // printf("SPI CRC8 Recive\n");
    // re spi_transaction->rx_buffer : A9
    // re spi_transaction->tx_buffer : A5

    // i. Dummy
    spi_receive_single_byte(spi_transaction, spistructer_spi_recv_buf->ui8_dummy);
    // printf("SPI Dummy Recive\n");
    // re spi_transaction->rx_buffer : 00
    // re spi_transaction->tx_buffer : A5

// CRC Check
    if(crc8(spistructer_spi_recv_buf) == *spistructer_spi_recv_buf->ui8_crc8){
        // printf("SPI Recive CRC8 Done\n");
        ui8_send_return = SPI_FIRST_WORD;
        // *(uint8_t*)spi_transaction->tx_buffer = ui8_send_return;
        // spi_send_single_byte(spi_transaction, spi_transaction->tx_buffer);
    }
    else{
        // printf("SPI Recive CRC8 Not Done\n");
        ui8_send_return = SPI_WRONG_WORD;
        // *(uint8_t*)spi_transaction->tx_buffer = ui8_send_return;
        // spi_send_single_byte(spi_transaction, spi_transaction->tx_buffer);
    }
    *(uint8_t*)spi_transaction->tx_buffer = ui8_send_return;
    spi_send_single_byte(spi_transaction, spi_transaction->tx_buffer);

    return ui8_send_return;
}

// 10. SPI CMD 수신 Data Setting (수신 공간 할당)
static uint8_t spi_rx_CMD_data_setting(spi_structer* spistructer_spi_recv_buf){
    
    uint8_t ui8_return_value = 0;

    // CMD 수신 공간 확보 ///////////////////////////////
    uint8_t* ui8_spi_sendbuf = (uint8_t*)malloc(sizeof(uint8_t));
    if (ui8_spi_sendbuf == NULL) {
        printf("malloc failed: ui8_spi_sendbuf\n");
        free(ui8_spi_sendbuf);
        return ERROR_MALLOC_FAIL;
    }
    memset(ui8_spi_sendbuf, 0, sizeof(uint8_t));
    uint8_t* ui8_spi_recvbuf = (uint8_t*)malloc(sizeof(uint8_t));
    if (ui8_spi_recvbuf == NULL) {
        printf("malloc failed: ui8_spi_recvbuf\n");
        free(ui8_spi_recvbuf);
        return ERROR_MALLOC_FAIL;
    }
    memset(ui8_spi_recvbuf, 0, sizeof(uint8_t));
    
    spi_slave_transaction_t spi_cmd_transaction;
    spi_cmd_transaction.length      = SPI_CMD_SPI_BUSTER_SIZE;  // 8bit
    spi_cmd_transaction.tx_buffer   = ui8_spi_sendbuf;          // 전송 버퍼 연결
    spi_cmd_transaction.rx_buffer   = ui8_spi_recvbuf;          // 수신 버퍼 연결

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // a. Sync Process -> b. Signal -> c. CMD -> d. ADDR_length -> e. ADDR -> f. Data_length -> g. Data -> h. CRC8 -> i. Dummy  //
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    ui8_return_value = spi_cmd_rx_transaction(&spi_cmd_transaction, spistructer_spi_recv_buf);
    // SPI_FIRST_WORD or SPI_WRONG_WORD
    // printf("spi_rx_CMD_data_setting ui8_return_value(<-spi_cmd_rx_transaction) : %02X\n", ui8_return_value);

    free(ui8_spi_sendbuf);
    ui8_spi_sendbuf = NULL;
    free(ui8_spi_recvbuf);
    ui8_spi_recvbuf = NULL;

    return ui8_return_value;
}

// 11. SPI CMD 송신 Transaction [CPU] Chip <- ESP
static uint8_t spi_tx_transaction(spi_slave_transaction_t* spi_transaction, spi_structer* spistructer_spi_send_buf){
    uint8_t ui8_send_return = 0;
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // a. Sync Process -> b. Signal -> c. CMD -> d. ADDR_length -> e. ADDR -> f. Data_length -> g. Data -> h. CRC8 -> i. Dummy  //
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // a. Sync Process
    while(*(uint8_t*)spi_transaction->rx_buffer != SPI_FIRST_WORD){
        memset(spi_transaction->rx_buffer, 0, sizeof(uint8_t));
        spi_send_single_byte(spi_transaction, spi_transaction->rx_buffer);
    }
    spi_send_single_byte(spi_transaction, spi_transaction->rx_buffer);
    // printf("SPI TX Sync Done\n");
    // spi_transaction->rx_buffer : A5
    // spi_transaction->tx_buffer : A5

    // b. Signal
    spi_send_single_byte(spi_transaction, spistructer_spi_send_buf->ui8_signal);
    // printf("SPI Signal\n");
    // spi_transaction->rx_buffer : A5
    // spi_transaction->tx_buffer : DD

    // c. CMD
    spi_send_single_byte(spi_transaction, spistructer_spi_send_buf->ui8_cmd);
    // printf("SPI CMD\n");
    // printf("spi_transaction->rx_buffer : %02X\n", *(uint8_t*)spi_transaction->rx_buffer);
    // printf("spi_transaction->tx_buffer : %02X\n", *(uint8_t*)spi_transaction->tx_buffer);
    // spi_transaction->rx_buffer : DD
    // spi_transaction->tx_buffer : CMD

    // d. ADDR_length
    spi_send_single_byte(spi_transaction, spistructer_spi_send_buf->ui8_addr_length);
    // printf("SPI ADDR len\n");
    // spi_transaction->rx_buffer : CMD
    // spi_transaction->tx_buffer : ADDR len

    // e. ADDR
    for(int i = 0; i < *spistructer_spi_send_buf->ui8_addr_length; i++){
        spi_send_single_byte(spi_transaction, spistructer_spi_send_buf->ui8_addr + i);
    }
    // printf("SPI ADDR\n");
    // spi_transaction->rx_buffer : ADDR - 1
    // spi_transaction->tx_buffer : ADDR

    // f. Data_length
    spi_send_single_byte(spi_transaction, spistructer_spi_send_buf->ui8_data_length);
    // printf("SPI DATA len\n");
    // spi_transaction->rx_buffer : ADDR
    // spi_transaction->tx_buffer : DATA len

    // g. Data
    for(int i = 0; i < *spistructer_spi_send_buf->ui8_data_length; i++){
        spi_send_single_byte(spi_transaction, spistructer_spi_send_buf->ui8_data + i);
    }
    // printf("SPI DATA\n");
    // spi_transaction->rx_buffer : DATA - 1
    // spi_transaction->tx_buffer : DATA

    // h. CRC8
    spi_send_single_byte(spi_transaction, spistructer_spi_send_buf->ui8_crc8);
    // printf("SPI CRC8\n");
    // spi_transaction->rx_buffer : DATA
    // spi_transaction->tx_buffer : CRC8

    // i. Dummy
    spi_send_single_byte(spi_transaction, spistructer_spi_send_buf->ui8_dummy);
    // printf("SPI Dummy\n");
    // spi_transaction->rx_buffer : CRC8
    // spi_transaction->tx_buffer : Dummy

    // [Chip] -> Dummy
    // Dummy <- [ESP]
    spi_send_single_byte(spi_transaction, spi_transaction->rx_buffer);
    // printf("SPI chk\n");
    // spi_transaction->rx_buffer : Dummy
    // spi_transaction->tx_buffer : Dummy + 1

    // [Chip] -> CRC8
        // 정상 SPI_FIRST_WORD
        // 비정상 SPI_WRONG_WORD
    spi_send_single_byte(spi_transaction, spi_transaction->rx_buffer);
    // spi_transaction->rx_buffer : A5
    // spi_transaction->tx_buffer : Dummy
    ui8_send_return = *(uint8_t*)spi_transaction->rx_buffer;
    printf("Chip CRC8 recive(A5) : %02X\n", ui8_send_return);
    // printf("CRC8 : %02X\n", *(uint8_t*)spistructer_spi_send_buf->ui8_crc8);
    return ui8_send_return;
}

// 12. SPI CMD 송신 Data Setting (송신 공간 할당 및 데이터 세팅)
static uint8_t spi_tx_CMD_data_setting(uint32_t* A_ui32_cmd_q_recv_128bit){
    uint8_t ui8_return_value = 0;

    // CMD 송신 공간 확보 ////////////////////////////////
    uint8_t* ui8_spi_sendbuf = (uint8_t*)malloc(sizeof(uint8_t));
    if (ui8_spi_sendbuf == NULL) {
        printf("malloc failed: ui8_spi_sendbuf\n");
        free(ui8_spi_sendbuf);
        return ERROR_MALLOC_FAIL;
    }
    memset(ui8_spi_sendbuf, 0, sizeof(uint8_t));

    uint8_t* ui8_spi_recvbuf = (uint8_t*)malloc(sizeof(uint8_t));
    if (ui8_spi_recvbuf == NULL) {
        printf("malloc failed: ui8_spi_recvbuf\n");
        free(ui8_spi_recvbuf);
        return ERROR_MALLOC_FAIL;
    }
    memset(ui8_spi_recvbuf, 0, sizeof(uint8_t));
    
    spi_slave_transaction_t spi_cmd_transaction;
    spi_cmd_transaction.length = SPI_CMD_SPI_BUSTER_SIZE;   // 8bit
    spi_cmd_transaction.tx_buffer = ui8_spi_sendbuf;        // 전송 버퍼 연결
    spi_cmd_transaction.rx_buffer = ui8_spi_recvbuf;        // 수신 버퍼 연결

    spi_structer spistructer_spi_send_buf;                  // SPI 규격
    if(!init_spi_structer(&spistructer_spi_send_buf)){
        printf("malloc failed: spisend_send_buf\n");
        free_spi_structer(&spistructer_spi_send_buf);
        return ERROR_MALLOC_FAIL;
    }

    // b. Signal
    *spistructer_spi_send_buf.ui8_signal         = CMD_SIGNAL;
    // c. CMD
    *spistructer_spi_send_buf.ui8_cmd            = WRITE_CMD;
    // d. ADDR_length [4줄]
    *spistructer_spi_send_buf.ui8_addr_length    = (uint8_t*)A_ui32_cmd_q_recv_128bit[0];
    if(spistructer_spi_send_buf.ui8_addr != NULL){
        free(spistructer_spi_send_buf.ui8_addr);
        spistructer_spi_send_buf.ui8_addr = NULL;
    }
    // e. ADDR
    spistructer_spi_send_buf.ui8_addr = (uint8_t*)malloc(sizeof(uint8_t) * (*spistructer_spi_send_buf.ui8_addr_length));
    if(spistructer_spi_send_buf.ui8_addr == NULL){
        printf("malloc failed: spistructer_spi_send_buf.ui8_addr\n");
        free(spistructer_spi_send_buf.ui8_addr);
        return ERROR_MALLOC_FAIL;
    }
    memset(spistructer_spi_send_buf.ui8_addr, 0, sizeof(uint8_t) * (*spistructer_spi_send_buf.ui8_addr_length));
    for (int i = 0; i < *spistructer_spi_send_buf.ui8_addr_length; i++) {
        spistructer_spi_send_buf.ui8_addr[i] = (uint8_t)((A_ui32_cmd_q_recv_128bit[1] >> (8 * i)) & 0xFF);
    }
    // f. Data_length [4줄]
    *spistructer_spi_send_buf.ui8_data_length    = (uint8_t*)A_ui32_cmd_q_recv_128bit[2];
    if(spistructer_spi_send_buf.ui8_data != NULL){
        // printf("spistructer_spi_send_buf.ui8_data not NULL\n");
        free(spistructer_spi_send_buf.ui8_data);
        spistructer_spi_send_buf.ui8_data = NULL;
    }
    // g. Data
    spistructer_spi_send_buf.ui8_data = (uint8_t*)malloc(sizeof(uint8_t) * (*spistructer_spi_send_buf.ui8_data_length));
    if(spistructer_spi_send_buf.ui8_data == NULL){
        printf("malloc failed: spistructer_spi_send_buf.ui8_data\n");
        free(spistructer_spi_send_buf.ui8_data);
        return ERROR_MALLOC_FAIL;
    }
    memset(spistructer_spi_send_buf.ui8_data, 0, sizeof(uint8_t) * (*spistructer_spi_send_buf.ui8_data_length));
    for (int i = 0; i < *spistructer_spi_send_buf.ui8_data_length; i++) {
        spistructer_spi_send_buf.ui8_data[i] = (uint8_t)((A_ui32_cmd_q_recv_128bit[3] >> (8 * i)) & 0xFF);
    }
    // h. CRC8
    *spistructer_spi_send_buf.ui8_crc8 = crc8(&spistructer_spi_send_buf);
    // i. Dummy
    *spistructer_spi_send_buf.ui8_dummy = 0;
    do{
        gpio_isr_on();  // GPIO INTERRUPT ON
        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // a. Sync Process -> b. Signal -> c. CMD -> d. ADDR_length -> e. ADDR -> f. Data_length -> g. Data -> h. CRC8 -> i. Dummy  //
        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        ui8_return_value = spi_tx_transaction(&spi_cmd_transaction, &spistructer_spi_send_buf);
        // printf("spi_tx_CMD_data_setting ui8_return_value(<-spi_tx_transaction)[A5] : %02X\n", ui8_return_value);
        gpio_isr_off(); // GPIO INTERRUPT OFF

        // ui8_return_value
            // 정상 SPI_FIRST_WORD
            // 비정상 0X88
    }while(ui8_return_value != SPI_FIRST_WORD);
    
    free_spi_structer(&spistructer_spi_send_buf);
    free(ui8_spi_sendbuf);
    ui8_spi_sendbuf = NULL;
    free(ui8_spi_recvbuf);
    ui8_spi_recvbuf = NULL;

    return ui8_return_value;
}

// 13. Uart Recive CMD Data Process
static uint8_t uart_recive_cmd_spi_tx_thread(void *arg)
{
    uint8_t         ui8_return_value                    = 0;
    uint32_t*       A_ui32_cmd_q_recv_128bit;
    uint32_t*       A_ui32_spi_recive_reg_q_send_64bit;
    uint32_t        ui32_reg_count_32bit;
    spi_send_state  enum_spi_send_state                 = RECIVE_UART_DATA;
    
    // UART Queue 수신 버퍼 [SPI(Chip) <- UART(PC)]
    ////////////////////////////////////////////////
    // ADDR_length // ADDR // DATA_length // DATA //
    ////////////////////////////////////////////////
    A_ui32_cmd_q_recv_128bit = (uint32_t*)malloc(sizeof(uint32_t) * 4);
    // 공간 생성 실패시
    if (A_ui32_cmd_q_recv_128bit == NULL) {
        printf("malloc failed: A_ui32_cmd_q_recv_128bit\n");
        free(A_ui32_cmd_q_recv_128bit);
        return ERROR_MALLOC_FAIL;
    }
    memset(A_ui32_cmd_q_recv_128bit, 0, sizeof(uint32_t) * 4);

    // SPI REG Queue 수신 버퍼 [SPI(Chip) -> UART(PC)]
    //////////////////
    // ADDR // DATA //
    //////////////////
    A_ui32_spi_recive_reg_q_send_64bit = (uint32_t*)malloc(sizeof(uint32_t) * 2);
    // 공간 생성 실패시
    if (A_ui32_spi_recive_reg_q_send_64bit == NULL) {
        printf("malloc failed: A_ui32_spi_recive_reg_q_send_64bit\n");
        free(A_ui32_spi_recive_reg_q_send_64bit);
        return ERROR_MALLOC_FAIL;
    }
    memset(A_ui32_spi_recive_reg_q_send_64bit, 0, sizeof(uint32_t) * 2);
    
    spi_structer spistructer_spi_recv_buf;                    // SPI 규격
    // 공간 생성 실패시
    if(!init_spi_structer(&spistructer_spi_recv_buf)){
        printf("malloc failed: spistructer_spi_recv_buf\n");
        free_spi_structer(&spistructer_spi_recv_buf);
        return ERROR_MALLOC_FAIL;
    }

    // Thread 반복
    while(1){
        if(uxQueueMessagesWaiting(Q_ui32_cmd_128bit) > 0){
            // CMD 일 때
            //////////////////////////////////////////////////////////////
            //      1       //  SPI_FIRST_WORD  //      4       //  CMD //
            //////////////////////////////////////////////////////////////

            // Write 일 때
            //////////////////////////////////////////////////////////////
            // ADDR_length //       ADDR        //      4       // DATA //
            //////////////////////////////////////////////////////////////

            xQueueReceive(Q_ui32_cmd_128bit, A_ui32_cmd_q_recv_128bit, portMAX_DELAY);
            printf("enum_spi_send_state : %d\n",enum_spi_send_state);
            switch (enum_spi_send_state) {
                case RECIVE_UART_DATA:
                    printf("enum_spi_send_state = RECIVE_UART_DATA\n");
                    //////////////////////////////////////
                    // 1 // SPI_FIRST_WORD // N // DATA //
                    //////////////////////////////////////
                    printf("ADDR_len = %08X\n", A_ui32_cmd_q_recv_128bit[0]);
                    printf("ADDR = %08X\n", A_ui32_cmd_q_recv_128bit[1]);
                    printf("DATA_len = %08X\n", A_ui32_cmd_q_recv_128bit[2]);
                    printf("DATA = %08X\n", A_ui32_cmd_q_recv_128bit[3]);
                    if ((A_ui32_cmd_q_recv_128bit[0] == 1) && (A_ui32_cmd_q_recv_128bit[1] == SPI_FIRST_WORD)){     // PC CMD 판단
                        if (A_ui32_cmd_q_recv_128bit[3] == CMD_READ_REG){                                                      // CMD == 0 [REG 읽기]
                            printf("SPI CMD = READ REG\n");
                            // Chip <- ESP  CMD 전송
                            ui8_return_value = spi_tx_CMD_data_setting(A_ui32_cmd_q_recv_128bit);
                            // Chip -> ESP  REG 개수 수신
                            do{
                                ui8_return_value = spi_rx_CMD_data_setting(&spistructer_spi_recv_buf);
                            }while(ui8_return_value != SPI_FIRST_WORD);
    
                            // REG 개수
                            ui32_reg_count_32bit = 0;
                            for(uint32_t i_sel_8bit = 0; i_sel_8bit < *spistructer_spi_recv_buf.ui8_data_length; i_sel_8bit++){
                                ui32_reg_count_32bit = ui32_reg_count_32bit | (spistructer_spi_recv_buf.ui8_data[i_sel_8bit] << (8 * ((*spistructer_spi_recv_buf.ui8_data_length - 1) - i_sel_8bit)));
                            }
                            
                            // REG 개수 많큼 REG 수신 [ADDR, DATA]
                            for(uint32_t i_reg_count = 0; i_reg_count < ui32_reg_count_32bit; i_reg_count++){
                                memset(A_ui32_spi_recive_reg_q_send_64bit, 0, sizeof(uint32_t) * 2);

                                // 수신 Error 없을 때 까지 반복
                                do{
                                    ui8_return_value = spi_rx_CMD_data_setting(&spistructer_spi_recv_buf);
                                }while(ui8_return_value != SPI_FIRST_WORD);
                                // 수신 Error 없으면

                                for(uint32_t i_sel_8bit = 0; i_sel_8bit < *spistructer_spi_recv_buf.ui8_addr_length; i_sel_8bit++){
                                    A_ui32_spi_recive_reg_q_send_64bit[0] = A_ui32_spi_recive_reg_q_send_64bit[0] | (spistructer_spi_recv_buf.ui8_addr[i_sel_8bit] << (8 * ((*spistructer_spi_recv_buf.ui8_addr_length - 1) - i_sel_8bit)));
                                }
                                for(uint32_t i_sel_8bit = 0; i_sel_8bit < *spistructer_spi_recv_buf.ui8_data_length; i_sel_8bit++){
                                    A_ui32_spi_recive_reg_q_send_64bit[1] = A_ui32_spi_recive_reg_q_send_64bit[1] | (spistructer_spi_recv_buf.ui8_data[i_sel_8bit] << (8 * ((*spistructer_spi_recv_buf.ui8_data_length - 1) - i_sel_8bit)));
                                }
                                xQueueSend(Q_ui32_spi_recive_reg_64bit, A_ui32_spi_recive_reg_q_send_64bit, 0);
                                // printf("SPI REG [ADDR DATA] -> UART Queue\n");
                            }
                        }
                        else if (A_ui32_cmd_q_recv_128bit[3] == CMD_WRITE_REG){                                                 // CMD == 1 [REG 쓰기]
                            printf("SPI CMD = WRITE REG\n");
                            // Chip <- ESP  CMD 전송
                            spi_tx_CMD_data_setting(A_ui32_cmd_q_recv_128bit);
                            // 상태 변경
                            enum_spi_send_state = SEND_CMD_1_WRITE_REG;
                        }
                    }
                    break;
                case SEND_CMD_1_WRITE_REG:
                    printf("SEND_CMD_1_WRITE_REG\n");
                    // Chip <- ESP  [ADDR / DATA] 전송
                    spi_tx_CMD_data_setting(A_ui32_cmd_q_recv_128bit);
                    enum_spi_send_state = RECIVE_UART_DATA;
                    break;
            }
        }
        vTaskDelay(1);
    }
    // Thread 반복
    free_spi_structer(&spistructer_spi_recv_buf);
}

// 14. SPI 초기화
int spi_init(void){

// spi_init start - 사용가능 heap : 334036  차이 : 0
// spi_init start - 연속 heap : 270336      차이 : 0
// spi_init start - 사용 stack : 1848       차이 : 64

    // SPI IMAGE 초기세팅
    spi_bus_config_t SPI_Image_uscfg = {
        .mosi_io_num = SPI_IMAGE_GPIO_MOSI,
        .miso_io_num = SPI_IMAGE_GPIO_MISO,
        .sclk_io_num = SPI_IMAGE_GPIO_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };

    // SPI IMAGE Slave 세팅
    spi_slave_interface_config_t SPI_Image_slvcfg = {
        .mode = 0,
        .spics_io_num = SPI_IMAGE_GPIO_CS,
        .queue_size = 3,                // 실험 요
        .flags = 0,
    };

    gpio_set_pull_mode(SPI_IMAGE_GPIO_MOSI, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(SPI_IMAGE_GPIO_SCLK, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(SPI_IMAGE_GPIO_CS, GPIO_PULLUP_ONLY);
    esp_err = spi_slave_initialize(SPI_IMAGE_RCV_HOST, &SPI_Image_uscfg, &SPI_Image_slvcfg, SPI_DMA_CH_AUTO);
    assert(esp_err == ESP_OK);

    // SPI CMD 초기세팅
    spi_bus_config_t SPI_CMD_uscfg = {
        .mosi_io_num = SPI_CMD_GPIO_MOSI,
        .miso_io_num = SPI_CMD_GPIO_MISO,
        .sclk_io_num = SPI_CMD_GPIO_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };

    // SPI CMD Slave 세팅
    spi_slave_interface_config_t SPI_CMD_slvcfg = {
        .mode = 0,
        .spics_io_num = SPI_CMD_GPIO_CS,
        .queue_size = 3,                // 실험 요
        .flags = 0,
    };

    gpio_set_pull_mode(SPI_CMD_GPIO_MOSI, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(SPI_CMD_GPIO_SCLK, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(SPI_CMD_GPIO_CS, GPIO_PULLUP_ONLY);
    esp_err = spi_slave_initialize(SPI_CMD_RCV_HOST, &SPI_CMD_uscfg, &SPI_CMD_slvcfg, SPI_DMA_CH_AUTO);
    assert(esp_err == ESP_OK);

                // 실행 함수,           스레드 이름,          사이즈,       , 우선순위,    ,
    // SPI Image 수신 스레드 시작 
    xTaskCreate(spi_image_rx_thread, "SPI_IMAGE_RECIVE_THREAD", SPI_IMAGE_RX_STACK_SIZE, NULL, configMAX_PRIORITIES - 1, NULL);
    // SPI Image 수신 데이터 재구성 스레드 시작
    xTaskCreate(spi_image_rx_data_process, "SPI_IMAGE_RECIVE_DATA_PROCESS_THREAD", SPI_IMAGE_RECIVE_DATA_PROCESS_STACK_SIZE, NULL, configMAX_PRIORITIES - 1, NULL);
    // SPI CMD 송신 스레드 시작
    xTaskCreate(uart_recive_cmd_spi_tx_thread, "SPI_SEND_THREAD", SPI_CMD_TX_STACK_SIZE, NULL, configMAX_PRIORITIES - 1, NULL);

    view_heap_stack("spi_init end");
// spi_init end - 사용가능 heap : 317660    차이 : 16376
// spi_init end - 연속 heap : 253952        차이 : 16384
// spi_init end - 사용 stack : 1848         차이 : 0

    return 0;
}