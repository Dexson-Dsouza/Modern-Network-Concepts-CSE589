/**
 * @dexsonjo_assignment1
 * @author  Dexson D'souza <dexsonjo@buffalo.edu>
 * @version 1.0
 *
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * http://www.gnu.org/copyleft/gpl.html
 *
 * @section DESCRIPTION
 *
 * This contains the main function. Add further description here....
 */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "../include/global.h"
#include "../include/logger.h"

#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/time.h>
#define STDIN 0
/**
 * main function
 *
 * @param  argc Number of arguments
 * @param  argv The argument list
 * @return 0 EXIT_SUCCESS
 */
int createSocket_server();
void bind_server(int socket);
int listen_server(int socket);
void *get_in_addr(struct sockaddr *sa);
void client(int port_no, int listening_fd);
void server(int port_no, int listening_fd);
int ValidIP(char *IP);

struct client
{
	char *ip;
	int client_fd;
	char *hostname;
	int block_status;
	int client_status;
	int port_no;
	int num_msg_recv;
	int num_msg_sent;
	struct client *next;
};

struct sockaddr_in my_info;
char IP[256];
int PORT;
struct client *head_ref = NULL;
struct client *c_ref = NULL;

int main(int argc, char **argv)
{
	/*Init. Logger*/
	cse4589_init_log(argv[2]);

	/*Clear LOGFILE*/
	fclose(fopen(LOGFILE, "w"));

	if (argc != 3)
	{
		printf("Usage:%s args %d\n", argv[0], argc);
		exit(-1);
	}
	/*Start Here*/

	PORT = atoi(argv[2]);
	int sockfd = createSocket_server();
	char s[INET6_ADDRSTRLEN];
	my_info.sin_family = AF_INET;
	my_info.sin_addr.s_addr = INADDR_ANY;
	my_info.sin_port = htons(PORT);
	bind_server(sockfd);
	int lid = listen_server(sockfd);
	// printf("Y o  /n");

	struct hostent *he;
	struct in_addr **addr_list;
	char hostname[100];
	gethostname(hostname, 1024);
	he = gethostbyname(hostname);
	addr_list = (struct in_addr **)he->h_addr_list;
	for (int i = 0; addr_list[i] != NULL; i++)
	{
		strcpy(IP, inet_ntoa(*addr_list[i]));
	}

	if (*argv[1] == 's')
	{
		// printf(" server waiting at port %d \n", PORT);
		server(atoi(argv[2]), lid);
	}
	else if (*argv[1] == 'c')
	{
		// printf("client at port %d \n", PORT);
		client(atoi(argv[2]), lid);
	}
	return 0;
}

