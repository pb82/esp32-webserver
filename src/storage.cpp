#include "storage.h"

#include "esp_log.h"

#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>

#include "sdkconfig.h"
#include "esp_littlefs.h"

static const char *TAG = "Storage";

static esp_vfs_littlefs_conf_t conf = {
    .base_path = "/spiffs",
    .partition_label = "spiffs",
    .format_if_mount_failed = false,
    .dont_mount = false,
};

Storage::Storage()
{
    if (esp_vfs_littlefs_register(&conf) != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_vfs_littlefs_register failed");
        return;
    }

    size_t total = 0, used = 0;
    if (esp_littlefs_info(conf.partition_label, &total, &used) != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_littlefs_info failed");
    }
    ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
}

Storage::~Storage()
{
    esp_vfs_littlefs_unregister(conf.partition_label);
}

bool Storage::read_index(File &file)
{
    FILE *f = fopen("/spiffs/index.html", "rb");
    if (!f)
    {
        ESP_LOGE(TAG, "read_index failed, file not found");
        return false;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    file.size = size;
    file.contents = (char *)malloc(size * sizeof(char));
    fread(file.contents, size, 1, f);

    fclose(f);
    return true;
}