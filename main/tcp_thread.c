/* tcp_perf Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/


/*
tcp_perf example

Using this example to test tcp throughput performance.
esp<->esp or esp<->ap

step1:
    init wifi as AP/STA using config SSID/PASSWORD.

step2:
    create a tcp server/client socket using config PORT/(IP).
    if server: wating for connect.
    if client connect to server.
step3:
    send/receive data to/from each other.
    if the tcp connect established. esp will send or receive data.
    you can see the info in serial output.
*/

#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_err.h"

#include "tcp_thread.h"


#define TAG "tcp_thread:"

tcp_thread_cb_t tcp_cb = NULL;
char target_ip_str[16+1]={0};
int target_port = -1;


/*socket*/
static int server_socket = -1;
static struct sockaddr_in server_addr;
static struct sockaddr_in client_addr;
static unsigned int socklen = sizeof(client_addr);
static int connect_socket = -1;


static TaskHandle_t pth_recv_data_task = NULL;
static tcp_data_t recv_buff;






int get_socket_error_code(int socket)
{
    int result;
    u32_t optlen = sizeof(int);
    if(getsockopt(socket, SOL_SOCKET, SO_ERROR, &result, &optlen) == -1) {
		ESP_LOGE(TAG, "getsockopt failed");
		return -1;
    }
    return result;
}

int show_socket_error_reason(int socket)
{
    int err = get_socket_error_code(socket);
    ESP_LOGW(TAG, "socket error %d %s", err, strerror(err));
    return err;
}

int check_working_socket()
{
    int ret;
#if EXAMPLE_ESP_TCP_MODE_SERVER
    ESP_LOGD(TAG, "check server_socket");
    ret = get_socket_error_code(server_socket);
    if(ret != 0) {
	ESP_LOGW(TAG, "server socket error %d %s", ret, strerror(ret));
    }
    if(ret == ECONNRESET)
	return ret;
#endif
    ESP_LOGD(TAG, "check connect_socket");
    ret = get_socket_error_code(connect_socket);
    if(ret != 0) {
    	//ESP_LOGW(TAG, "connect socket error %d %s", ret, strerror(ret));
    }

	return ret;
}

static void close_socket()
{
	if(connect_socket){
		close(connect_socket);
		connect_socket = -1;
	}
	if(server_socket){
		close(server_socket);
		server_socket = -1;
	}
}



//use this esp32 as a tcp server. return ESP_OK:success ESP_FAIL:error
static esp_err_t create_tcp_server()
{
	if(server_socket!=0)
		return ESP_OK;

    ESP_LOGI(TAG, "server socket....port=%d\n", EXAMPLE_DEFAULT_PORT);
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
    	show_socket_error_reason(server_socket);
    	return ESP_FAIL;
    }
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(EXAMPLE_DEFAULT_PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
    	show_socket_error_reason(server_socket);
    	close(server_socket);
    	return ESP_FAIL;
    }
    if (listen(server_socket, 5) < 0) {
    	show_socket_error_reason(server_socket);
    	close(server_socket);
    	return ESP_FAIL;
    }
    connect_socket = accept(server_socket, (struct sockaddr*)&client_addr, &socklen);
    if (connect_socket < 0) {
    	show_socket_error_reason(connect_socket);
    	close(server_socket);
    	return ESP_FAIL;
    }
    /*connection established，now can send/recv*/
    ESP_LOGI(TAG, "tcp connection established!");
    return ESP_OK;
}

//use this esp32 as a tcp client. return ESP_OK:success ESP_FAIL:error
static esp_err_t create_tcp_client()
{
	if(connect_socket >= 0){
		return ESP_OK;
	}


	char *ip = strlen(target_ip_str) ? (target_ip_str) : (EXAMPLE_DEFAULT_SERVER_IP);
	int port = (target_port>0) ? (target_port) : (EXAMPLE_DEFAULT_PORT);

    ESP_LOGI(TAG, "client socket....serverip:port=%s:%d\n", ip, port);
    connect_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (connect_socket < 0) {
    	show_socket_error_reason(connect_socket);
    	return ESP_FAIL;
    }
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip);
    ESP_LOGI(TAG, "connecting to server...");
    if (connect(connect_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    	show_socket_error_reason(connect_socket);
    	return ESP_FAIL;
    }
    ESP_LOGI(TAG, "connect to server success %d!", connect_socket);

    return ESP_OK;
}


