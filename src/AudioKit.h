#pragma once
#include "AudioKitSettings.h"

// include drivers
#include "audio_hal/driver/es7148/es7148.h"
#include "audio_hal/driver/es7210/es7210.h"
#include "audio_hal/driver/es7243/es7243.h"
#include "audio_hal/driver/es8311/es8311.h"
#include "audio_hal/driver/es8374/es8374.h"
#include "audio_hal/driver/es8388/es8388.h"
#include "audio_hal/driver/tas5805m/tas5805m.h"
#include "audiokit_board.h"
#include "driver/i2s.h"
#include "esp_a2dp_api.h"
#include "esp_system.h"

#if ESP_IDF_VERSION_MAJOR < 4 && !defined(I2S_COMM_FORMAT_STAND_I2S)
#define I2S_COMM_FORMAT_STAND_I2S \
  (I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_LSB)
#define I2S_COMM_FORMAT_STAND_MSB \
  (I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB)
#define I2S_COMM_FORMAT_STAND_PCM_LONG \
  (I2S_COMM_FORMAT_PCM | I2S_COMM_FORMAT_PCM_LONG)
#define I2S_COMM_FORMAT_STAND_PCM_SHORT \
  (I2S_COMM_FORMAT_PCM | I2S_COMM_FORMAT_PCM_SHORT)
typedef int eps32_i2s_sample_rate_type;
#else
typedef uint32_t eps32_i2s_sample_rate_type;
#endif

/**
 * @brief Configuation for AudioKit
 *
 */
struct AudioKitConfig {
  i2s_port_t i2s_num = (i2s_port_t)0;
  gpio_num_t mclk_gpio = (gpio_num_t)0;

  audio_hal_adc_input_t adc_input =
      AUDIOKIT_DEFAULT_INPUT; /*!<  set adc channel with audio_hal_adc_input_t
                               */
  audio_hal_dac_output_t dac_output =
      AUDIOKIT_DEFAULT_OUTPUT;       /*!< set dac channel */
  audio_hal_codec_mode_t codec_mode; /*!< select codec mode: adc, dac or both */
  audio_hal_iface_mode_t master_slave_mode =
      AUDIOKIT_DEFAULT_MASTER_SLAVE; /*!< audio codec chip mode */
  audio_hal_iface_format_t fmt =
      AUDIOKIT_DEFAULT_I2S_FMT; /*!< I2S interface format */
  audio_hal_iface_samples_t sample_rate =
      AUDIOKIT_DEFAULT_RATE; /*!< I2S interface samples per second */
  audio_hal_iface_bits_t bits_per_sample =
      AUDIOKIT_DEFAULT_BITSIZE; /*!< i2s interface number of bits per sample */

  /// Returns true if the CODEC is the master
  bool isMaster() { return master_slave_mode == AUDIO_HAL_MODE_MASTER; }

  /// provides the bits per sample
  int bitsPerSample() {
    switch (bits_per_sample) {
      case AUDIO_HAL_BIT_LENGTH_16BITS:
        return 16;
      case AUDIO_HAL_BIT_LENGTH_24BITS:
        return 24;
      case AUDIO_HAL_BIT_LENGTH_32BITS:
        return 32;
    }
    return 0;
  }

  /// Provides the sample rate in samples per second
  int sampleRate() {
    switch (sample_rate) {
      case AUDIO_HAL_08K_SAMPLES: /*!< set to  8k samples per second */
        return 8000;
      case AUDIO_HAL_11K_SAMPLES: /*!< set to 11.025k samples per second */
        return 11000;
      case AUDIO_HAL_16K_SAMPLES: /*!< set to 16k samples in per second */
        return 16000;
      case AUDIO_HAL_22K_SAMPLES: /*!< set to 22.050k samples per second */
        return 22000;
      case AUDIO_HAL_24K_SAMPLES: /*!< set to 24k samples in per second */
        return 24000;
      case AUDIO_HAL_32K_SAMPLES: /*!< set to 32k samples in per second */
        return 32000;
      case AUDIO_HAL_44K_SAMPLES: /*!< set to 44.1k samples per second */
        return 44000;
      case AUDIO_HAL_48K_SAMPLES: /*!< set to 48k samples per second */
        return 48000;
    }
    return 0;
  }

