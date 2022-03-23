/**
 *
 * @author  Dexson Dsouza <dexsonjo@buffalo.edu>
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
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <netdb.h>
#include "../include/global.h"
#include "../include/logger.h"
// #include <arpa/inet.h>

#define BACKLOG 5		// baclokg limit
#define STDIN 0			// take input
#define CMD_LEN 100		// command size
#define BUFFER_SIZE 256 // buffer limit
#define MSG_SIZE 256	// buffer limit

char str_ip[INET_ADDRSTRLEN];
int login_status_client = 0;
int port_no;
typedef struct list_clients
{
	int id;
	char hostname[BUFFER_SIZE];
	char IPaddress[CMD_LEN];
	int port_num;
	int log_status;
	int num_sent;
	int num_recv;
	int is_block;
	char blocked_by[100];
	struct list_clients *next;
} list_clients;

typedef struct client_info
{
	char hostname[BUFFER_SIZE];
	char IPaddress[CMD_LEN];
	int port;
	int is_block;
	struct client_info *next;

} client_info;

/**
 * main function
 *
 * @param  argc Number of arguments
 * @param  argv The argument list
 * @return 0 EXIT_SUCCESS
 */

int startServer(int port_no);
int startClient(int port_no);

int validate_ip(char *ip);
void add_client_list(char buffer[256], client_info **head);
void add_server_list(char hosts[200], int client_listen_port, int s, list_clients **head);
int connect_to_host(char *ip_server, int client_port, int server_port);
void print_client(list_clients **client_list);
int connect_to_host(char *ip_server, int client_port, int server_port);
void send_server_list(int socket, list_clients **list);
void send_msg_to_client(list_clients **list, char ipaddr[256], char message_to_client[256]);
int check_client_list(char *IPaddr, client_info **head);
void sendToIP(char ipaddress[256], list_clients **list, char messageToSend[256]);
int isAlreadyBlocked(char *IP, client_info **head);

int main(int argc, char **argv)
{
	cse4589_init_log(argv[2]);
	fclose(fopen(LOGFILE, "w"));
	/*Start Here*/
	if (argc != 3)
	{
		printf("Only 2 arguments accepted.\n");
		return -1;
	}

	if (strcmp(argv[1], "s") == 0)
	{
		port_no = atoi(argv[2]);
		startServer(port_no);
	}
	else
	{
		port_no = atoi(argv[2]);
		startClient(port_no);
	}

	return 0;
}

int checkinServerList(char *IPaddress, list_clients **head)
{
	list_clients *temp = *head;
	while (temp != NULL)
	{
		if (strcmp(temp->IPaddress, IPaddress) == 0)
		{
			return 1;
		}
		temp = temp->next;
	}
	return 0;
}

