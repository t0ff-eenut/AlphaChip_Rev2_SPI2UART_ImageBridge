/**
 * @file        app_image_processing.h
 * @brief       Image Processing 모듈 헤더 파일
 * @author      T0T
 * @date        2026-01-14
 * @version     1.0.0
 * 
 * @details     이 파일은 Image Processing 모듈을 구현한 헤더 파일임.
 */

#ifndef APP_IMAGE_PROCESSING_H
#define APP_IMAGE_PROCESSING_H

#include "app_level_top.h"         // app Level 통합 헤더 사용

/*===========================================================================*/
/* 메크로 정의
/*===========================================================================*/
// // [.c] Setter 함수만 생성하는 매크로 (읽기 전용 변수용)
// #define GENERATE_SETTER(value_type, func_suffix, value_name) \
//     void custom_set_##func_suffix(value_type input_data) { \
//         value_name = input_data; \
//     } 
// // [.c] Getter와 Setter 함수를 동시에 생성하는 매크로
// #define GENERATE_GETTER_SETTER(value_type, func_suffix, value_name) \
//     GENERATE_SETTER(value_type, func_suffix, value_name); \
//     GENERATE_GETTER(value_type, func_suffix, value_name);

/*===========================================================================*/
/* 열거형 정의
/*===========================================================================*/
/**
 * @enum        ipsle
 * @typedef     image_processing_status_level_enum
 * @brief       Image Processing 모듈 초기화 상태 열거형
 * @details     Image Processing 모듈 초기화 순서에 따라 모듈의 초기화 상태를 저장하는 배열
 */
typedef enum image_processing_status_level_enum{
    MODE_START,
    READ_IMAGE_TO_SPI,
    REMAKE_IMAGE,
    SEND_IMAGE_TO_UART,
    MODE_END,
}ipsle;

/*===========================================================================*/
/* 변수 정의
/*===========================================================================*/

/*===========================================================================*/
/* 함수 정의
/*===========================================================================*/
/**
 * @brief       app_image_processing_start() Function
 * @param[in]   void
 * @return      void
 * @details     Image Processing 모듈을 시작하는 함수
 */
void app_image_processing_start(void);

#endif