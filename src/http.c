#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include "http.h"

#define BUF_SIZE 1024
#define HTTP_GET "GET /%s HTTP/1.0\r\nHost: %s\r\nRange: bytes=%s\r\nUser-Agent: getter\r\n\r\n"
#define HTTP_HEAD "HEAD /%s HTTP/1.0\r\nHost: %s\r\n\r\n"


/**
 * Creates a TCP socket from a given host and port and gives
 * the file descriptor for the socket
 * 
 * @params host - A pointer to the hostname to connect to
 * @params port - The integer of the port number
 * 
 * @return int - The socket file descriptor that was created
 */
int create_connection(char *host, int port) {
    struct addrinfo serv_addrinfo;
    struct addrinfo *addr = NULL;
    int sockfd;
    char port_str[20];

    // Get port number
    int n = snprintf(port_str, 20, "%d", port);
    if (n > 20 || n < 0) {
        perror("Port Forming");
    }

    // Create TCP socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
    }

    memset(&serv_addrinfo, 0, sizeof(struct addrinfo));

    // Set addrinfo paramaters 
    serv_addrinfo.ai_family = AF_INET; 
    serv_addrinfo.ai_socktype = SOCK_STREAM;
    serv_addrinfo.ai_protocol = PF_UNSPEC;
    
    getaddrinfo(host, port_str, &serv_addrinfo, &addr);

    // Connect the socket
    if(connect(sockfd, addr->ai_addr, addr->ai_addrlen) != 0) {
        perror("Connect");
    }

    freeaddrinfo(addr);    
    return sockfd;
}

/*
 * Sends data to a given socket file decriptor, the content 
 * contains the header and data to send
 * 
 * @param content - The content of reques to send
 * @param sockfd - The socket file discriptor to send the data to
 * 
 * @return int - showing if the operation was successful 0 if not 1
 *               if it was successful
 */
int http_send(char *content, int sockfd) {

    int n = write(sockfd, content, strlen(content));
    if (n < strlen(content)) {
        return -1;
    }
    return 0;
}

/**
 * Takes a buffer and a socket file descriptor and puts the 
 * incoming data from the socket into the buffer
 * 
 * @param buffer - Pointer to a buffer
 * @param sockfd - The file descriptor for the socket to be read
 *                 from
 * 
 * @return int - Returns 0 if read successful -1 if the read failed
 */
int http_recieve(Buffer *buffer, int sockfd) {
    int n;
    char data[BUF_SIZE];
    do {
        n = read(sockfd, data, BUF_SIZE);
        if (n < 0) {
            return -1;
        }
        buffer_append(buffer, data, n);
    } while (n > 0);

    return 0;
}


/**
 * Perform an HTTP 1.0 query to a given host and page and port number.
 * host is a hostname and page is a path on the remote server. The query
 * will attempt to retrievev content in the given byte range.
 * User is responsible for freeing the memory.
 * 
 * @param host - The host name e.g. www.canterbury.ac.nz
 * @param page - e.g. /index.html
 * @param range - Byte range e.g. 0-500. NOTE: A server may not respect this
 * @param port - e.g. 80
 * @return Buffer - Pointer to a buffer holding response data from query
 *                  NULL is returned on failure.
 */
Buffer* http_query(char *host, char *page, const char *range, int port) {
    int sockfd;
    Buffer *request, *recieve;
    
    // Create the request and recieve buffers
    request = create_buffer(BUF_SIZE);
    recieve = create_buffer(BUF_SIZE);
    // Writes the formatted string into the request buffer
    snprintf(request->data, BUF_SIZE-1, HTTP_GET, page, host, range);
    
    // Create the socket for connection
    sockfd = create_connection(host, port);
    
    // Send data to the socket
    int send_status = http_send(request->data, sockfd);
    if (send_status != 0) {
        perror("Send query");
    }

    // Recieve data from the socket
    int recieve_status = http_recieve(recieve, sockfd);
    if (recieve_status != 0) {
        perror("Recieve query");
    }

    // Free alloc'd data
    buffer_free(request);
    close(sockfd);

    // Return the recieved data
    return recieve;    
}

