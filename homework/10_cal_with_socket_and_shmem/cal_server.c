#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/wait.h>

#define PORT 3600

typedef struct _cal_data
{
        int left_num;
        int right_num;
        char op;
        int result;
	char string_input[32];
	short int error;
} cal_data;

int main(int argc, char **argv)
{
    	time_t rawtime;
    	struct tm *timeinfo;
        
	struct sockaddr_in client_addr, sock_addr;
        int listen_sockfd, client_sockfd;
        int addr_len;
	int shm_key=0x1000;
 	pid_t pid1;

        if( (listen_sockfd  = socket(AF_INET, SOCK_STREAM, 0)) == -1 )
        {
               	perror("Error ");
               	return 1;
        }
       	memset((void *)&sock_addr, 0x00, sizeof(sock_addr));
       	sock_addr.sin_family = AF_INET;
       	sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);
       	sock_addr.sin_port = htons(PORT);
       	if( bind(listen_sockfd, (struct sockaddr *)&sock_addr, sizeof(sock_addr)) == -1)
       	{
               	perror("Error ");
               	return 1;
       	}
       	if(listen(listen_sockfd, 5) == -1)
       	{
               	perror("Error ");
               	return 1;
       	}

	for (;;)
	{
        	addr_len = sizeof(client_addr);
        	client_sockfd = accept(listen_sockfd,(struct sockaddr *)&client_addr, &addr_len);
                
		if(client_sockfd == -1)
                {
                        perror("Error ");
                        return 1;
                }
		pid1 = fork();
		shm_key++;
		if (pid1 == 0)
		{
			printf("New Client Connected : %s\n", inet_ntoa(client_addr.sin_addr));
			int shmid;
        		int semid;
			pid_t pid2;

			struct sembuf semopen = {0, -1, SEM_UNDO};
        		struct sembuf semclose = {0, 1, SEM_UNDO};
				
			pid2 = fork();
			if (pid2 == 0) //producer
			{
				int left_num, right_num, cal_result;
				cal_data rdata;
				cal_data* sh_cal_data;
				short int cal_error;
				char string_input[32];

				void *shared_memory = NULL;
				
				shmid = shmget((key_t)shm_key, sizeof(cal_data), 0666|IPC_CREAT);
	      			if (shmid == -1)
        			{
                			return 1;
        			}	

        			shared_memory = shmat(shmid, NULL, 0);
        			if (shared_memory == (void *)-1)
        			{
                			return 1;
        			}
					

				sh_cal_data = (cal_data *)shared_memory;
				memset(sh_cal_data, 0x00, sizeof(cal_data));

				while(1)
				{
					memset(&rdata, 0x00, sizeof(cal_data));
                			read(client_sockfd, &rdata, sizeof(cal_data));
						
	        	        	cal_result = 0;
        		        	cal_error = 0;
	
                			left_num = ntohl(rdata.left_num);
		                	right_num = ntohl(rdata.right_num);
				printf("%d %d %c %s\n", left_num, right_num, rdata.op, rdata.string_input);
					if (strcmp(rdata.string_input, "quit")==0)
					{
						strcpy(sh_cal_data->string_input, "quit");
						return 1;
					}

					switch(rdata.op)
	                		{
	                        		case '+':
                                			cal_result = left_num + right_num;
                                			break;
                        			case '-':
                	                		cal_result = left_num  - right_num;
        	                        		break;
	                        		case 'x':
                                			cal_result = left_num * right_num;
                        	        		break;
                	        		case '/':
        	                        		if(right_num == 0)
	                                		{
                                        			cal_error = 2;
                                        			break;
                                			}
                        	        		cal_result = left_num / right_num;
                	                		break;
		                        	default:
	        	                        	cal_error = 1;

                			}
					sh_cal_data->left_num = htonl(left_num);
					sh_cal_data->right_num = htonl(right_num);
					sh_cal_data->op = rdata.op;
                			sh_cal_data->result = htonl(cal_result);
        	        		sh_cal_data->error = htonl(cal_error);
						
				}
        			
			}
			else if ( pid2 > 0 ) //consumer
			{
				void *shared_memory = NULL;
				cal_data* sh_cal_data;
				usleep(10);
				shmid = shmget((key_t)shm_key, sizeof(cal_data), 0666);
	        		if (shmid == -1)
	        		{
 			         	perror("shmget failed : ");
                			exit(0);
				}

			        shared_memory = shmat(shmid, NULL, 0);
			        if (shared_memory == (void *)-1)
			        {
			                perror("shmat failed : ");
			                exit(0);
			        }

			        sh_cal_data = (cal_data *)shared_memory;
        	        	while(1)
			        {
					if (strcmp(sh_cal_data->string_input, "quit")==0)
					{
						write(client_sockfd, sh_cal_data, sizeof(cal_data));
						memset(sh_cal_data, 0x00, sizeof(cal_data));	
						return 2;
					}
					time(&rawtime);
					timeinfo = localtime(&rawtime);
					strftime(sh_cal_data->string_input, sizeof(sh_cal_data->string_input),
						       	"%a %b %d %H:%M:%S %Y", timeinfo);
					write(client_sockfd, sh_cal_data, sizeof(cal_data));
					
					sleep(7);
		        	}
			}
		}
		else if( pid1 > 0 )
			close(client_sockfd);
        }
       	close(listen_sockfd);

        return 0;
}

