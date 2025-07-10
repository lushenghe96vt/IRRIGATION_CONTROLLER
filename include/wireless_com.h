#pragma once

// for sensor board, to send moisture data to main board

void send_moisture_data(const char *json_payload, const char *server_ip);