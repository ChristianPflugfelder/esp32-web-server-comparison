#include <stdio.h>
#include <sys/fcntl.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "lwip/err.h"
#include "lwip/sys.h"

#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/api.h"

#include <string.h>
#include <stdio.h>
#include "application_core.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_log.h"

void start_server();
void tcp_server(void *arg);
void handle_client(void *arg);
int get_header_value(const char *data, const char *header);
char* get_header(const char *data, const char *header);
int len_helper(unsigned x);


//quelle https://community.st.com/s/question/0D53W00000kvg9USAQ/http-server-example-based-on-lwip-netconn

#define MESSAGE "HTTP/1.1 200 OK\r\nDate: Fri, 26 Aug 2022 21:59:59 GMT\r\nServer: ESP32\r\nContent-Type: text/plain; charset=utf-8\r\nContent-Length: %i\r\n\r\nfree_heap=%i;"


// Main --------------------------------------------

void app_main()
{	

    ESP_ERROR_CHECK(nvs_flash_init());
    esp_netif_init();
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());

    /* This helper function configures blocking UART I/O */
    ESP_ERROR_CHECK(example_configure_stdin_stdout());

	start_server();
}


// TCP Server --------------------------------------

void start_server()
{
	printf("Start server\n");
		
	struct netconn *conn = netconn_new(NETCONN_TCP);

	if(conn!=NULL)
	{
		if(netconn_bind(conn, NULL, 80) == ERR_OK)
		{
			netconn_listen(conn);
			xTaskCreate(&tcp_server, "tcp_server", 3584, (void*)conn, 5, NULL);
			return;
		}
		
		netconn_delete(conn);
	}
	printf("Error creating server\n");
}



// TCP Server --------------------------------------

void tcp_server(void *arg)
{
	printf("Server running\n");
	struct netconn *conn  = (struct netconn*) arg;
	            			

    for(;;)
	{
		struct netconn *newconn; // Structure for client connection

		if (netconn_accept(conn, &newconn) != ERR_OK)
		{
			printf(">> Error accepting request\n");
			vTaskDelay(1000 / portTICK_PERIOD_MS);
			continue;
		}
		
		xTaskCreate(&handle_client, "handle_client", 3072, (void*)newconn, 5, NULL);
	}
}



// Client Handler ----------------------------------

void handle_client(void *arg)
{
	printf("Client connected\n");

	struct netconn *newconn= (struct netconn*) arg;

	struct netbuf *inbuf;                			// Pointer to connection buffer
	u16_t recv_buflen;
	char* recv_buf;
    char send_buf[200];

    while(1)
	{
		if (netconn_recv(newconn, &inbuf) != ERR_OK)
		{
			printf("Error reading\n");
			break;
		}

		printf("\trecv done\n");
		
    //the netbuf can be fragmented -> can use netbuf_first() and netbuf_next() to get all data
		netbuf_data(inbuf, (void**)&recv_buf, &recv_buflen); 

        int delay = get_header_value(recv_buf, "Delay");
        printf("\tDelay: %d\n", delay);

        if(delay > 0){
            vTaskDelay(delay / portTICK_PERIOD_MS);
        }

        int free_heap = esp_get_free_heap_size();
        int content_len = 11 + len_helper(free_heap);
        snprintf(send_buf, sizeof(send_buf), MESSAGE, content_len, free_heap);

		
		if(netconn_write(newconn, send_buf, strlen(send_buf), NETCONN_NOCOPY) != ERR_OK)
		{
			printf("Error writing\n");
			break;
		}
		
		netbuf_delete(inbuf);
		printf("\twrite done\n");
	}
	
	
	printf("Connection closed\n");
	netconn_close(newconn);
	netconn_delete(newconn);
    vTaskDelete(NULL);
}

int len_helper(unsigned x) {
    if (x >= 1000000000) return 10;
    if (x >= 100000000)  return 9;
    if (x >= 10000000)   return 8;
    if (x >= 1000000)    return 7;
    if (x >= 100000)     return 6;
    if (x >= 10000)      return 5;
    if (x >= 1000)       return 4;
    if (x >= 100)        return 3;
    if (x >= 10)         return 2;
    return 1;
}




// HTTP Parsen -------------------------------------

int get_header_value(const char *data, const char *header)
{
  int value_as_int;

  char *header_value = get_header(data, header);

  if(header_value != NULL){
    sscanf(header_value, "%d", &value_as_int);
    free(header_value);

    return value_as_int;
  }

  return 0;
}

char* get_header(const char *data, const char *header)
{
  char *header_start = strstr(data, header);
  if(header_start == NULL){
    return NULL;
  }

  char *header_value_start = strstr(header_start, ":") + 2 * sizeof(char);
  char *header_value_end = strstr(header_start, "\r\n");
  int value_length = header_value_end - header_value_start;

  if(value_length <= 0){
	printf("Problem reading header\n");
    return NULL;
  }
    
  char *value = malloc(sizeof(char) * (value_length + 2));

  strncpy(value, header_value_start, value_length);
  value[value_length] = '\0';

  return value;
}
