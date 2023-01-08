#include <string.h>
#include <inttypes.h>

#include <esp_log.h>
#include <lwip/netdb.h>
#include <ping/ping_sock.h>

#include "ping_test.h"

static const char *TAG = "wireguard_example_ping_test";

static void test_on_ping_success(esp_ping_handle_t hdl, void *args)
{
    uint8_t ttl;
    uint16_t seqno;
    uint32_t elapsed_time, recv_len;
    ip_addr_t target_addr;
    esp_ping_get_profile(hdl, ESP_PING_PROF_SEQNO, &seqno, sizeof(seqno));
    esp_ping_get_profile(hdl, ESP_PING_PROF_TTL, &ttl, sizeof(ttl));
    esp_ping_get_profile(hdl, ESP_PING_PROF_IPADDR, &target_addr, sizeof(target_addr));
    esp_ping_get_profile(hdl, ESP_PING_PROF_SIZE, &recv_len, sizeof(recv_len));
    esp_ping_get_profile(hdl, ESP_PING_PROF_TIMEGAP, &elapsed_time, sizeof(elapsed_time));
    ESP_LOGI(TAG, "%" PRIu32 " bytes from %s icmp_seq=%" PRIu16 " ttl=%" PRIi8 " time=%" PRIu32 " ms",
           recv_len, ipaddr_ntoa(&target_addr), seqno, ttl, elapsed_time);
}

static void test_on_ping_timeout(esp_ping_handle_t hdl, void *args)
{
    uint16_t seqno;
    ip_addr_t target_addr;
    esp_ping_get_profile(hdl, ESP_PING_PROF_SEQNO, &seqno, sizeof(seqno));
    esp_ping_get_profile(hdl, ESP_PING_PROF_IPADDR, &target_addr, sizeof(target_addr));
    ESP_LOGI(TAG, "From %s icmp_seq=%" PRIu16 " timeout", ipaddr_ntoa(&target_addr), seqno);
}

static void test_on_ping_end(esp_ping_handle_t hdl, void *args)
{
    uint32_t transmitted;
    uint32_t received;
    uint32_t total_time_ms;

    esp_ping_get_profile(hdl, ESP_PING_PROF_REQUEST, &transmitted, sizeof(transmitted));
    esp_ping_get_profile(hdl, ESP_PING_PROF_REPLY, &received, sizeof(received));
    esp_ping_get_profile(hdl, ESP_PING_PROF_DURATION, &total_time_ms, sizeof(total_time_ms));
    ESP_LOGI(TAG, "%" PRIu32 " packets transmitted, %" PRIu32 " received, time %" PRIu32 "ms", transmitted, received, total_time_ms);
}

void start_ping()
{
    ESP_LOGI(TAG, "Initializing ping...");
    /* convert URL to IP address */
    ip_addr_t target_addr;
    struct addrinfo *res = NULL;
    struct addrinfo hint;
    memset(&hint, 0, sizeof(hint));
    memset(&target_addr, 0, sizeof(target_addr));
    ESP_ERROR_CHECK(lwip_getaddrinfo(CONFIG_EXAMPLE_PING_ADDRESS, NULL, &hint, &res) == 0 ? ESP_OK : ESP_FAIL);
    struct in_addr addr4 = ((struct sockaddr_in *) (res->ai_addr))->sin_addr;
    inet_addr_to_ip4addr(ip_2_ip4(&target_addr), &addr4);
    lwip_freeaddrinfo(res);
    ESP_LOGI(TAG, "ICMP echo target: %s", CONFIG_EXAMPLE_PING_ADDRESS);
    esp_ping_config_t ping_config = ESP_PING_DEFAULT_CONFIG();
    ping_config.target_addr = target_addr;          // target IP address
    ping_config.count = ESP_PING_COUNT_INFINITE;    // ping in infinite mode, esp_ping_stop can stop it

    /* set callback functions */
    esp_ping_callbacks_t cbs;
    cbs.on_ping_success = test_on_ping_success;
    cbs.on_ping_timeout = test_on_ping_timeout;
    cbs.on_ping_end = test_on_ping_end;
    cbs.cb_args = NULL;

    esp_ping_handle_t ping;
    ESP_ERROR_CHECK(esp_ping_new_session(&ping_config, &cbs, &ping));
    esp_ping_start(ping);
}