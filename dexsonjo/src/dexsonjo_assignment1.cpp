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

struct client
{
	int list_id;
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
int list_id = 1;

struct sockaddr_in my_info;
char IP[256];
char ListenPORT[256];

int PORT;
int C_PORT;

struct client *head_ref = NULL;
struct client *client_ref = NULL;

int createSocket_server();
void bind_server(int socket);
int listen_server(int socket);
void *get_in_addr(struct sockaddr *sa);
void client(int port_no, int listening_fd);
void server(int port_no, int listening_fd);
int ValidIP(char *IP);
void print_list_of_clients();
void print_list_of_clients2();

void to_string(char str[], int num)
{
	int i, rem, len = 0, n;

	n = num;
	while (n != 0)
	{
		len++;
		n /= 10;
	}
	for (i = 0; i < len; i++)
	{
		rem = num % 10;
		num = num / 10;
		str[len - (i + 1)] = rem + '0';
	}
	str[len] = '\0';
}

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
	strcpy(ListenPORT, argv[2]);
	int sockfd = createSocket_server();
	int lid = sockfd;
	char s[INET6_ADDRSTRLEN];
	my_info.sin_family = AF_INET;
	my_info.sin_addr.s_addr = INADDR_ANY;
	my_info.sin_port = htons(PORT);
	bind_server(sockfd);
	listen_server(sockfd);
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
	if (sockfd == 0)
	{
		cse4589_print_and_log("Can't open socket");
		exit(0);
	}

