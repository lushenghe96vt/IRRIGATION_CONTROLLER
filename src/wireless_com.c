#include "wireless_com.h"
#include "esp_http_client.h"
#include "esp_log.h"

#define TAG "HTTP_CLIENT"

void send_moisture_data(const char *json_payload, const char *server_ip) {
    char url[64];
    snprintf(url, sizeof(url), "http://%s/moisture", server_ip);

    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_POST };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, json_payload, strlen(json_payload));

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) ESP_LOGI(TAG, "POST success. Status = %d", esp_http_client_get_status_code(client));
    
    else ESP_LOGE(TAG, "POST failed: %s", esp_err_to_name(err));

    esp_http_client_cleanup(client);
}