  /// Provides the ESP32 i2s_config_t
  i2s_config_t i2sConfig() {
    int mode = isMaster() ? I2S_MODE_MASTER : I2S_MODE_SLAVE;
    if (codec_mode == AUDIO_HAL_CODEC_MODE_DECODE) {
      mode = mode | I2S_MODE_TX;
    } else if (codec_mode == AUDIO_HAL_CODEC_MODE_ENCODE) {
      mode = mode | I2S_MODE_RX;
    } else if (codec_mode == AUDIO_HAL_CODEC_MODE_BOTH) {
      mode = mode | I2S_MODE_RX | I2S_MODE_TX;
    }

    const i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)mode,
        .sample_rate = sampleRate(),
        .bits_per_sample = (i2s_bits_per_sample_t)bitsPerSample(),
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = (i2s_comm_format_t)I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = 0,  // default interrupt priority
        .dma_buf_count = 8,
        .dma_buf_len = 64,
        .use_apll = true};
    return i2s_config;
  }

  i2s_pin_config_t i2sPins() { 
    i2s_pin_config_t result;
    get_i2s_pins(i2s_num, &result);
    return result;
   }
};

/**
 * @brief AudioKit API using the audio_hal
 *
 */

class AudioKit {
 public:
  /// Provides a default configuration for input & output
  AudioKitConfig defaultConfig() {
    AudioKitConfig result;
    result.codec_mode = AUDIO_HAL_CODEC_MODE_BOTH;
    return result;
  }

  /// Provides the default configuration for input or output
  AudioKitConfig defaultConfig(bool isOutput) {
    AudioKitConfig result;
    if (isOutput) {
      result.codec_mode = AUDIO_HAL_CODEC_MODE_DECODE;  // dac
    } else {
      result.codec_mode = AUDIO_HAL_CODEC_MODE_ENCODE;  // adc
    }
    return result;
  }

  /// Starts the codec
  bool begin(AudioKitConfig cnfg) {
    cfg = cnfg;
    audio_hal_conf.adc_input = cfg.adc_input;
    audio_hal_conf.dac_output = cfg.dac_output;
    audio_hal_conf.codec_mode = cfg.codec_mode;
    // audio_hal_conf.i2s_iface = cfg.master_slave_mode;
    if (audio_hal_init(&audio_hal_conf, &audio_hal) != ESP_OK) {
      return false;
    }

    // setup audio_hal_codec_i2s_iface_t
    iface.mode = cfg.master_slave_mode;
    iface.fmt = cfg.fmt;
    iface.samples = cfg.sample_rate;
    iface.bits = cfg.bits_per_sample;

    // configure codec
    if (audio_hal_codec_iface_config(&audio_hal, cfg.codec_mode, &iface) !=
        ESP_OK) {
      return false;
    }

    // start codec driver
    if (audio_hal_ctrl_codec(&audio_hal, cfg.codec_mode,
                             AUDIO_HAL_CTRL_START) != ESP_OK) {
      return false;
    }

    // setup i2s driver
    i2s_config_t i2s_config = cfg.i2sConfig();
    if (i2s_driver_install(cfg.i2s_num, &i2s_config, 0, NULL)) {
      return false;
    }

    // define i2s pins
    i2s_pin_config_t pin_config = cfg.i2sPins();
    if (i2s_set_pin(cfg.i2s_num, &pin_config)!=ESP_OK){
      return false;
    }

    if (i2s_mclk_gpio_select(cfg.i2s_num, cfg.mclk_gpio) != ESP_OK) {
      return false;
    }

    return true;
  }

  /// Stops the CODEC
  bool end() {
    // uninstall i2s driver
    i2s_driver_uninstall(cfg.i2s_num);
    // stop codec driver
    audio_hal_ctrl_codec(&audio_hal, cfg.codec_mode, AUDIO_HAL_CTRL_STOP);
    // deinit
    audio_hal_deinit(&audio_hal);
    return true;
  }

  /// Provides the actual configuration
  AudioKitConfig config() {
      return cfg;
  }

  /// Sets the codec active / inactive
  bool setActive(bool active) {
    return audio_hal_ctrl_codec(
               &audio_hal, cfg.codec_mode,
               active ? AUDIO_HAL_CTRL_START : AUDIO_HAL_CTRL_STOP) == ESP_OK;
  }