int startServer(int port_no)
{
	// printf("Startr\n");
	int server_socket, fd_max, selret, sock_index, fdaccept = 0, caddr_len;
	struct sockaddr_in client_addr, server_addr;
	fd_set master_list, watch_list;
	list_clients *server_list = NULL;
	server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket < 0)
		perror("Cannot create socket");
	// printf("Startr\n");
	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(port_no);
	if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
		perror("Bind failed");
	// printf("Startr\n");
	if (listen(server_socket, BACKLOG) < 0)
		perror("Unable to listen on port");
	// printf("Startr\n");
	/* FD sets Zero   */
	FD_ZERO(&master_list);
	FD_ZERO(&watch_list);

	/* Register the listening socket and STDIN*/
	FD_SET(server_socket, &master_list);
	FD_SET(STDIN, &master_list);
	fd_max = server_socket;
	char *cmd = (char *)malloc(sizeof(char) * CMD_LEN);

	struct sockaddr_in temp_udp;
	int len = sizeof(temp_udp);
	int error_flag = 0;
	int socketfd;
	char ip_str[INET_ADDRSTRLEN];
	socketfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	// printf("Hello I am above!\n");
	if (socketfd < 0)
	{
		printf("Error while creating socket\n");
		exit(-1);
	}

	memset((char *)&temp_udp, 0, sizeof(temp_udp));
	temp_udp.sin_family = AF_INET;
	temp_udp.sin_port = htons(2000);
	inet_pton(AF_INET, "8.8.8.8", &temp_udp.sin_addr);
	// temp_udp.sin_addr.s_addr = "8.8.8.8";

	int flag = connect(socketfd, (struct sockaddr *)&temp_udp, sizeof(temp_udp));
	// printf("%d\n",flag);

	int flag2 = getsockname(socketfd, (struct sockaddr *)&temp_udp, (unsigned int *)&len);
	inet_ntop(AF_INET, &(temp_udp.sin_addr), ip_str, sizeof(temp_udp));

	while (1)
	{
		memcpy(&watch_list, &master_list, sizeof(master_list));
		selret = select(fd_max + 1, &watch_list, NULL, NULL, NULL);
		if (selret < 0)
		{
			printf("select eror\n");
		}

		/* Check if we have sockets/STDIN to process */
		if (selret > 0)
		{
			/* Loop through socket descriptors to check which ones are ready */
			for (sock_index = 0; sock_index <= fd_max; sock_index += 1)
			{

				if (FD_ISSET(sock_index, &watch_list))
				{

					/* Check if new command on STDIN */
					if (sock_index == STDIN)
					{
						char *cmd = (char *)malloc(sizeof(char) * CMD_LEN);

						memset(cmd, '\0', CMD_LEN);
						if (fgets(cmd, CMD_LEN - 1, stdin) == NULL) // Mind the newline character that will be written to cmd
							exit(-1);

						int size_of_cmd = strlen(cmd);
						if (cmd[size_of_cmd - 1] == '\n')
						{
							cmd[size_of_cmd - 1] = '\0';
						}

						char *messageToSend = (char *)malloc(sizeof(char) * MSG_SIZE);
						memset(messageToSend, '\0', MSG_SIZE);
						strncpy(messageToSend, cmd, strlen(cmd));
						printf("message at client side: %s \n", messageToSend);
						if (strlen(cmd) == 0)
						{
							continue;
						}
						int i = 0;
						char *token = strtok(cmd, " ");
						char *argvs[3];

						while (token)
						{
							argvs[i] = malloc(strlen(token) + 1);
							strcpy(argvs[i], token);
							i += 1;
							token = strtok(NULL, " ");
						}
						if (strcmp(argvs[0], "AUTHOR") == 0)
						{
							cse4589_print_and_log("[AUTHOR:SUCCESS]\n");
							cse4589_print_and_log("I, Dexson, have read and understood the course academic integrity policy.\n");
							cse4589_print_and_log("[AUTHOR:END]\n");
						}

						else if (strcmp(argvs[0], "PORT") == 0)
						{
							cse4589_print_and_log("[PORT:SUCCESS]\n");
							cse4589_print_and_log("PORT:%d\n", port_no);
							cse4589_print_and_log("[PORT:END]\n");
						}
						else if (strcmp(argvs[0], "IP") == 0)
						{
							cse4589_print_and_log("[IP:SUCCESS]\n");
							cse4589_print_and_log("IP:%s\n", ip_str);
							cse4589_print_and_log("[IP:END]\n");
						}
						else if (strcmp(argvs[0], "LIST") == 0)
						{
							cse4589_print_and_log("[LIST:SUCCESS]\n");
							print_client(&server_list);
							cse4589_print_and_log("[LIST:END]\n");
						}
						else if (strcmp(argvs[0], "BLOCKED") == 0)
						{
							char *IP = (char *)malloc(sizeof(char) * strlen(argvs[1]));
							strncpy(IP, argvs[1], strlen(argvs[1]));
							// printf("%d \n",validate_ip(checkinServerList(argvs[1], &server_list));
							if (validate_ip(argvs[1]) == 1 && checkinServerList(argvs[1], &server_list) == 1)
							{
								cse4589_print_and_log("[BLOCKED:SUCCESS]\n");
								list_clients *temp = server_list;
								int list_id = 1;
								while (temp != NULL)
								{

									printf("%s ==== %s .... %d ....%d\n",IP,temp->blocked_by,temp->is_block,strncmp(IP, temp->blocked_by, strlen(temp->blocked_by)));
									if (temp->is_block == 1 && strncmp(IP, temp->blocked_by, strlen(temp->blocked_by)) == 0)
									{
										cse4589_print_and_log("%-5d%-35s%-20s%-8d\n", list_id, temp->hostname, temp->IPaddress, temp->port_num);
										list_id++;
									}
									temp = temp->next;
								}

								cse4589_print_and_log("[BLOCKED:END]\n");
							}
							else
							{
								cse4589_print_and_log("[BLOCKED:ERROR]\n");
								cse4589_print_and_log("[BLOCKED:END]\n");
							}
						}

						free(cmd);
					}
					/* Check if new client is requesting connection */
					else if (sock_index == server_socket)
					{
						caddr_len = sizeof(client_addr);
						fdaccept = accept(server_socket, (struct sockaddr *)&client_addr, &caddr_len);
						// printf("FD Accept--->%d",fdaccept);
						if (fdaccept < 0)
							perror("Accept failed.");
						char hosts[1024];
						char serv[20];

						getnameinfo(&client_addr, sizeof client_addr, hosts, sizeof hosts, serv, sizeof serv, 0);
						char *buffer = (char *)malloc(sizeof(char) * BUFFER_SIZE);
						memset(buffer, '\0', BUFFER_SIZE);

						if (recv(fdaccept, buffer, BUFFER_SIZE, 0) >= 0)
						{
							// printf("Client on listen port %s \n", buffer);
						}

						int client_listening_port = atoi(buffer);
						add_server_list(hosts, client_listening_port, fdaccept, &server_list);
						send_server_list(fdaccept, &server_list);

						/* Add to watched socket list */
						FD_SET(fdaccept, &master_list);
						if (fdaccept > fd_max)
							fd_max = fdaccept;
					}
					/* Read from existing clients */
					else
					{
						/* Initialize buffer to receieve response */
						char *buffer = (char *)malloc(sizeof(char) * BUFFER_SIZE);
						memset(buffer, '\0', BUFFER_SIZE);

						if (recv(sock_index, buffer, BUFFER_SIZE, 0) <= 0)
						{
							close(sock_index);
							printf("Remote Host terminated connection!\n");

							/* Remove from watched list */
							FD_CLR(sock_index, &master_list);
						}
						else
						{
							// Process incoming data from existing clients here ...

							// printf("Client se nt me: %s\n", buffer);

							int is_client_blocked = 0;
							char ip_addr_client[50];
							memset(ip_addr_client, 0, sizeof ip_addr_client);
							list_clients *temp = server_list;

							while (temp != NULL)
							{
								if (temp->id == sock_index)
								{
									strncpy(ip_addr_client, temp->IPaddress, sizeof(temp->IPaddress));
									is_client_blocked = temp->is_block;
								}
								temp = temp->next;
							}
							free(temp);

							char *msg_to_send = (char *)calloc(strlen(buffer) + 1, sizeof(char));
							strncpy(msg_to_send, buffer, strlen(buffer));
							printf("Client se nt me: %s\n", msg_to_send);
							int i = 0;
							char *token = strtok(msg_to_send, " ");
							char *argv_client[500];

							while (token)
							{
								argv_client[i] = malloc(strlen(token) + 1);
								strcpy(argv_client[i], token);
								i += 1;
								token = strtok(NULL, " ");
							}

							if (strcmp(argv_client[0], "REFRESH") == 0)
							{
								// printf("Heelo from refresh in server:\n");
								list_clients *temp = server_list;
								while (temp != NULL)
								{
									// printf("Hello inside while loop\n");
									if (temp->id == sock_index)
									{
										send_server_list(sock_index, &server_list);
										break;
									}
									temp = temp->next;
								}
							}

							if (strcmp(argv_client[0], "SEND") == 0)
							{
								char message_from_client[256];
								memset(message_from_client, 0, sizeof message_from_client);

								if (is_client_blocked == 0)
								{
									cse4589_print_and_log("[RELAYED:SUCCESS]\n");
									// printf("Message coming from client: %s\n", message_from_client);
									char *first_token, *second_token, *remaining_part, *context;
									char delimiter[] = " ";
									// int inputLength = strlen(input);
									// char *inputCopy = (char*) calloc(inputLength + 1, sizeof(char));
									// strncpy(inputCopy, input, inputLength);
									first_token = strtok_r(buffer, delimiter, &context);
									second_token = strtok_r(NULL, delimiter, &context);
									remaining_part = context;
									strcat(message_from_client, "RCV ");
									strcat(message_from_client, ip_addr_client);
									strcat(message_from_client, " ");
									strcat(message_from_client, remaining_part);
									cse4589_print_and_log("msg from:%s, to:%s\n[msg]:%s\n", ip_addr_client, second_token, remaining_part);
									sendToIP(second_token, &server_list, message_from_client);
									cse4589_print_and_log("[RELAYED:END]\n");
								}
							}

							else if (strcmp(argv_client[0], "BROADCAST") == 0)
							{
								// get the message and sender
								char forwardMessage[256];
								memset(forwardMessage, 0, sizeof forwardMessage);
								int reciever = sock_index;
								char delimiter[] = " ";
								char *firstWord, *remainder, *context, *remaining_part;
								firstWord = strtok_r(buffer, delimiter, &context);
								remainder = context;

								strcat(forwardMessage, "DEXS ");
								strcat(forwardMessage, ip_addr_client);
								strcat(forwardMessage, " ");
								strcat(forwardMessage, remainder);
								// printf("Message for broadcast  =  : %s  ,from %s \n", remainder,ip_addr_client);
								for (int i = 0; i <= fd_max; i++)
								{
									if (FD_ISSET(i, &master_list) && i != 0 && i != server_socket && i != reciever)
									{
										if (send(i, forwardMessage, strlen(forwardMessage), 0) == strlen(forwardMessage))
										{
											cse4589_print_and_log("[RELAYED:SUCCESS]\n");
											cse4589_print_and_log("msg from:%s, to:%s\n[msg]:%s\n", ip_addr_client, "255.255.255.255", remainder);
											cse4589_print_and_log("[RELAYED:END]\n");
										}
										fflush(stdout);
									}
								}
							}
							else if (strcmp(argv_client[0], "BLOCK") == 0)
							{

								char delimiter[] = " ";
								char *firstWord, *context;
								firstWord = strtok_r(buffer, delimiter, &context);
								char *remainder;
								remainder = context;
								list_clients *temp1 = server_list;
								printf("%s wants to block %s\n", ip_addr_client, remainder);

								while (temp1 != NULL)
								{
									if (strcmp(temp1->IPaddress, remainder) == 0)
									{
										printf("%s had blocked %s\n", ip_addr_client, remainder);
										temp1->is_block = 1;
										// this blocked by client
										strcpy(temp1->blocked_by, ip_addr_client);
									}
									temp1 = temp1->next;
								}
							}
							else if (strcmp(argv_client[0], "UNBLOCK") == 0)
							{
								char delimiter[] = " ";
								char *firstWord, *remainder, *context;
								firstWord = strtok_r(buffer, delimiter, &context);
								remainder = context;
								list_clients *temp = server_list;
								printf("%s wants to unblock %s\n", ip_addr_client, remainder);
								while (temp != NULL)
								{
									if (strcmp(temp->IPaddress, remainder) == 0)
									{
										printf("%s had unblocked %s\n", ip_addr_client, remainder);
										temp->is_block = 0;
										memset(temp->blocked_by, '\0', strlen(temp->blocked_by));
										break;
									}
									temp = temp->next;
								}
							}
						}

						free(buffer);
					}
				}
			}
		}
	}
}

