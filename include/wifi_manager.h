#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "esp_wifi.h"
#include "esp_event.h"
#include "freertos/event_groups.h"

#include <esp_http_server.h>

enum WifiState
{
    NONE,
    STA_CONNECTED,
    STA_DISCONNECTED
};

class WifiManager
{
public:
    WifiManager();
    ~WifiManager();

    bool startSta(const char *ssid, const char *password);
    void startMdns(const char *host);
    void startWebserver();

    WifiState getState() const
    {
        return this->state;
    }

    void setAuthMode(wifi_auth_mode_t auth_mode)
    {
        this->auth_mode = auth_mode;
    }

private:
    WifiState state = NONE;
    wifi_auth_mode_t auth_mode = WIFI_AUTH_WPA2_PSK;

    httpd_handle_t server;
    httpd_uri_t get_handler;

    esp_event_handler_instance_t wifi_event_handler_ref;
    esp_event_handler_instance_t ip_event_handler_ref;

    static esp_err_t handle_get(httpd_req_t *req);
    static void wifi_event_handler(void *handler_arg, esp_event_base_t base, int32_t id, void *event_data);
    static void ip_event_handler(void *handler_arg, esp_event_base_t base, int32_t id, void *event_data);
};

#endif