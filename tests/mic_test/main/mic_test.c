
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "audio_hal.h"
#include "bsp_board.h"

static const char *TAG = "MIC_TEST";

void app_main(void) {
    ESP_LOGI(TAG,"Initializing board and codec");
    // init Korvo audio board
    bsp_board_init();
    audio_hal_codec_config_t cfg = AUDIO_HAL_CODEC_DEFAULT_CONFIG();
    audio_hal_handle_t codec = bsp_audio_codec_microphone_init();

    if (!codec) {
        ESP_LOGE(TAG,"Codec init failed");
        return;
    }

    // open mic, 16kHz, mono, 16 bits per sample
    esp_codec_dev_sample_info_t sample_info = {
        .sample_rate = 16000,
        .channel = 1,
        .bits_per_sample = 16
    };

    ESP_LOGI(TAG,"Opening microphone");
    esp_err_t err = esp_codec_dev_open(codec, &sample_info);
    if (err!=ESP_OK) {
        ESP_LOGE(TAG,"esp_codec_dev_open failed: %d", err);
        return;
    }

    ESP_LOGI(TAG,"Ready to read mic");

    uint8_t inbuf[512];

    while (1) {
        // read raw PCM audio from mic
        int read_bytes = esp_codec_dev_read(codec, inbuf, sizeof(inbuf));
        if (read_bytes > 0) {
            int16_t *samples = (int16_t*)inbuf;
            int16_t peak = 0;
            for (int i=0; i < read_bytes/2; i++) {
                int16_t v = samples[i];
                if (abs(v) > peak) peak = abs(v);
            }
            ESP_LOGI(TAG,"Read %d bytes, peak sample: %d", read_bytes, peak);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
