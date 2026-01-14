    
file(GLOB_RECURSE component_sources "*.c")

idf_component_register(
SRCS ${component_sources}
INCLUDE_DIRS 
    "."                                          # main.c, project_top.h
    "hw_level"                                   # hw_level_top.h, hw_level_handle.h
    "hw_level/custom_esp_gpio"
    # "hw_level/custom_esp_nvs"
    "hal_level"                                  # hal_level_top.h, hal_level_handle.h
    "hal_level/custom_esp_queue"
    "hal_level/custom_esp_uart"
    "hal_level/custom_esp_spi"
    "app_level"
    "app_level/app_image_processing"
REQUIRES 
    driver 
#     esp_adc 
#     nvs_flash 
    esp_timer 
    esp_driver_ledc
    led_strip 
#     esp-dsp
)  # 관리형 컴포넌트 (Managed Components)