void print_client(list_clients **client_list)
{
	list_clients *temp = *client_list;

	int swapped;

	// struct list_clients *ptr1 = client_list;
	// struct list_clients *lptr = NULL;

	// /* Checking for empty list */
	// if (ptr1 == NULL)
	// 	return;

	// do
	// {
	// 	swapped = 0;
	// 	ptr1 = client_list;

	// 	while (ptr1->next != lptr)
	// 	{
	// 		if (ptr1->port_num > ptr1->next->port_num)
	// 		{
	// 			swap(ptr1, ptr1->next);
	// 			swapped = 1;
	// 		}
	// 		ptr1 = ptr1->next;
	// 	}
	// 	lptr = ptr1;
	// } while (swapped);

	// for (int i = 0; i < size; i++)
	// {
	// 	for (int j = 0; j < size - 1; j++)
	// 	{
	// 		if (arr[j]->port_num > arr[j + 1]->port_num)
	// 		{
	// 			list_clients *t = arr[j];
	// 			arr[j] = arr[j + 1];
	// 			arr[j + 1] = t;
	// 		}
	// 	}
	// }

	// int index = 0;
	// while (index < size)
	// {
	// 	cse4589_print_and_log("%-5d%-35s%-20s%-8d\n", index + 1, arr[index]->hostname, arr[index]->IPaddress, arr[index]->port_num);
	// 	index++;
	// }

	int id = 1;
	while (temp != NULL)
	{
		// cse4589_print_and_log("Hi");
		cse4589_print_and_log("%-5d%-35s%-20s%-8d\n", id, temp->hostname, temp->IPaddress, temp->port_num);
		id = id + 1;
		temp = temp->next;
	}
}

