#include <string.h>
#include <hal/log.h>
#include <sys/socket.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "udp.h"

static const char *TAG = "Udp";

static udp_t udp;
// static int socket_obj = 0;

static struct sockaddr_in server_addr;
static struct sockaddr_in remote_addr;
static unsigned int socklen;

void wifi_udp_init() 
{
	udp.socket_obj_client = 0;
	udp.socket_obj_server = 0;
	udp.server_port = UDP_PORT;
	udp.server_ip = 0;
	udp.remote_ip = 0;
}

static int get_socket_error_code(int socket) 
{
	int result;
	u32_t optlen = sizeof(int);
	if (getsockopt(socket, SOL_SOCKET, SO_ERROR, &result, &optlen) == -1) {
		LOG_W(TAG, "getsockopt failed");
		return -1;
	}
	return result;
}

static int show_socket_error_reason(int socket) 
{
	int err = get_socket_error_code(socket);
	LOG_W(TAG, "socket error %d %s", err, strerror(err));
	return err;
}

static void setnonblocking(int sockfd) {
    int flag = fcntl(sockfd, F_GETFL, 0);
    if (flag < 0) {
		LOG_I(TAG, "fcntl F_GETFL fail");
        return;
    }
    if (fcntl(sockfd, F_SETFL, flag | O_NONBLOCK) < 0) {
		LOG_I(TAG, "fcntl F_SETFL fail");
    }
}

//create a udp client socket. return ESP_OK:success ESP_FAIL:error
esp_err_t wifi_create_udp_client() {

    if (udp.socket_obj_client != 0)
    {
        close(udp.socket_obj_client);
    }

	LOG_I(TAG, "create_udp_client()");
	// LOG_I(TAG, "connecting to %s:%d", ip4addr_ntoa(&udp.server_ip), udp.server_port);

	udp.socket_obj_client = socket(AF_INET, SOCK_DGRAM, 0);

	if (udp.socket_obj_client < 0) {
		show_socket_error_reason(udp.socket_obj_client);
		return ESP_FAIL;
	}
	/*for client remote_addr is also server_addr*/
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(UDP_PORT);
	server_addr.sin_addr.s_addr = udp.server_ip;

	return ESP_OK;
}

//create a udp server socket. return ESP_OK:success ESP_FAIL:error
esp_err_t wifi_create_udp_server() {

    if (udp.socket_obj_server != 0)
    {
        close(udp.socket_obj_server);
    }

	LOG_I(TAG, "Create Udp Server port : %d \n", udp.server_port);

	udp.socket_obj_server = socket(AF_INET, SOCK_DGRAM, 0);

	if (udp.socket_obj_server < 0) {
		show_socket_error_reason(udp.socket_obj_server);
		return ESP_FAIL;
	}

	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(udp.server_port);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	setnonblocking(udp.socket_obj_server);

	if (bind(udp.socket_obj_server, (struct sockaddr *) &server_addr, sizeof(server_addr))
			< 0) {
		show_socket_error_reason(udp.socket_obj_server);
		close(udp.socket_obj_server);
		return ESP_FAIL;
	}

	return ESP_OK;
}

void wifi_udp_close()
{
    if (udp.socket_obj_server != 0)
    {
        close(udp.socket_obj_server);
    }

	if (udp.socket_obj_client != 0)
    {
        close(udp.socket_obj_client);
    }
}

void wifi_udp_set_server_ip(uint32_t *ip)
{
	udp.server_ip = *ip;
    server_addr.sin_addr.s_addr = udp.server_ip;
	server_addr.sin_port = htons(UDP_PORT);
}

int wifi_udp_send(char *buffer, int length) 
{	int result = sendto(udp.socket_obj_client, buffer, length, 0,
			(struct sockaddr *) &server_addr, sizeof(server_addr));
	// LOG_I(TAG, "Send Data: %d", result);

	LOG_D(TAG, "Send to : %s:%u -> %d", inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port), length);

	return result;
}

int wifi_udp_receive(char *buffer, int length) 
{
	int len = 0;
	socklen = sizeof(remote_addr);
	memset(buffer, 0x00, length);

	// start recive
	len = recvfrom(udp.socket_obj_server, buffer, length, 0, (struct sockaddr *) &remote_addr, &socklen);
	
	// print recived data
	if (len > 0) 
	{
		if (udp.remote_ip == 0 || udp.remote_ip != remote_addr.sin_addr.s_addr || remote_addr.sin_addr.s_addr != server_addr.sin_addr.s_addr)
		{
			udp.remote_ip = remote_addr.sin_addr.s_addr;
			server_addr.sin_addr.s_addr = remote_addr.sin_addr.s_addr;
			LOG_I(TAG, "Remote ip: %d.%d.%d.%d", (uint8_t)(udp.remote_ip), (uint8_t)(udp.remote_ip >> 8), (uint8_t)(udp.remote_ip >> 16), (uint8_t)(udp.remote_ip >> 24));
		}

		LOG_D(TAG, "Receive Data: %d", len);
	}
	if (len <= 0 && LOG_LOCAL_LEVEL >= ESP_LOG_DEBUG) {
		show_socket_error_reason(udp.socket_obj_server);
	}

    return len;
}