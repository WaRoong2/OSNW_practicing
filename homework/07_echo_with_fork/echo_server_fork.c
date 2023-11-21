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
#define PORTNUM 3600

int main(int argc, char **argv)
{
	int listen_fd, client_fd;
	pid_t pid;
	socklen_t addrlen;
	int readn;
	char buf[MAXLINE];
	char input_string[MAXLINE*32];
	char* ptr;
	struct sockaddr_in client_addr, server_addr;

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
	while(1)
	{
		addrlen = sizeof(client_addr);
		client_fd = accept(listen_fd,(struct sockaddr *)&client_addr, &addrlen);
		if(client_fd == -1)
		{
			printf("accept error\n");
			break;
		}
		pid = fork();
		if(pid == 0)
		{
			printf("New Client Connected..\n");
			close( listen_fd );
			memset(input_string, 0x00, MAXLINE*32);
			memset(buf, 0x00, MAXLINE);
			while((readn = read(client_fd, buf, MAXLINE)) > 0)
			{
				if ( strcmp(buf, "quit\n") == 0 ) {
					if ( strlen(input_string) == 0 )
						write(client_fd, "NO INPUT STRING\n", 16);
					else
					{
						input_string[strlen(input_string)-1] = '\n';
						write(client_fd, input_string, strlen(input_string));
					}
				}
				buf[strlen(buf)-1] = ' ';
				ptr = strcat(input_string, buf);
				memset(buf, 0x00, MAXLINE);
			}
			close(client_fd);
			return 0;
		}
		else if( pid > 0 )
			close(client_fd);
	}
	return 0;
}