int validate_ip(char *IP)
{ // check whether the IP is valid or not
	int len = strlen(IP);
	if (len > 16)
		return 0;
	int i, numofDig = 0, dots = 0;
	for (i = 0; i < len; i++)
	{
		if (IP[i] == '.')
		{
			dots++;
			if (dots > 3 || numofDig < 1 || numofDig > 3)
			{
				return 0;
			}
			numofDig = 0;
		}
		else if (isdigit(IP[i]) == 0)
		{
			return 0;
		}
		else
		{
			numofDig++;
		}
	}
	if (dots < 3)
	{
		return 0;
	}
	return 1;
}

int connect_to_host(char *ip_server, int client_port, int server_port)
{
	int fdsocket, len;
	struct sockaddr_in remote_server_addr;

	fdsocket = socket(AF_INET, SOCK_STREAM, 0);
	if (fdsocket < 0)
		perror("Failed to create socket");

	bzero(&remote_server_addr, sizeof(remote_server_addr));
	remote_server_addr.sin_family = AF_INET;
	inet_pton(AF_INET, ip_server, &remote_server_addr.sin_addr);
	remote_server_addr.sin_port = htons(server_port);

	if (connect(fdsocket, (struct sockaddr *)&remote_server_addr, sizeof(remote_server_addr)) < 0)
		perror("Connect failed");
	char portNo[100];
	memset(portNo, 0, sizeof(portNo));
	sprintf(portNo, "%d", client_port);
	if (send(fdsocket, portNo, strlen(portNo), 0) == strlen(portNo) < 0)
	{
		printf("Done failed\n");
	}
	fflush(stdout);
	// cse4589_print_and_log((char *)"[%s:SUCCESS]\n", "LOGIN");
	// cse4589_print_and_log((char *)"[%s:END]\n", "LOGIN");

	return fdsocket;
}