//send data
static int send_data(char *p_data, int dat_len)
{
    if((p_data==NULL)||(dat_len<=0))
    	return 0;
    if(connect_socket < 0)
    	return 0;

    //send function
	int len = send(connect_socket, p_data, dat_len, 0);
	if((len<=0)&&(LOG_LOCAL_LEVEL >= ESP_LOG_DEBUG)) {
    	show_socket_error_reason(connect_socket);
    }
	return len;
}

//receive data
static void recv_data_task(void *pvParameters)
{
    while (1) {
    	if(connect_socket >= 0){
        	recv_buff.len = recv(connect_socket, recv_buff.data, sizeof(recv_buff.data), 0);
    		if (recv_buff.len > 0) {
    			if(tcp_cb){
    				if(tcp_cb(TCP_DATA_RECV, &recv_buff)==true){
    					recv_buff.len = 0;
    					memset(recv_buff.data, 0, sizeof(recv_buff.data));
    				}
    			}
    		} else {
    				if (LOG_LOCAL_LEVEL >= ESP_LOG_DEBUG) {
    				show_socket_error_reason(connect_socket);
    			}
    			vTaskDelay(100 / portTICK_RATE_MS);
    		}
    	}
    	else{
    		ESP_LOGI(TAG, "not connect yet!");
    		vTaskDelay(1000 / portTICK_RATE_MS);
    	}
    }
}
//this task establish a TCP connection and receive data from TCP
static void tcp_conn_task(void *pvParameters)
{
    ESP_LOGI(TAG, "task tcp_conn.");
    /*create tcp socket*/
    int socket_ret;
    
#if EXAMPLE_ESP_TCP_MODE_SERVER
    ESP_LOGI(TAG, "tcp_server will start after 3s...");
    vTaskDelay(3000 / portTICK_RATE_MS);
    ESP_LOGI(TAG, "create_tcp_server.");
    socket_ret = create_tcp_server();

#else /*EXAMPLE_ESP_TCP_MODE_SERVER*/
    ESP_LOGI(TAG, "tcp_client will start after 10s...");
    vTaskDelay(10000 / portTICK_RATE_MS);
    ESP_LOGI(TAG, "create_tcp_client.");
    socket_ret = create_tcp_client();
#endif
    if(socket_ret == ESP_FAIL) {
    	ESP_LOGI(TAG, "create tcp socket error,stop.");
		vTaskDelete(NULL);
		if(tcp_cb){
			tcp_cb(TCP_TRY_CONNECT_FAILED, NULL);
		}
    }

	if(tcp_cb){
		tcp_cb(TCP_CONNECTED, NULL);
	}

	xTaskCreate(&recv_data_task, "recv_data_task", 4096, NULL, 4, &pth_recv_data_task);

	/*
	while(1){
		if(tcp_cb){
			tcp_cb(TCP_DISCONNECTED, NULL);
		}
	}*/
    vTaskDelete(NULL);
}




int tcpthread_put(tcp_data_t data){
	return send_data(data.data, data.len);
}

int tcpthread_get(tcp_data_t *data){
	if(data==NULL)
		return 0;

	data->len = recv_buff.len;
	memcpy(data->data, recv_buff.data, sizeof(recv_buff.data));
	return data->len;
}


void tcpthread_close(void){
	close_socket();
	tcp_cb = NULL;
}


void tcpthread_open(char *ip, int port, tcp_thread_cb_t cb)
{
	tcp_cb = cb;
	if(ip){
		strcpy(target_ip_str, ip);
	}
	target_port = port;

    xTaskCreate(&tcp_conn_task, "tcp_conn_task", 4096, NULL, 5, NULL);
}
