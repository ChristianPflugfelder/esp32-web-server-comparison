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

#include <string.h>
#include <stdio.h>
#include "application_core.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_log.h"

int create_server_socket();
void client_handler(void *parameter);
void tcp_server(void *pvParam);
int get_header_value(const char *data, const char *header);
char* get_header(const char *data, const char *header);
int len_helper(unsigned x);


// LOG
static const char *TAG = "tcp server";

// TCP Server
#define MESSAGE "HTTP/1.1 200 OK\r\nDate: Fri, 26 Aug 2022 21:59:59 GMT\r\nServer: ESP32\r\nContent-Type: text/plain; charset=utf-8\r\nContent-Length: %i\r\n\r\nfree_heap=%i;"
#define LISTENQ 2



// Main --------------------------------------------

void app_main()
{	
    ESP_ERROR_CHECK(nvs_flash_init());
    esp_netif_init();
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Connect to Wifi
    ESP_ERROR_CHECK(example_connect());

    // blocking UART I/O
    ESP_ERROR_CHECK(example_configure_stdin_stdout());

    xTaskCreate(&tcp_server,"tcp_server",3584,NULL,5,NULL);
}



// TCP Server --------------------------------------

void tcp_server(void *pvParam){
    ESP_LOGI(TAG,"tcp_server task started \n");
    int server_socket, client_socket;
    static struct sockaddr_in remote_addr;
    static unsigned int socklen = sizeof(remote_addr);

    while((server_socket = create_server_socket()) < 0){
        vTaskDelay(4000 / portTICK_PERIOD_MS); //retry
    }

    while(1){
        client_socket=accept(server_socket, (struct sockaddr *)&remote_addr, &socklen);
        
        //set O_BLOCK because own thread
        fcntl(client_socket, F_SETFL, ~O_NONBLOCK);
        
        xTaskCreate(&client_handler,"client_handler",3584,(void*)&client_socket,5,NULL);
    }

    ESP_LOGE(TAG, "Server closed");
    close(server_socket);
}

int create_server_socket(){

    struct sockaddr_in tcpServerAddr;
    tcpServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    tcpServerAddr.sin_family = AF_INET;
    tcpServerAddr.sin_port = htons( 80 );

    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(server_socket < 0) {
        ESP_LOGE(TAG, "... Failed to allocate socket.\n");
        return -1;
    }
    ESP_LOGI(TAG, "... allocated socket\n");

    if(bind(server_socket, (struct sockaddr *)&tcpServerAddr, sizeof(tcpServerAddr)) != 0) {
        ESP_LOGE(TAG, "... socket bind failed errno=%d \n", errno);
        close(server_socket);
        return -1;
    }
    ESP_LOGI(TAG, "... socket bind done \n");

    if(listen (server_socket, LISTENQ) != 0) {
        ESP_LOGE(TAG, "... socket listen failed errno=%d \n", errno);
        close(server_socket);
        return -1;
    }

    return server_socket;
}



// Client Handler ----------------------------------

void client_handler(void *parameter){

    int client_socket = *(int*)parameter;

    char recv_buf[1024];
    char send_buf[200];

    while (1){
        //Read
        bzero(recv_buf, sizeof(recv_buf));
        recv(client_socket, recv_buf, sizeof(recv_buf)-1,0);

        int delay = get_header_value(recv_buf, "Delay");
        printf("\tDelay: %d\n", delay);
        
        if(delay > 0){
            vTaskDelay(delay / portTICK_PERIOD_MS);
        }
        

        printf("\tFree heap size: %d\n", esp_get_free_heap_size());
        int free_heap = esp_get_free_heap_size();
        int content_len = 11 + len_helper(free_heap);

        
        snprintf(send_buf, sizeof(send_buf), MESSAGE, content_len, free_heap);

        printf("%d - Finish reading from socket\n", client_socket);

        //Write
        if( write(client_socket , send_buf , strlen(send_buf)) < 0)
        {
            ESP_LOGE(TAG, "Send failed");
            break;
        }
        printf("%d - Response send successfully\n\n", client_socket);
    }

    
    printf(">>> Socket %d closed\n\n", client_socket);
    close(client_socket);
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
    ESP_LOGE(TAG, "Problem reading header");
    return NULL;
  }
    
  char *value = malloc(sizeof(char) * (value_length + 2));

  strncpy(value, header_value_start, value_length);
  value[value_length] = '\0';

  return value;
}