void add_client_list(char buffer[256], client_info **head)
{
	// printf("Hello from adding client to list");
	char *line;
	char *token;
	char buf[256];
	// printf("%sBuffer in client---->\n",buffer);
	for (line = strtok(buffer, "+"); line != NULL;
		 line = strtok(line + strlen(line) + 1, "+"))
	{
		strncpy(buf, line, sizeof(buf));
		int i = 1;
		client_info *last = *head;
		client_info *new = (client_info *)malloc(sizeof(client_info));

		for (token = strtok(buf, "-"); token != NULL;
			 token = strtok(token + strlen(token) + 1, "-"))
		{
			if (i == 1)
			{
				new->port = atoi(token);
				i += 1;
			}
			else if (i == 2)
			{
				memset(new->IPaddress, 0, 100);
				strncpy(new->IPaddress, token, strlen(token));
				i += 1;
			}
			else if (i == 3)
			{
				memset(new->hostname, 0, 256);
				strncpy(new->hostname, token, strlen(token));
				i += 1;
			}
		}
		new->is_block = 0;
		// printf("\nPort is %d\n", new->port);
		// printf("\nIP address %s\n", new->IPaddress);
		// printf("\nHost Is %s\n", new->hostname);
		new->next = NULL;
		if (*head == NULL)
		{
			*head = new;
		}
		else
		{
			while (last->next != NULL)
			{
				last = last->next;
			}
			last->next = new;
		}

		// if(*head==NULL){

		// *head = new;
		// new->next = NULL;
		// printf("First insertion\n");
		// }
		// else{
		// 	// printf("oye here!")
		// 	printf("Second insertions\n");
		// 	new->next = *head;
		// 	*head = new;
		// }
	}
	// printf("Client added succesfully-----\n");
}

void add_server_list(char hosts[200], int client_listen_port, int s, list_clients **head)
{
	// printf("Helllo from adding to server");
	// printf("Hosts ---> %s",hosts);
	socklen_t len;
	struct sockaddr_storage sock_addr;
	char ipstr[INET6_ADDRSTRLEN];
	int port;

	len = sizeof sock_addr;
	getpeername(s, (struct sockaddr *)&sock_addr, &len);
	if (sock_addr.ss_family == AF_INET)
	{
		struct sockaddr_in *s = (struct sockaddr_in *)&sock_addr;
		port = ntohs(s->sin_port);
		inet_ntop(AF_INET, &s->sin_addr, ipstr, sizeof ipstr);
	}

	list_clients *new_node = (list_clients *)malloc(sizeof(list_clients));
	new_node->id = s;
	strncpy(new_node->IPaddress, ipstr, strlen(ipstr));
	strncpy(new_node->hostname, hosts, strlen(hosts));
	new_node->port_num = client_listen_port;
	new_node->is_block = 0;
	new_node->log_status = 1;
	new_node->next = NULL;
	list_clients *current = *head;

	if (*head == NULL)
	{
		// new node is head
		new_node->next = NULL;
		*head = new_node;
	}
	else if ((*head)->port_num > new_node->port_num)
	{
		// new node is head
		new_node->next = *head;
		*head = new_node;
	}
	else
	{
		// insert in order
		while (current->next != NULL && current->next->port_num < new_node->port_num)
		{
			current = current->next;
		}
		new_node->next = current->next;
		current->next = new_node;
	}
}

