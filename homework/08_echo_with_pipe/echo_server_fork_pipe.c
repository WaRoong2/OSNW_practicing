#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#define MAXLINE 1024
#define MAXCLIENT 3 //
#define PORTNUM 3600

int main(int argc, char **argv)
{
	int listen_fd;
	int client_fd[MAXCLIENT]; //
	int fd[2]; //
	int client_num = 0; //
	int index = 0;
	pid_t pid;
	socklen_t addrlen;
	int readn;
	char buf[MAXLINE];
	char total_buf[MAXLINE*MAXCLIENT]; //
	char* ptr;
	struct sockaddr_in client_addr, server_addr;
	
	if( pipe(fd) < 0)
	{
		perror("pipe error : ");
		return 1;
	}

	if( (listen_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		return 1;
	}
	memset((void *)&server_addr, 0x00, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(PORTNUM);

	if(bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) ==-1)
	{
		perror("bind error");
		return 1;
	}
	if(listen(listen_fd, 5) == -1)
	{
		perror("listen error");
		return 1;
	}

	signal(SIGCHLD, SIG_IGN);
	memset(total_buf, 0x00, MAXLINE*MAXCLIENT); //
	while(1)
	{
		addrlen = sizeof(client_addr);
		client_fd[client_num] = accept(listen_fd,(struct sockaddr *)&client_addr, &addrlen);
		if(client_fd[client_num] == -1)
		{
			printf("accept error\n");
			break;
		}
		pid = fork();
		index = client_num++;
		if(pid == 0)
		{
			printf("New Client Connected..#%d\n", index + 1);
			close( listen_fd );
			memset(buf, 0x00, MAXLINE);
			read(client_fd[index], buf, MAXLINE);
			if ( strlen(buf) == 0 )
				write(fd[1], "#NO INPUT# ", 11);
			else
			{
				buf[strlen(buf)-1] = ' ';
				write(fd[1], buf, strlen(buf));
			}
			close(client_fd[index]);
			return 0;
		}
		else if( pid > 0 )
		{
			if ( client_num==MAXCLIENT )
			{
				for (int j=0 ; j<client_num ; j++)
				{	
					memset(buf, 0x00, MAXLINE);
					read(fd[0], buf, MAXLINE);
					ptr = strcat(total_buf, buf);
				}
				while ( client_num > 0 )
				{
					write(client_fd[--client_num], total_buf, strlen(total_buf));
					close( client_fd[client_num] );
				}
				printf("Clients Disconnected..\n");
				memset(total_buf, 0x00, MAXLINE*MAXCLIENT); //
			}

		}
	}
	return 0;
}
