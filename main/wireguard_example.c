#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>

#include <esp_log.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <esp_netif.h>
#include <esp_event.h>
#include <nvs_flash.h>
#include <lwip/netdb.h>
#include <ping/ping_sock.h>

#include "wifi_prov.h"
#include "sync_time.h"
#include "ping_test.h"
#include <esp_wireguard.h>

static const char *TAG = "wireguard_example";

static wireguard_config_t wg_config = ESP_WIREGUARD_CONFIG_DEFAULT();

static esp_err_t wireguard_setup(wireguard_ctx_t* ctx)
{
    esp_err_t err = ESP_FAIL;

    ESP_LOGI(TAG, "Initializing WireGuard.");
    wg_config.private_key = CONFIG_WG_PRIVATE_KEY;
    wg_config.listen_port = CONFIG_WG_LOCAL_PORT;
    wg_config.public_key = CONFIG_WG_PEER_PUBLIC_KEY;
    if (strcmp(CONFIG_WG_PRESHARED_KEY, "") != 0) {
        wg_config.preshared_key = CONFIG_WG_PRESHARED_KEY;
    } else {
        wg_config.preshared_key = NULL;
    }
    wg_config.allowed_ip = CONFIG_WG_LOCAL_IP_ADDRESS;
    wg_config.allowed_ip_mask = CONFIG_WG_LOCAL_IP_NETMASK;
    wg_config.endpoint = CONFIG_WG_PEER_ADDRESS;
    wg_config.port = CONFIG_WG_PEER_PORT;
    wg_config.persistent_keepalive = CONFIG_WG_PERSISTENT_KEEP_ALIVE;

    err = esp_wireguard_init(&wg_config, ctx);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_wireguard_init: %s", esp_err_to_name(err));
        goto fail;
    }

    ESP_LOGI(TAG, "Connecting to the peer.");
    err = esp_wireguard_connect(ctx);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_wireguard_connect: %s", esp_err_to_name(err));
        goto fail;
    }

    err = ESP_OK;
fail:
    return err;
}

void app_main(void)
{
    /* Initialize NVS partition */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        /* NVS partition was truncated
         * and needs to be erased */
        ESP_ERROR_CHECK(nvs_flash_erase());

        /* Retry nvs_flash_init */
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    /* Provision and connect to WiFi AP */
    wifi_softap_prov();

    /* Synchronize time - needed for a successful Wireguard connection */
    sync_time();

    wireguard_ctx_t ctx = {0};
    esp_err_t err = wireguard_setup(&ctx);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "wireguard_setup: %s", esp_err_to_name(err));
        goto fail;
    }

    while (1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        err = esp_wireguardif_peer_is_up(&ctx);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Peer is up");
            break;
        } else {
            ESP_LOGI(TAG, "Peer is down");
        }
    }
    start_ping();

    while (1) {
        vTaskDelay(1000 * 10 / portTICK_PERIOD_MS);
        ESP_LOGI(TAG, "Disconnecting.");
        esp_wireguard_disconnect(&ctx);
        ESP_LOGI(TAG, "Disconnected.");

        vTaskDelay(1000 * 10 / portTICK_PERIOD_MS);
        ESP_LOGI(TAG, "Connecting.");
        err = esp_wireguard_connect(&ctx);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "esp_wireguard_connect: %s", esp_err_to_name(err));
            goto fail;
        }
        while (esp_wireguardif_peer_is_up(&ctx) != ESP_OK) {
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
        ESP_LOGI(TAG, "Peer is up");
        esp_wireguard_set_default(&ctx);
    }

fail:
    ESP_LOGE(TAG, "Halting due to error");
    while (1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
