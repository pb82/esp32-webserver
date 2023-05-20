#include "wifi_manager.h"

#include <string.h>

#include <mdns.h>
#include "storage.h"
#include "esp_log.h"



#define MAX_RECONNECT_ATTMEPTS 10

static EventGroupHandle_t wifi_event_group;
static const char *TAG = "WifiManager";
static int retries = 0;

static size_t wifi_strncpy(char *dst, const char *src, size_t dst_len)
{
    if (!dst || !src || !dst_len)
    {
        return 0;
    }
    size_t src_len = strlen(src);
    if (src_len >= dst_len)
    {
        src_len = dst_len;
    }
    else
    {
        src_len += 1;
    }
    memcpy(dst, src, src_len);
    return src_len;
}

WifiManager::WifiManager()
{
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, WifiManager::wifi_event_handler, this, &wifi_event_handler_ref));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, WifiManager::ip_event_handler, this, &ip_event_handler_ref));
}

WifiManager::~WifiManager()
{
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler_ref));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, ESP_EVENT_ANY_ID, ip_event_handler_ref));
}

void WifiManager::wifi_event_handler(void *handler_arg, esp_event_base_t base, int32_t id, void *event_data)
{
    if (base != WIFI_EVENT)
        return;

    switch (id)
    {
    case WIFI_EVENT_STA_START:
        esp_wifi_connect();
        break;
    case WIFI_EVENT_STA_DISCONNECTED:
        ((WifiManager *)handler_arg)->state = STA_DISCONNECTED;
        if (retries < MAX_RECONNECT_ATTMEPTS)
        {
            ESP_LOGI(TAG, "wifi disconnected, retry #%d / %d", retries, MAX_RECONNECT_ATTMEPTS);
            esp_wifi_connect();
            retries++;
            break;
        }
        ESP_LOGI("WifiManager", "wifi disconnected, retries exhausted");
        xEventGroupSetBits(wifi_event_group, BIT1);
        break;
    default:
        break;
    }
}

void WifiManager::ip_event_handler(void *handler_arg, esp_event_base_t base, int32_t id, void *event_data)
{
    if (base != IP_EVENT)
        return;

    switch (id)
    {
    case IP_EVENT_STA_GOT_IP:
        xEventGroupSetBits(wifi_event_group, BIT0);
        ((WifiManager *)handler_arg)->state = STA_CONNECTED;
        retries = 0;
        break;
    default:
        break;
    }
}

bool WifiManager::startSta(const char *ssid, const char *password)
{
    ESP_ERROR_CHECK(esp_netif_init());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    wifi_config_t wifi_config;
    memset(&wifi_config, 0, sizeof(wifi_config_t));
    wifi_config.sta.threshold.authmode = this->auth_mode;
    strncpy((char *)wifi_config.sta.ssid, ssid, 32);
    strncpy((char *)wifi_config.sta.password, password, 64);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    wifi_event_group = xEventGroupCreate();
    EventBits_t bits = xEventGroupWaitBits(wifi_event_group,
                                           BIT0 | BIT1,
                                           pdFALSE,
                                           pdFALSE,
                                           portMAX_DELAY);
    // connected bit set?
    return bits & BIT0;
}

void WifiManager::startMdns(const char *host)
{
    ESP_ERROR_CHECK(mdns_init());
    ESP_ERROR_CHECK(mdns_hostname_set(host));
    ESP_ERROR_CHECK(mdns_service_add(NULL, "_http", "_tcp", 80, NULL, 0));
}

esp_err_t WifiManager::handle_get(httpd_req_t *req)
{
    Storage s;
    File f;
    s.read_index(f);
    httpd_resp_send(req, f.contents, f.size);
    return ESP_OK;
}

void WifiManager::startWebserver()
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    ESP_ERROR_CHECK(httpd_start(&this->server, &config));

    this->get_handler.method = HTTP_GET;
    this->get_handler.uri = "/";
    this->get_handler.handler = WifiManager::handle_get;

    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &get_handler));
}