  /// Mutes the output
  bool setMute(bool mute) {
    return audio_hal_set_mute(&audio_hal, mute) == ESP_OK;
  }

  /// Defines the Volume
  bool setVolume(int vol) {
    return (vol > 0) ? audio_hal_set_volume(&audio_hal, vol) == ESP_OK : false;
  }

  /// Determines the volume
  int volume() {
    int volume;
    if (audio_hal_get_volume(&audio_hal, &volume) != ESP_OK) {
      volume = -1;
    }
    return volume;
  }

  /// Writes the audio data via i2s to the DAC
  size_t write(const void *src, size_t size,
               TickType_t ticks_to_wait = portMAX_DELAY) {
    size_t bytes_written = 0;
    i2s_write(cfg.i2s_num, src, size, &bytes_written, ticks_to_wait);
    return bytes_written;
  }

  /// Reads the audio data via i2s from the ADC
  size_t read(void *dest, size_t size,
              TickType_t ticks_to_wait = portMAX_DELAY) {
    size_t bytes_read = 0;
    i2s_read(cfg.i2s_num, dest, size, &bytes_read, ticks_to_wait);
    return bytes_read;
  }

  /**
   * @brief  Get the gpio number for auxin detection
   *
   * @return  -1      non-existent
   *          Others  gpio number
   */
  int8_t pinAuxin() { return get_auxin_detect_gpio(); }

  /**
   * @brief  Get the gpio number for headphone detection
   *
   * @return  -1      non-existent
   *          Others  gpio number
   */
  int8_t pinHeadphoneDetect() { return get_headphone_detect_gpio(); }

  /**
   * @brief  Get the gpio number for PA enable
   *
   * @return  -1      non-existent
   *          Others  gpio number
   */
  int8_t pinPaEnable() { return get_pa_enable_gpio(); }

  /**
   * @brief  Get the gpio number for adc detection
   *
   * @return  -1      non-existent
   *          Others  gpio number
   */
  int8_t pinAdcDetect() { return get_adc_detect_gpio(); }

  /**
   * @brief  Get the mclk gpio number of es7243
   *
   * @return  -1      non-existent
   *          Others  gpio number
   */
  int8_t pinEs7243Mclk() { return get_es7243_mclk_gpio(); }

  /**
   * @brief  Get the record-button id for adc-button
   *
   * @return  -1      non-existent
   *          Others  button id
   */
  int8_t pinInputRec() { return get_input_rec_id(); }

  /**
   * @brief  Get the number for mode-button
   *
   * @return  -1      non-existent
   *          Others  number
   */
  int8_t pinInputMode() { return get_input_mode_id(); }

  /**
   * @brief Get number for set function
   *
   * @return -1       non-existent
   *         Others   number
   */
  int8_t pinInputSet() { return get_input_set_id(); };

  /**
   * @brief Get number for play function
   *
   * @return -1       non-existent
   *         Others   number
   */
  int8_t pinInputPlay() { return get_input_play_id(); }

  /**
   * @brief number for volume up function
   *
   * @return -1       non-existent
   *         Others   number
   */
  int8_t pinVolumeUp() { return get_input_volup_id(); }

  /**
   * @brief Get number for volume down function
   *
   * @return -1       non-existent
   *         Others   number
   */
  int8_t pinVolumeDown() { return get_input_voldown_id(); }

  /**
   * @brief Get green led gpio number
   *
   * @return -1       non-existent
   *        Others    gpio number
   */
  int8_t pinResetCodec() { return get_reset_codec_gpio(); }

  /**
   * @brief Get DSP reset gpio number
   *
   * @return -1       non-existent
   *         Others   gpio number
   */
  int8_t pinResetBoard() { return get_reset_board_gpio(); }

  /**
   * @brief Get DSP reset gpio number
   *
   * @return -1       non-existent
   *         Others   gpio number
   */
  int8_t pinGreenLed() { return get_green_led_gpio(); }

  /**
   * @brief Get green led gpio number
   *
   * @return -1       non-existent
   *         Others   gpio number
   */
  int8_t pinBlueLed() { return get_blue_led_gpio(); }

 protected:
  audio_hal_func_t audio_hal;
  audio_hal_codec_config_t audio_hal_conf;
  audio_hal_codec_i2s_iface_t iface;
  AudioKitConfig cfg;
};