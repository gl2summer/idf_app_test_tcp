#ifndef __TCP_THREAD_H__
#define __TCP_THREAD_H__


#ifdef __cplusplus
extern "C" {
#endif






#define EXAMPLE_DEFAULT_SERVER_IP 	CONFIG_TCP_PERF_SERVER_IP
#define EXAMPLE_DEFAULT_PORT 		CONFIG_TCP_PERF_SERVER_PORT

#define EXAMPLE_DEFAULT_PKTSIZE CONFIG_TCP_PERF_PKT_SIZE







typedef enum{
	TCP_EVT_NONE=0,
	TCP_TRY_CONNECT_FAILED,
	TCP_CONNECTED,
	TCP_DISCONNECTED,
	TCP_DATA_RECV,
}TCP_EVT;

typedef bool (*tcp_thread_cb_t)(TCP_EVT evt, void *p_data);



typedef struct{
	char data[EXAMPLE_DEFAULT_PKTSIZE];
	int len;
}tcp_data_t;








int tcpthread_put(tcp_data_t data);
int tcpthread_get(tcp_data_t *data);
void tcpthread_close(void);
void tcpthread_open(char *ip, int port, tcp_thread_cb_t cb);




#ifdef __cplusplus
}
#endif


#endif /*#ifndef __TCP_THREAD_H__*/