void send_server_list(int socket, list_clients **list)
{
	// printf("Hello I am here in send server list");
	list_clients *temp = *list;
	char buffer_to_send[100];
	char list_to_client[1000];

	memset(buffer_to_send, 0, sizeof(buffer_to_send));
	memset(list_to_client, 0, sizeof(list_to_client));
	// printf("%s---->",buffer_to_send);
	while (temp != NULL)
	{

		// printf("The port here  is %d",temp->port_num);

		sprintf(buffer_to_send, "%d-%s-%s+", temp->port_num, temp->IPaddress, temp->hostname);
		strcat(list_to_client, buffer_to_send);
		fflush(stdout);
		temp = temp->next;
	}

	if (send(socket, list_to_client, strlen(list_to_client), 0) == strlen(list_to_client) < 0)
	{
		printf("Send FAilled!!");
	}
}

void send_msg_to_client(list_clients **list, char ipaddr[256], char message_to_client[256])
{
	list_clients *temp = *list;
	while (temp != NULL)
	{
		if (strcmp(temp->IPaddress, ipaddr) == 0)
		{
			if (send(temp->id, message_to_client, strlen(message_to_client), 0) != strlen(message_to_client))
				printf("Sending Failure!\n");
			fflush(stdout);
		}
		temp = temp->next;
	}
}

int check_client_list(char *IPaddr, client_info **head)
{
	client_info *temp = *head;
	while (temp != NULL)
	{
		if (strcmp(temp->IPaddress, IPaddr) == 0)
		{
			return 1;
		}
		temp = temp->next;
	}
	return 0;
}

void sendToIP(char ipaddress[256], list_clients **list, char messageToSend[MSG_SIZE])
{
	list_clients *temp = *list;
	while (temp != NULL)
	{
		if (strcmp(temp->IPaddress, ipaddress) == 0)
		{
			if (send(temp->id, messageToSend, strlen(messageToSend), 0) == strlen(messageToSend))
				printf("Done!\n");
			fflush(stdout);
		}
		temp = temp->next;
	}
}

int isAlreadyBlocked(char *IP, client_info **head)
{
	client_info *temp = *head;
	while (temp != NULL)
	{
		if (strcmp(temp->IPaddress, IP) == 0)
		{
			if (temp->is_block == 1)
			{
				return 1;
			}
		}
		temp = temp->next;
	}
	return 0;
}