	// cse4589_print_and_log("Create function on server called %d\n", sockfd);
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
		printf("Listen function failed\n");
		exit(EXIT_FAILURE);
	}
	// cse4589_print_and_log("Listen function on server called %d\n", conn);
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
		fd_set master;
		FD_ZERO(&master);
		FD_SET(STDIN, &master);
		FD_SET(lid, &master);
		while (1)
		{
			read_fds = master;
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
						else if ((strcmp(buf, "LIST") == 0))
						{
							print_list_of_clients2();
						}
						else if (strncmp(buf, "LOGIN", 5) == 0)
						{
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

							// printf("---%s ---%s -----%s ---ARGS--\n", arr[0], arr[1], arr[2]);
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
								// printf("---ARGS-2--\n");
							}
							else
							{
								int sockfd;
								char buf[1024];
								struct addrinfo hints, *servinfo;
								char s[INET6_ADDRSTRLEN];
								memset(&hints, 0, sizeof hints);
								hints.ai_family = AF_UNSPEC;
								hints.ai_socktype = SOCK_STREAM;
								getaddrinfo(arr[1], arr[2], &hints, &servinfo);
								// Get the VALID SERVER information
								struct addrinfo *p;
								for (p = servinfo; p != NULL; p = p->ai_next)
								{
									sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
									if (sockfd < 0)
									{
										continue;
									}
									if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1)
									{
										close(sockfd);
										continue;
									}
									// printf("connection success\n");
									break;
								}
								// if not valid info then error
								if (p == NULL)
								{
									cse4589_print_and_log("[LOGIN:ERROR]\n");
									cse4589_print_and_log("[LOGIN:END]\n");
									return;
								}
								strcpy(buf, (char *)"ADDPORT");
								strcat(buf, " ");
								strcat(buf, ListenPORT);
								if (send(sockfd, buf, strlen(buf), 0) < 0)
								{
									cse4589_print_and_log("[LOGIN:ERROR]\n");
									cse4589_print_and_log("[LOGIN:END]\n");
									return;
								}
								FD_SET(sockfd, &master);
								if (sockfd > fdmax)
								{
									fdmax = sockfd;
								}
								// cse4589_print_and_log("Server socket id %d --- %s--\n", sockfd, buf);
								cse4589_print_and_log((char *)"[%s:SUCCESS]\n", "LOGIN");
								cse4589_print_and_log((char *)"[%s:END]\n", "LOGIN");

								// if (connect(my_fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
								// {
								// 	cse4589_print_and_log("[%s:ERROR]\n", "LOGIN");
								// 	cse4589_print_and_log("[%s:END]\n", "LOGIN");
								// 	printf("%d ---%s ---%s -----%s ---ARGS-3--\n", my_fd, arr[0], arr[1], arr[2]);
								// }
								// else
								// {
								// 	char buf[1024];
								// 	strcpy(buf, (char *)"SETCLNTPORT");
								// 	strcat(buf, " ");
								// 	strcat(buf, arr[2]);
								// 	if (send(my_fd, buf, strlen(buf), 0) == -1)
								// 	{
								// 		cse4589_print_and_log("CAnnot send \n");
								// 	}
								// 	cse4589_print_and_log("[%s:SUCCESS]\n", "LOGIN");
								// 	cse4589_print_and_log("[%s:END]\n", "LOGIN");
								// 	printf("---ARGS--4-\n");
								// }
							}
						}
					}

					else
					{
						// printf("\n In client receive! ");
						char *msg = (char *)malloc(sizeof(char) * 500);
						memset(msg, '\0', 500);
						if (i == 0 && i == lid)
							break;
						else
						{
							int nbyte_recvd = recv(i, msg, 500, 0);
							msg[nbyte_recvd] = '\0';

							// printf(" %s %d\n", identify,strlen("users"));
							// cse4589_print_and_log("==== %s==\n", msg);
							if (nbyte_recvd <= 0)
							{
								close(i);
								printf("Remote Host terminated connection!\n");

								/* Remove from watched list */
								FD_CLR(i, &master);
							}
							else
							{
								client_ref = NULL;
								char *input = (char *)malloc(sizeof(char) * 500);
								strcpy(input, msg);
								struct client *temp_c_list = NULL;
								char *identify = (char *)malloc(sizeof(char) * 50);
								identify = strtok(input, "*");
								// printf("\n!!recieved   %s",msg);
								// create client List

								char *string = (char *)malloc(sizeof(char) * 50);
								// char delim = "\n";
								int i = 0;
								string = strtok(msg, "*");
								string = strtok(NULL, "*");

								while (string != NULL)
								{
									i++;

									struct client *node = (struct client *)malloc(sizeof(client));
									node->list_id = atoi(string);
									string = strtok(NULL, "*");
									node->ip = (char *)malloc(sizeof(250));
									node->ip = string;
									string = strtok(NULL, "*");
									node->hostname = (char *)malloc(sizeof(250));
									node->hostname = string;
									string = strtok(NULL, "*");
									node->port_no = atoi(string);
									string = strtok(NULL, "*");
									if (client_ref == NULL)
									{
										client_ref = node;
										node->next = NULL;
									}
									else
									{
										node->next = client_ref;
										client_ref = node;
									}
									// cse4589_print_and_log("%-5d%-35s%-20s%-8d\n", node->list_id, node->hostname, node->ip, node->port_no);
								}
								// cse4589_print_and_log("DONE %d", i);
								// client_ref = temp_c_list;
							}
							// printf("recieved %s\n", msg);
							// 	}
							// 	else
							// 	{
							// 		// char *msg_from = (char *)malloc(sizeof(MSG_LENGTH));
							// 		// char *a = (char *)malloc(sizeof(MSG_LENGTH));
							// 		// char *msg = (char *)malloc(sizeof(MSG_LENGTH));

							// 		// int count = 0;
							// 		// if (recv_buf != NULL)
							// 		// {
							// 		// 	// printf(" recv_buf %s", recv_buf);

							// 		// 	input = strtok(input, " ");
							// 		// 	strcpy(msg_from, input);
							// 		// 	while (input != NULL)
							// 		// 	{
							// 		// 		// printf("\n %s here", input);
							// 		// 		input = strtok(NULL, " ");
							// 		// 		if (input != NULL)
							// 		// 			strcpy(msg, input);
							// 		// 	}

							// 		// 	cse4589_print_and_log("[RECEIVED:SUCCESS]\n");
							// 		// 	cse4589_print_and_log("msg from:%s\n[msg]:%s\n", msg_from, msg);
							// 		// 	cse4589_print_and_log("[RECEIVED:END]\n");
							// 		// }
							// 		// else
							// 		// {
							// 		// 	cse4589_print_and_log("[RECEIVED:ERROR]\n");
							// 		// 	cse4589_print_and_log("msg from:%s\n[msg]:%s\n", msg_from, msg);
							// 		// 	cse4589_print_and_log("[RECEIVED:END]\n");
							// 		// }

							// 		// // strcat(identify,recv_buf);
							// 		// // printf("%s\n" , recv_buf);
							// 	}
						}
						// fflush(stdout);
					}
				}
			}
		}
	}
}

