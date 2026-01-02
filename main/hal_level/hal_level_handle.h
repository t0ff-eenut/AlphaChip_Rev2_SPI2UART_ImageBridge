/*
******************************************************************************
* File Name          : hal_level_handle.h
* Description        : HAL Level Handle Header - HAL 핸들 통합
******************************************************************************
* Queue, ADC, UART, Band 등 HAL 레벨 핸들 및 전역 변수 선언
* 현재는 각 모듈에서 개별 관리하므로 향후 확장을 위한 빈 헤더
******************************************************************************
* first update : 2025/11/27
******************************************************************************
*/

#ifndef HAL_LEVEL_HANDLE_H
#define HAL_LEVEL_HANDLE_H

// #include "hal_level_top.h"
#include "custom_esp_adc_thread.h"
#include "custom_esp_uart_thread.h"
// #include "custom_esp_band_thread.h"

// Queue, ADC 등 HAL 레벨 핸들 관련 타입 및 전역 변수
// (현재는 각 모듈에서 개별 관리)
// 향후 필요 시 공통 HAL 핸들을 이곳에 선언

#endif // HAL_LEVEL_HANDLE_H