void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET)
	{
		return &(((struct sockaddr_in *)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int createSocket_server()
{
	int sockfd = 0;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
		// cse4589_print_and_log("Can't open socket");

		// cse4589_print_and_log("Create function on server called\n");
		return sockfd;
}

void bind_server(int socket)
{

	int conn = bind(socket, (struct sockaddr *)&my_info, sizeof(my_info));
	if (conn < 0)
	{
		// printf("Bind failed\n");
		exit(EXIT_FAILURE);
	}
	// cse4589_print_and_log("Bind function on server called\n");
}

int listen_server(int sockfd)
{
	int conn = listen(sockfd, 4);
	if (conn < 0)
	{
		// printf("Listen function failed\n");
		exit(EXIT_FAILURE);
	}
	// cse4589_print_and_log("Listen function on server called\n");
	return conn;
}

void client(int port, int lid)
{
	struct sockaddr_in server_address;
	while (1)
	{
		fd_set read_fds;
		int fdmax = lid;
		FD_ZERO(&read_fds);
		FD_SET(STDIN, &read_fds);
		char buf[256];
		while (1)
		{
			if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1)
			{
				perror("select");
				exit(4);
			}
			for (int i = 0; i <= fdmax; i++)
			{
				memset((void *)&buf, '\0', sizeof(buf));
				if (FD_ISSET(i, &read_fds))
				{
					if (i == STDIN)
					{
						read(STDIN, buf, 256);
						buf[strlen(buf) - 1] = '\0';
						if (strcmp(buf, "AUTHOR") == 0)
						{
							cse4589_print_and_log("[AUTHOR:SUCCESS]\n");
							cse4589_print_and_log("I, Dexson, have read and understood the course academic integrity policy.\n");
							cse4589_print_and_log("[AUTHOR:END]\n");
						}
						else if (strcmp(buf, "IP") == 0)
						{
							cse4589_print_and_log("[IP:SUCCESS]\n");
							cse4589_print_and_log("IP:%s\n", IP);
							cse4589_print_and_log("[IP:END]\n");
						}
						else if (strcmp(buf, "PORT") == 0)
						{
							cse4589_print_and_log("[PORT:SUCCESS]\n");
							cse4589_print_and_log("PORT:%d\n", PORT);
							cse4589_print_and_log("[PORT:END]\n");
						}
						else if (strncmp(buf, "LOGIN", 5) == 0)
						{
							printf("---ARGS---\n");
							char *arr[20];
							int i = 0;
							char a[500];
							strcpy(a, buf);
							char *temp;
							temp = strtok(a, " ");
							while (temp != NULL)
							{
								arr[i] = temp;
								i++;
								temp = strtok(NULL, " ");
							}

							int valid_port = 0;
							if (strlen(arr[2]) > 4)
							{
								valid_port = 1;
							}
							for (int i = 0; i < strlen(arr[2]); i++)
							{
								if (!isdigit(arr[2][i]))
								{
									valid_port = 1;
								}
							}

							if (ValidIP(arr[1]) == 1 || i != 3 || valid_port == 1)
							{
								cse4589_print_and_log("[LOGIN:ERROR]\n");
								cse4589_print_and_log("[LOGIN:END]\n");
								printf("---ARGS-2--\n");
							}
							else
							{
								int port_temp = atoi(arr[2]);
								server_address.sin_port = htons(port_temp);
								inet_pton(AF_INET, arr[1], &(server_address.sin_addr));
								int my_fd = socket(AF_INET, SOCK_STREAM, 0);
								if (connect(my_fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
								{
									cse4589_print_and_log("[%s:ERROR]\n", "LOGIN");
									cse4589_print_and_log("[%s:END]\n", "LOGIN");
									printf("---ARGS-3--\n");
								}
								else
								{
									char buf[1024];
									strcpy(buf, (char *)"SETCLNTPORT");
									strcat(buf, " ");
									strcat(buf, arr[2]);
									if (send(my_fd, buf, strlen(buf), 0) == -1)
									{
										cse4589_print_and_log("CAnnot send \n");
									}
									cse4589_print_and_log("[%s:SUCCESS]\n", "LOGIN");
									cse4589_print_and_log("[%s:END]\n", "LOGIN");
									printf("---ARGS--4-\n");
								}
							}
						}
					}
				}
			}
		}
	}
}

void server(int port, int lid)
{
	while (1)
	{
		fd_set read_fds;
		int fdmax = lid;
		FD_ZERO(&read_fds);
		FD_SET(STDIN, &read_fds);
		char buf[256];
		while (1)
		{
			if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1)
			{
				perror("select");
				exit(4);
			}
			for (int i = 0; i <= fdmax; i++)
			{
				memset((void *)&buf, '\0', sizeof(buf));
				if (FD_ISSET(i, &read_fds))
				{
					if (i == STDIN)
					{
						read(STDIN, buf, 256);
						buf[strlen(buf) - 1] = '\0';
						if (strcmp(buf, "AUTHOR") == 0)
						{
							cse4589_print_and_log("[AUTHOR:SUCCESS]\n");
							cse4589_print_and_log("I, Dexson, have read and understood the course academic integrity policy.\n");
							cse4589_print_and_log("[AUTHOR:END]\n");
						}
						else if (strcmp(buf, "IP") == 0)
						{
							cse4589_print_and_log("[IP:SUCCESS]\n");
							cse4589_print_and_log("IP:%s\n", IP);
							cse4589_print_and_log("[IP:END]\n");
						}
						if (strcmp(buf, "PORT") == 0)
						{
							cse4589_print_and_log("[PORT:SUCCESS]\n");
							cse4589_print_and_log("PORT:%d\n", PORT);
							cse4589_print_and_log("[PORT:END]\n");
						}
					}
				}
			}
		}
	}
}

int ValidIP(char *IP)
{
	int len = strlen(IP);
	if (len > 16)
		return 1;
	int i, numofDig = 0, dots = 0;
	for (i = 0; i < len; i++)
	{
		if (IP[i] == '.')
		{
			dots++;
			if (dots > 3 || numofDig < 1 || numofDig > 3)
			{
				return 1;
			}
			numofDig = 0;
		}
		else if (isdigit(IP[i]) == 0)
		{
			return 1;
		}
		else
		{
			numofDig++;
		}
	}
	if (dots < 3)
	{
		return 1;
	}
	return 0;
}