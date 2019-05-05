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

static struct sockaddr_in remote_addr;
static unsigned int socklen;

void wifi_udp_init() 
{
	udp.socket_obj = 0;
	udp.server_port = UDP_PORT;
	udp.server_ip = (char*)malloc(16);
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
        // Perror("fcntl F_GETFL fail");
        return;
    }
    if (fcntl(sockfd, F_SETFL, flag | O_NONBLOCK) < 0) {
        // Perror("fcntl F_SETFL fail");
    }
}

//create a udp client socket. return ESP_OK:success ESP_FAIL:error
esp_err_t wifi_create_udp_client() {

    if (udp.socket_obj != 0)
    {
        close(udp.socket_obj);
    }

	LOG_I(TAG, "create_udp_client()");
	LOG_I(TAG, "connecting to %s:%d", udp.server_ip, udp.server_port);

	udp.socket_obj = socket(AF_INET, SOCK_DGRAM, 0);

	if (udp.socket_obj < 0) {
		show_socket_error_reason(udp.socket_obj);
		return ESP_FAIL;
	}
	/*for client remote_addr is also server_addr*/
	remote_addr.sin_family = AF_INET;
	remote_addr.sin_port = htons(UDP_PORT);
	remote_addr.sin_addr.s_addr = inet_addr(udp.server_ip);

	return ESP_OK;
}

//create a udp server socket. return ESP_OK:success ESP_FAIL:error
esp_err_t wifi_create_udp_server() {

    if (udp.socket_obj != 0)
    {
        close(udp.socket_obj);
    }

	LOG_I(TAG, "Create Udp Server port : %d \n", udp.server_port);

	udp.socket_obj = socket(AF_INET, SOCK_DGRAM, 0);

	if (udp.socket_obj < 0) {
		show_socket_error_reason(udp.socket_obj);
		return ESP_FAIL;
	}

	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(udp.server_port);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	setnonblocking(udp.socket_obj);

	if (bind(udp.socket_obj, (struct sockaddr *) &server_addr, sizeof(server_addr))
			< 0) {
		show_socket_error_reason(udp.socket_obj);
		close(udp.socket_obj);
		return ESP_FAIL;
	}

	return ESP_OK;
}

void wifi_udp_close()
{
    if (udp.socket_obj != 0)
    {
        close(udp.socket_obj);
    }
}

void wifi_udp_set_server_ip(char *ip)
{
    memset(udp.server_ip, 0x00, strlen(udp.server_ip));
	strncpy(udp.server_ip, ip, strlen(ip));
}

int wifi_udp_send(char *buffer, int length) 
{
    LOG_I(TAG, "Send Data: %s", buffer);
	int result = sendto(udp.socket_obj, buffer, length, 0,
			(struct sockaddr *) &remote_addr, sizeof(remote_addr));

	return result;
}

int wifi_udp_receive(char *buffer, int length) 
{
	int len = 0;
	socklen = sizeof(remote_addr);
	memset(buffer, 0x00, length);

	// start recive
	len = recvfrom(udp.socket_obj, buffer, length, 0, (struct sockaddr *) &remote_addr, &socklen);
	// print recived data
	if (len > 0) LOG_I(TAG, "Receive Data: %s", buffer);
	if (len <= 0 && LOG_LOCAL_LEVEL >= ESP_LOG_DEBUG) {
		show_socket_error_reason(udp.socket_obj);
	}

    return len;
}