int startClient(int port_no)
{
	int server_flag;
	client_info *mylist_clients = NULL;
	int server;
	int fd_max, selret, sock_index, fdaccept = 0, caddr_len;
	fd_set master_list, watch_list;

	FD_ZERO(&master_list);
	FD_ZERO(&watch_list);
	FD_SET(STDIN, &master_list);

	fd_max = STDIN;
	struct sockaddr_in temp_udp;
	int len = sizeof(temp_udp);
	int error_flag = 0;
	int socketfd;
	char ip_str[INET_ADDRSTRLEN];
	socketfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (socketfd < 0)
	{
		printf("Error while creating socket\n");
		exit(-1);
	}

	memset((char *)&temp_udp, 0, sizeof(temp_udp));
	temp_udp.sin_family = AF_INET;
	temp_udp.sin_port = htons(2000);
	inet_pton(AF_INET, "8.8.8.8", &temp_udp.sin_addr);
	int flag = connect(socketfd, (struct sockaddr *)&temp_udp, sizeof(temp_udp));
	int flag2 = getsockname(socketfd, (struct sockaddr *)&temp_udp, (unsigned int *)&len);
	inet_ntop(AF_INET, &(temp_udp.sin_addr), ip_str, sizeof(temp_udp));

	while (1)
	{
		memcpy(&watch_list, &master_list, sizeof(master_list));
		selret = select(fd_max + 1, &watch_list, NULL, NULL, NULL);
		if (selret < 0)
			perror("select failed.");

		if (selret > 0)
		{
			for (sock_index = 0; sock_index <= fd_max; sock_index += 1)
			{

				if (FD_ISSET(sock_index, &watch_list))
				{
					if (sock_index == STDIN)
					{
						char *cmd = (char *)malloc(sizeof(char) * MSG_SIZE);

						memset(cmd, '\0', MSG_SIZE);
						if (fgets(cmd, MSG_SIZE - 1, stdin) == NULL)
							exit(-1);

						int size_of_cmd = strlen(cmd);
						if (cmd[size_of_cmd - 1] == '\n')
						{
							cmd[size_of_cmd - 1] = '\0';
						}
						char *message_to_client = (char *)malloc(sizeof(char) * MSG_SIZE);
						memset(message_to_client, '\0', 256);
						strncpy(message_to_client, cmd, strlen(cmd));
						if (strlen(message_to_client) == 0)
						{
							continue;
						}
						int i = 0;
						char *token = strtok(cmd, " ");
						char *argvs[1000];
						memset(argvs, 0, sizeof(argvs));

						while (token)
						{
							argvs[i] = malloc(strlen(token) + 1);
							strcpy(argvs[i], token);
							i += 1;
							token = strtok(NULL, " ");
						}
						// int i =0;
						// char *token = strtok(cmd, " ");
						// char *argvs[1000];

						// while(token!=NULL){
						// 	argvs[i++] = token;
						// 	token = strtok(NULL, " ");
						// }

						// printf("First arguement %d",*argvs[0]);

						// Process PA1 commands here ...
						if (strcmp(argvs[0], "AUTHOR") == 0)
						{
							cse4589_print_and_log("[AUTHOR:SUCCESS]\n");
							cse4589_print_and_log("I, Dexson, have read and understood the course academic integrity policy.\n");
							cse4589_print_and_log("[AUTHOR:END]\n");
						}

						else if (strcmp(argvs[0], "PORT") == 0)
						{
							cse4589_print_and_log("[PORT:SUCCESS]\n", "PORT");
							cse4589_print_and_log("PORT:%d\n", port_no);
							cse4589_print_and_log("[PORT:END]\n", "PORT");
						}
						else if (strcmp(argvs[0], "IP") == 0)
						{
							cse4589_print_and_log("[IP:SUCCESS]\n", "IP");
							cse4589_print_and_log("IP:%s\n", ip_str);
							cse4589_print_and_log("[IP:END]\n", "IP");
						}
						else if (strcmp(argvs[0], "LIST") == 0)
						{
							cse4589_print_and_log("[LIST:SUCCESS]\n");
							client_info *temp = mylist_clients;
							int client_id = 1;
							while (temp != NULL)
							{
								cse4589_print_and_log("%-5d%-35s%-20s%-8d\n", client_id, temp->hostname, temp->IPaddress, temp->port);
								temp = temp->next;
								client_id = client_id + 1;
							}
							cse4589_print_and_log("[LIST:END]\n");
						}
						else if (strcmp(argvs[0], "LOGIN") == 0)
						{
							cse4589_print_and_log("[LOGIN:SUCCESS]\n");
							cse4589_print_and_log("[LOGIN:END]\n");
							char *ip = (char *)malloc(sizeof(char) * strlen(argvs[1]));
							strncpy(ip, argvs[1], strlen(argvs[1]));
							// printf("%s",argvs[2]);
							if (validate_ip(ip))
							{

								// printf("Helllo inside LOGIN ------\n");
								server = connect_to_host(argvs[1], port_no, atoi(argvs[2]));
								char *buffer = (char *)malloc(sizeof(char) * BUFFER_SIZE);
								memset(buffer, '\0', BUFFER_SIZE);
								mylist_clients == NULL;
								// int tp = recv(server,buffer,BUFFER_SIZE,0);
								// printf("TP---->\n",tp);
								if (recv(server, buffer, BUFFER_SIZE, 0) >= 0)
								{
									// printf("Helllo from here-----%s",buffer);
									// printf("Server responded:\n %s", buffer);
									login_status_client = 1;
									add_client_list(buffer, &mylist_clients);
									fflush(stdout);
								}
								FD_SET(server, &master_list);
								if (server > fd_max)
								{
									fd_max = server;
								}
							}
							else
							{
								cse4589_print_and_log("[LOGIN:ERROR]\n");
								cse4589_print_and_log("[LOGIN:ERROR]\n");
							}
						}
						else if (strcmp(argvs[0], "REFRESH") == 0)
						{

							if (send(server, message_to_client, strlen(message_to_client), 0) != strlen(message_to_client))
							{
								printf("sending list failed for refresh\n");
							}
							char *buffer = (char *)malloc(sizeof(char) * BUFFER_SIZE);
							memset(buffer, '\0', BUFFER_SIZE);

							if (recv(server, buffer, BUFFER_SIZE, 0) >= 0)
							{
								// printf("Got List---->%s",buffer);
								client_info *current = mylist_clients;
								client_info *next;
								// free every thing to remake new list
								while (current != NULL)
								{
									next = current->next;
									free(current);
									current = next;
								}
								mylist_clients = NULL;
								add_client_list(buffer, &mylist_clients);
								fflush(stdout);
							}
						}

						else if (strcmp(argvs[0], "SEND") == 0)
						{
							// get ip
							char *ip = (char *)malloc(sizeof(char) * strlen(argvs[1]));
							strncpy(ip, argvs[1], strlen(argvs[1]));
							// check if ip is valid and client is valied
							if (validate_ip(ip) && check_client_list(ip, &mylist_clients))
							{
								if (send(server, message_to_client, strlen(message_to_client), 0) < strlen(message_to_client))
								{
									printf("Send Failed\n");
								}
								cse4589_print_and_log("[SEND:SUCCESS]\n");
								cse4589_print_and_log("[SEND:END]\n");
							}
							else
							{
								cse4589_print_and_log("[SEND:ERROR]\n");
								cse4589_print_and_log("[SEND:END]\n");
							}
							fflush(stdout);
						}
						else if (strcmp(argvs[0], "BROADCAST") == 0)
						{
							// printf("msg for Boardcast %s", message_to_client);
							if (send(server, message_to_client, strlen(message_to_client), 0) < strlen(message_to_client))
							{
								printf("Send failed!\n");
							}
							fflush(stdout);
						}
						else if (strcmp(argvs[0], "BLOCK") == 0)
						{
							char *IP = (char *)malloc(sizeof(char) * strlen(argvs[1]));
							strncpy(IP, argvs[1], strlen(argvs[1]));

							if (validate_ip(IP) && isAlreadyBlocked(IP, &mylist_clients) == 0 && check_client_list(IP, &mylist_clients))
							{
								if (send(server, message_to_client, strlen(message_to_client), 0) != strlen(message_to_client))
								{
									printf("Send failed!\n");
								}

								client_info *temp = mylist_clients;
								while (temp != NULL)
								{

									if (strcmp(argvs[1], temp->IPaddress) == 0)
									{
										// printf(" set done\n");
										temp->is_block = 1;
									}
									temp = temp->next;
								}
								cse4589_print_and_log("[BLOCK:SUCCESS]\n");
								cse4589_print_and_log("[BLOCK:END]\n");
							}
							else
							{
								cse4589_print_and_log("[BLOCK:ERROR]\n");
								cse4589_print_and_log("[BLOCK:END]\n");
							}
						}
						else if (strcmp(argvs[0], "UNBLOCK") == 0)
						{
							char *IP = (char *)malloc(sizeof(char) * strlen(argvs[1]));
							strncpy(IP, argvs[1], strlen(argvs[1]));
							// printf("%s+++%s\n", IP, argvs[1]);
							// if (check_client_list(IP, &mylist_clients) == 0)
							// {
							// 	printf("not exists\n");
							// }
							// else
							// {
							// 	printf(" exists\n");
							// }

							// if (isAlreadyBlocked(IP, &mylist_clients) == 0)
							// {
							// 	printf("not blocked \n");
							// }
							// else
							// {
							// 	printf("blocked \n");
							// }
							// printf("%s+++%s\n", IP, argvs[1]);
							if (validate_ip(IP) && check_client_list(IP, &mylist_clients) == 1 && (isAlreadyBlocked(IP, &mylist_clients) == 1))
							{
								if (send(server, message_to_client, strlen(message_to_client), 0) != strlen(message_to_client))
								{
									printf("Send failed!\n");
								}

								client_info *temp = mylist_clients;
								while (temp != NULL)
								{
									if (strcmp(argvs[1], temp->IPaddress) == 0)
									{
										temp->is_block = 0;
									}
									temp = temp->next;
								}
								cse4589_print_and_log("[UNBLOCK:SUCCESS]\n");
								cse4589_print_and_log("[UNBLOCK:END]\n");
							}
							else
							{
								cse4589_print_and_log("[UNBLOCK:ERROR]\n");
								cse4589_print_and_log("[UNBLOCK:END]\n");
							}
						}
					}
					else
					{
						/* Check for new comnnections*/
						/* Initialize buffer to receieve response */
						char *buffer = (char *)malloc(sizeof(char) * BUFFER_SIZE);
						memset(buffer, '\0', BUFFER_SIZE);

						if (recv(sock_index, buffer, BUFFER_SIZE, 0) <= 0)
						{
							close(sock_index);
							// remove from watching list
							FD_CLR(sock_index, &master_list);
						}
						else
						{
							char *context;
							char delimiter[] = " ";
							// int inputLength = strlen(input);
							// char *inputCopy = (char*) calloc(inputLength + 1, sizeof(char));
							// strncpy(inputCopy, input, inputLength);
							char *first_token;
							first_token = strtok_r(buffer, delimiter, &context);
							char *second_token;
							second_token = strtok_r(NULL, delimiter, &context);
							char *remaining_part;
							remaining_part = context;
							cse4589_print_and_log("[RECEIVED:SUCCESS]\n");
							cse4589_print_and_log("msg from:%s\n[msg]:%s\n", second_token, remaining_part);
							cse4589_print_and_log("[RECEIVED:END]\n");
						}

						free(buffer);
					}
				}
			}
		}
	}
}