/**
 * Creates an empty buffer of size length returning 
 * a pointer to the buffer
 * @param length - The length of the buffer to be created
 * @return Buffer* buffer - The buffer that was just created
 */
Buffer* create_buffer(size_t length) {
	Buffer *buffer = (Buffer*)malloc(sizeof(Buffer));
	buffer->data = (char*)malloc(sizeof(char)*length);
	if (!buffer->data) {
		perror("Malloc Buffer");
	}
	
	memset(buffer->data, '\0', length);
	buffer->length = 0;
	
	return buffer;
}

/**
 * Appends char data of size length to the given buffer
 * @param buffer - Pointer to a allocated buffer
 * @param data - Pointer to a char array of data to append
 *               into buffer
 * @param length - The length of the data to be appended
 */
void buffer_append(Buffer *buffer, char *data, size_t length) {
    char *new_data = (char*)realloc(buffer->data, length+buffer->length);
    if (!new_data) {
	    perror("Append Buffer");
    }
     
    memset(new_data+buffer->length, '\0', length);
    
    void *dest = memcpy(new_data+buffer->length, data, length);
    if (!dest) {
        perror("Memcpy");
    }

    buffer->data = new_data;
    buffer->length = buffer->length + length;
}


/**
 * Separate the content from the header of an http request.
 * NOTE: returned string is an offset into the response, so
 * should not be freed by the user. Do not copy the data.
 * @param response - Buffer containing the HTTP response to separate 
 *                   content from
 * @return string response or NULL on failure (buffer is not HTTP response)
 */
char* http_get_content(Buffer *response) {

    char* header_end = strstr(response->data, "\r\n\r\n");

    if (header_end) {
        return header_end + 4;
    }
    else {
        return response->data;
    }
}


/**
 * Splits an HTTP url into host, page. On success, calls http_query
 * to execute the query against the url. 
 * @param url - Webpage url e.g. learn.canterbury.ac.nz/profile
 * @param range - The desired byte range of data to retrieve from the page
 * @return Buffer pointer holding raw string data or NULL on failure
 */
Buffer *http_url(const char *url, const char *range) {
    char host[BUF_SIZE];
    strncpy(host, url, BUF_SIZE);

    char *page = strstr(host, "/");
    
    if (page) {
        page[0] = '\0';

        ++page;
        return http_query(host, page, range, 80);
    }
    else {

        fprintf(stderr, "could not split url into host/page %s\n", url);
        return NULL;
    }
}


/**
 * Makes a HEAD request to a given URL and gets the content length
 * Then determines max_chunk_size and number of split downloads needed
 * @param url   The URL of the resource to download
 * @param threads   The number of threads to be used for the download
 * @return int  The number of downloads needed satisfying max_chunk_size
 *              to download the resource
 */
int get_num_tasks(char *url, int threads) {
    char host[BUF_SIZE];

    strncpy(host, url, BUF_SIZE);

    char *page = strstr(host, "/");
    if (page) {
        page[0] = '\0';
        page++;
    }
    else {
        perror("URL split failed");
    }

    char request[BUF_SIZE];
    Buffer *recieve = create_buffer(BUF_SIZE);

    snprintf(request, BUF_SIZE-1, HTTP_HEAD, page, host);

    int sockfd = create_connection(host, 80);
    http_send(request, sockfd);
    http_recieve(recieve, sockfd);

    char *start_content = strstr(recieve->data, "Content-Length: ");
    start_content += strlen("Content-Length: ");
    char *end_content = strstr(start_content, "\r\n");
    end_content++;

    char content_length[25];
    strncpy(content_length, start_content, ((size_t)end_content - (size_t)start_content));
    int length = atoi(content_length);

    if (strstr(recieve->data, "Accept-Ranges: bytes") == NULL) {
        max_chunk_size = length;
        buffer_free(recieve);
        close(sockfd);
        return 1;
    }

    max_chunk_size = (length / threads) + 1;

    buffer_free(recieve);

    close(sockfd);

    return threads;
}


int get_max_chunk_size() {
    return max_chunk_size;
}
