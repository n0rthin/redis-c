#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "thread_pool.h"

#define MSG_MAX_LEN 80
#define CONN_BACKLOG 5
#define NUM_THREADS 5

typedef struct
{
	int fd;
	bool free;
} Conn;

void handle_conn(void *arguments);
void format_msg(char buff[MSG_MAX_LEN], char formated_msg[MSG_MAX_LEN]);

int main()
{
	// Disable output buffering
	setbuf(stdout, NULL);

	int server_fd, client_addr_len;
	Conn conns[CONN_BACKLOG];
	struct sockaddr_in client_addr;
	Pool pool;
	pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == -1)
	{
		printf("Socket creation failed: %s...\n", strerror(errno));
		return 1;
	}
	// Since the tester restarts your program quite often, setting SO_REUSEADDR
	// ensures that we don't run into 'Address already in use' errors
	int reuse = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
	{
		printf("SO_REUSEADDR failed: %s \n", strerror(errno));
		return 1;
	}

	struct sockaddr_in serv_addr = {
			.sin_family = AF_INET,
			.sin_port = htons(6379),
			.sin_addr = {htonl(INADDR_ANY)},
	};

	if (bind(server_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) != 0)
	{
		printf("Bind failed: %s \n", strerror(errno));
		return 1;
	}

	if (listen(server_fd, CONN_BACKLOG) != 0)
	{
		printf("Listen failed: %s \n", strerror(errno));
		return 1;
	}

	if (!init_thread_pool(&pool, NUM_THREADS))
	{
		printf("Failed to initialize thread pool\n");
		return 1;
	}

	int i;
	Conn default_conn = {-1, false};
	client_addr_len = sizeof(client_addr);

	printf("Waiting for a client to connect...\n");
	for (;;)
	{
		Conn *conn = &default_conn;

		for (i = 0; i < CONN_BACKLOG; i++)
		{
			if (conns[i].free)
			{
				conn = &conns[i];
				conn->free = false;
				break;
			}
		}

		if (conn->fd == -1)
			continue;

		conn->fd = accept(server_fd, (struct sockaddr *)&client_addr, (socklen_t *)&client_addr_len);
		if (conn->fd < 0)
		{
			printf("Failed to accept connection: %s \n", strerror(errno));
			continue;
		}
		else
			printf("[%d] Client connected\n", conn->fd);

		if (!thread_pool_add_job(&pool, handle_conn, (void *)conn))
		{
			printf("[%d] Failed to add job to pool", conn->fd);
			close(conn->fd);
		}
	}

	close(server_fd);
	thread_pool_destroy(&pool);

	return 0;
}

void handle_conn(void *arguments)
{
	Conn *conn = (Conn *)arguments;
	char msg[] = "+PONG\r\n";
	char buff[MSG_MAX_LEN];
	char formated_msg[MSG_MAX_LEN];
	int len;

	for (;;)
	{
		memset(buff, 0, MSG_MAX_LEN);
		memset(formated_msg, 0, MSG_MAX_LEN);

		len = read(conn->fd, buff, sizeof(buff));
		if (len == 0)
		{
			printf("[%d] Connection closed\n", conn->fd);
			break;
		}
		else if (len < 0)
		{
			printf("[%d] Failed to read socket message: %s \n", conn->fd, strerror(errno));
			break;
		}

		format_msg(buff, formated_msg);
		printf("[%d] Received nessage: %s\n", conn->fd, formated_msg);

		printf("[%d] Writing message: %s \n", conn->fd, msg);
		if (write(conn->fd, msg, strlen(msg)) < 0)
		{
			printf("[%d] Failed to write to socket: %s \n", conn->fd, strerror(errno));
		}
	}

	close(conn->fd);
	conn->free = true;
}

void format_msg(char buff[MSG_MAX_LEN], char formated_msg[MSG_MAX_LEN])
{
	int i, j;
	for (i = 0, j = 0; i < MSG_MAX_LEN; i++, j++)
	{
		if (buff[i] == '\n')
		{
			formated_msg[j++] = '\\';
			formated_msg[j] = 'n';
		}
		else if (buff[i] == '\r')
		{
			formated_msg[j++] = '\\';
			formated_msg[j] = 'r';
		}
		else if (buff[i] == '\t')
		{
			formated_msg[j++] = '\\';
			formated_msg[j] = 't';
		}
		else
		{
			formated_msg[j] = buff[i];
		}
	}
}