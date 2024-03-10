#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#define MAX 80

void handle_conn(int conn_fd);
void format_msg(char buff[MAX], char formated_msg[MAX]);

int main()
{
	// Disable output buffering
	setbuf(stdout, NULL);

	int server_fd, conn_fd, client_addr_len;
	struct sockaddr_in client_addr;

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

	int connection_backlog = 5;
	if (listen(server_fd, connection_backlog) != 0)
	{
		printf("Listen failed: %s \n", strerror(errno));
		return 1;
	}

	printf("Waiting for a client to connect...\n");
	client_addr_len = sizeof(client_addr);

	conn_fd = accept(server_fd, (struct sockaddr *)&client_addr, (socklen_t *)&client_addr_len);
	if (conn_fd < 0)
	{
		printf("Server accept failed: %s \n", strerror(errno));
		return 1;
	}
	else
		printf("Client connected\n");

	handle_conn(conn_fd);

	close(server_fd);

	return 0;
}

void handle_conn(int conn_fd)
{
	char msg[] = "+PONG\r\n";
	char buff[MAX];
	char formated_msg[MAX];
	int len;

	for (;;)
	{
		memset(buff, 0, MAX);
		memset(formated_msg, 0, MAX);

		len = read(conn_fd, buff, sizeof(buff));
		if (len == 0)
		{
			printf("Connection closed\n");
			return;
		}
		else if (len < 0)
		{
			printf("Failed to read socket message: %s \n", strerror(errno));
			return;
		}

		format_msg(buff, formated_msg);
		printf("Received nessage: %s\n", formated_msg);

		write(conn_fd, msg, strlen(msg));
	}
}

void format_msg(char buff[MAX], char formated_msg[MAX])
{
	int i, j;
	for (i = 0, j = 0; i < MAX; i++, j++)
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