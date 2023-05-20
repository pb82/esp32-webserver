#include <Arduino.h>
#include <Adafruit_Protomatter.h>

// defines WIFI_SSID and WIFI_PASSWORD
#include "secrets.h"

#include "nvs_flash.h"
#include "wifi_manager.h"

// start non volatile storage. This is required by the WiFi driver.
void start_nvs()
{
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);
}

void network_task(void *)
{
  WifiManager manager;
  if (manager.startSta(WIFI_SSID, WIFI_PASSWORD) && manager.getState() == STA_CONNECTED)
  {
    manager.startMdns("koddly");
    manager.startWebserver();
    ESP_LOGI("network", "webserver is running");
  }
  else
  {
    ESP_LOGE("network", "error");
  }

  vTaskDelete(nullptr);
}

void setup()
{
  start_nvs();
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  xTaskCreate(network_task, "NETWORK", 4096, nullptr, 1, nullptr);
}

void loop()
{
  vTaskDelete(nullptr);
}