void addIP(struct client *new_node, char *IP)
{
	new_node->ip = (char *)malloc(sizeof((int)strlen(IP)));
	// printf("%s", str);

	strcpy(new_node->ip, IP);
}

void server(int port, int lid)
{

	fd_set master;
	fd_set read_fds;
	int fdmax = lid;
	FD_ZERO(&read_fds);
	FD_SET(STDIN, &read_fds);
	FD_SET(lid, &read_fds);
	char buf[256];
	int newfd; // fd of new incoming request

	struct sockaddr_storage remoteaddr; // client info
	socklen_t addrlen;
	FD_ZERO(&master);
	FD_SET(STDIN, &master);
	FD_SET(lid, &master);
	while (1)
	{
		memcpy(&read_fds, &master, sizeof(master));
		if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1)
		{
			printf("select eror\n");
			exit(4);
		}
		for (int i = 0; i <= fdmax; i++)
		{
			// printf("checking %d \n", fdmax);
			memset((void *)&buf, '\0', sizeof(buf));
			if (FD_ISSET(i, &read_fds))
			{
				if (i == STDIN)
				{
					read(STDIN, buf, 256);
					buf[strlen(buf) - 1] = '\0';
					// printf("stdin tried\n");
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
					else if ((strcmp(buf, "LIST") == 0))
					{
						// printf("priting lIst \n");
						print_list_of_clients();
					}
				}
				else if (i == lid)
				{
					// printf("connect tried\n");
					addrlen = sizeof(remoteaddr);
					newfd = accept(lid, (struct sockaddr *)&remoteaddr, &addrlen);
					if (newfd == -1)
					{
						cse4589_print_and_log("cannot Accept \n");
					}
					else
					{
						FD_SET(newfd, &master);
						if (newfd > fdmax)
						{
							fdmax = newfd;
						}
						struct sockaddr_in *clientadd = get_in_addr((struct sockaddr *)&remoteaddr);
						char remoteIP[INET_ADDRSTRLEN];

						inet_ntop(AF_INET, clientadd, remoteIP, INET_ADDRSTRLEN);
						struct in_addr t_arr;

						inet_pton(AF_INET, remoteIP, &t_arr);
						struct hostent *he = gethostbyaddr(&t_arr, sizeof(t_arr), AF_INET);
						// printf("====here1=====\n"); // new_node->port_no = client_port;
						// char *hostname=he->h_name;
						// gethostname(hostname, 2048);
						// clients.add(remoteIP, (char *)"", newfd, hostname, -1, (char *)"ONLINE");
						// cse4589_print_and_log("connection IP port: %s, %d\n", remoteIP, clientadd->sin_port);

						struct client *new_node = (struct client *)malloc(sizeof(client));

						// printf("====here=====\n"); // new_node->port_no = client_port;

						new_node->ip = (char *)malloc((size_t)1024);
						// printf("%s", str);

						strcpy(new_node->ip, remoteIP);
						// // printf("%s", new_node->ip);
						// strcpy(client_ip[c], str);
						// // printf("\n %s", client_ip[c]);
						new_node->list_id = list_id;
						list_id++;
						new_node->client_fd = newfd;
						u_int16_t port_num = ntohs(clientadd->sin_port);
						// new_node->port_no = malloc(sizeof(port_num));
						new_node->port_no = port_num;
						// printf("====here1=====%d===\n", he->h_length);
						new_node->hostname = (char *)malloc(sizeof((int)strlen(he->h_name)));
						addIP(new_node, remoteIP);
						// printf("====here12====\n");
						strcpy(new_node->hostname, he->h_name);

						// printf("====%s==%s==%d=====\n", new_node->hostname, new_node->ip, port_num);
						new_node->client_status = 1;
						// new_node->next = NULL;
						if (head_ref == NULL)
						{

							head_ref = new_node;
							new_node->next = NULL;
						}
						else
						{
							new_node->next = head_ref;
							head_ref = new_node;
						}
					}
				}
				else
				{
					// printf("Here\n");
					int BUFFER_SIZE = 256;
					if (i == 0)
						break;
					char *buffer = (char *)malloc(sizeof(char) * BUFFER_SIZE);
					memset(buffer, '\0', BUFFER_SIZE);
					int recd_bytes;
					if (recd_bytes = recv(i, buffer, BUFFER_SIZE, 0) <= 0)
					{
						// if(recd_bytes == 0)
						//  printf("Socket %d congested",sock_index);
						// else{
						// printf("No Bytes recieved!\n");
						close(i);
						FD_CLR(i, &master);
						// }
						// close(i);

						/* Remove from watched list */
						// FD_CLR(sock_index, &master_list);
						// printf("Removed sock_index %d", sock_index);
					}
					else
					{
						// Process incoming data from existing clients here ...
						//  sending to everyone

						// strcpy(buffer, strtok(NULL, "\n"));

						// printf("\n Executing Send\n");
						// unsigned int client_port = ntohs(client_addr.sin_port);
						// char str[INET_ADDRSTRLEN];
						// inet_ntop(AF_INET, &(client_addr.sin_addr), str, INET_ADDRSTRLEN);

						// struct client *temp = *head_ref;
						// char *send_to_ip = (char *)malloc(sizeof(char) * INET_ADDRSTRLEN);

						// printf("recieved info \n");
						// printf("%s \n", buffer);
						char *command = (char *)malloc(2000);
						// send_to_ip = strtok(buffer, " ");
						if (strncmp(buffer, "ADDPORT", 7) == 0)
						{
							char *temp;
							char new_port[256];
							temp = strtok(buffer, " ");
							for (int i = 0; i < 2; i++)
							{
								strcpy(new_port, temp);
								temp = strtok(NULL, " ");
							}
							head_ref->port_no = atoi(new_port);

							struct client *tt = head_ref;
							char client_data[300];
							char client_list[300] = "";
							strcat(client_list, "users");
							char str[20] = "";
							while (tt != NULL)
							{
								strcat(client_list, "*");
								to_string(str, tt->list_id);
								strcat(client_list, str);
								strcat(client_list, "*");
								strcat(client_list, tt->ip);
								strcat(client_list, "*");
								strcat(client_list, tt->hostname);
								strcat(client_list, "*");
								to_string(str, tt->port_no);
								strcat(client_list, str);
								tt = tt->next;
							}

							strcpy(client_data, client_list);
							if (send(newfd, client_data, strlen(client_data), 0) != strlen(client_data))
							{
								printf("Failed\n");
							}
							// printf("msgs = %s \n", client_data);
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

void print_list_of_clients()
{
	struct client *temp = head_ref;
	struct client *arr[10];
	int size = 0;
	while (temp != NULL)
	{
		arr[size] = temp;
		size++;
		temp = temp->next;
	}
	for (int i = 0; i < size; i++)
	{
		for (int j = 0; j < size - 1; j++)
		{
			if (arr[j]->port_no > arr[j + 1]->port_no)
			{
				struct client *t = arr[j];
				arr[j] = arr[j + 1];
				arr[j + 1] = t;
			}
		}
	}
	// printf("List ID %d", list_id);
	cse4589_print_and_log("[LIST:SUCCESS]\n");
	for (int i = 0; i < size; i++)
	{
		cse4589_print_and_log("%-5d%-35s%-20s%-8d\n", i + 1, arr[i]->hostname, arr[i]->ip, arr[i]->port_no);
	}

	cse4589_print_and_log("[LIST:END]\n");
}

void print_list_of_clients2()
{
	struct client *temp = client_ref;
	struct client *arr[10];
	int size = 0;
	while (temp != NULL)
	{
		arr[size] = temp;
		size++;
		temp = temp->next;
	}
	for (int i = 0; i < size; i++)
	{
		for (int j = 0; j < size - 1; j++)
		{
			if (arr[j]->port_no > arr[j + 1]->port_no)
			{
				struct client *t = arr[j];
				arr[j] = arr[j + 1];
				arr[j + 1] = t;
			}
		}
	}
	// printf("List ID %d", list_id);
	cse4589_print_and_log("[LIST:SUCCESS]\n");
	for (int i = 0; i < size; i++)
	{
		cse4589_print_and_log("%-5d%-35s%-20s%-8d\n", i + 1, arr[i]->hostname, arr[i]->ip, arr[i]->port_no);
	}

	cse4589_print_and_log("[LIST:END]\n");
}