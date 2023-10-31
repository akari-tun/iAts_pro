#pragma once

#include <esp_err.h>
#include <sys/socket.h>
#define UDP_PORT 8898


typedef struct udp_s
{
    int server_port;
    uint32_t server_ip;
    int socket_obj_client;
    int socket_obj_server;
    uint32_t remote_ip;
} udp_t;

void wifi_udp_init();
esp_err_t wifi_create_udp_server();
esp_err_t wifi_create_udp_client();
void wifi_udp_close();
void wifi_udp_set_server_ip(uint32_t *ip);
int wifi_udp_send(char *buffer, int length);
int wifi_udp_receive(char *buffer, int length);