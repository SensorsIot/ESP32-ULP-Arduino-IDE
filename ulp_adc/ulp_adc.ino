/*
   Must allocate more memory for the ulp in
   Arduino15\packages\esp32\hardware\esp32\1.0.0\tools\sdk\sdkconfig.h
   -> #define CONFIG_ULP_COPROC_RESERVE_MEM
   for this sketch to compile. 2048b seems
   good.
*/
#include "esp_sleep.h"
#include "driver/rtc_io.h"
#include "driver/adc.h"
#include "esp32/ulp.h"
#include "ulp_main.h"

extern const uint8_t ulp_main_bin_start[] asm("_binary_ulp_main_bin_start");
extern const uint8_t ulp_main_bin_end[]   asm("_binary_ulp_main_bin_end");

gpio_num_t ulp_27 = GPIO_NUM_27;
gpio_num_t ulp_13 = GPIO_NUM_13;


void setup() {
  esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
  if (cause != ESP_SLEEP_WAKEUP_ULP) {
    printf("Not ULP wakeup\n");
    init_ulp_program();
  } else {

    // ***** HERE YOUR SKETCH *****
    printf("Deep sleep wakeup\n");
    printf("ULP did %d measurements since last reset\n", ulp_sample_counter & UINT16_MAX);
    printf("Thresholds:  low=%d  high=%d\n", ulp_low_threshold, ulp_high_threshold);
    ulp_ADC_reading &= UINT16_MAX;
    printf("Value=%d was %s threshold\n", ulp_ADC_reading, ulp_ADC_reading < ulp_low_threshold ? "below" : "above");

  }
  // printf("Entering deep sleep\n\n");
  delay(100);
  start_ulp_program();
  ESP_ERROR_CHECK( esp_sleep_enable_ulp_wakeup() );
  esp_deep_sleep_start();

}

void loop() {

}

static void init_ulp_program()
{
  esp_err_t err = ulp_load_binary(0, ulp_main_bin_start,
                                  (ulp_main_bin_end - ulp_main_bin_start) / sizeof(uint32_t));
  ESP_ERROR_CHECK(err);

  rtc_gpio_init(ulp_27);
  rtc_gpio_set_direction(ulp_27, RTC_GPIO_MODE_OUTPUT_ONLY);

  rtc_gpio_init(ulp_13);
  rtc_gpio_set_direction(ulp_13, RTC_GPIO_MODE_OUTPUT_ONLY);


  /* Configure ADC channel */
  /* Note: when changing channel here, also change 'adc_channel' constant
     in adc.S */
  adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_11);
  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_ulp_enable();
  ulp_low_threshold = 1 * (4095 / 3.3);  //1 volt
  ulp_high_threshold = 2 * (4095 / 3.3);   // 2 volts

  /* Set ULP wake up period to 100ms */
  ulp_set_wakeup_period(0, 100 * 1000);

  /* Disable pullup on GPIO15, in case it is connected to ground to suppress
     boot messages.
  */
  //  rtc_gpio_pullup_dis(GPIO_NUM_15);
  //  rtc_gpio_hold_en(GPIO_NUM_15);
}

static void start_ulp_program()
{
  /* Start the program */
  esp_err_t err = ulp_run((&ulp_entry - RTC_SLOW_MEM) / sizeof(uint32_t));
  ESP_ERROR_CHECK(err);
}
