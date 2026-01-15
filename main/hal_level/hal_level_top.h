/**
 * @file        hal_level_top.h
 * @brief       HAL 핸들 통합 최상위 헤더 파일
 * @author      T0T
 * @date        2026-01-07
 * @version     1.0.0
 * 
 * @details     이 파일은 HAL 핸들 통합 헤더 파일로,
 *              Queue, SPI, UART 등 HAL 핸들 모듈을 최상위 헤더로 관리
 */

#ifndef HAL_LEVEL_TOP_H
#define HAL_LEVEL_TOP_H

#include "hw_level_handle.h"

// HAL Level 모듈 포함 (순서 중요: Queue가 먼저 정의되어야 함)
// #include "custom_esp_queue.h"
// #include "custom_esp_spi_thread.h"
// #include "custom_esp_uart_thread.h"

#endif // HAL_LEVEL_TOP_H
