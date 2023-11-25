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

union semun
{
        int val;
};

int main(int argc, char **argv)
{
    	time_t rawtime;
    	struct tm *timeinfo;
        
	struct sockaddr_in client_addr, sock_addr;
        int listen_sockfd, client_sockfd;
        int addr_len;
 	pid_t pid;

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
		pid = fork();
		if (pid == 0)
		{
			printf("New Client Connect : %s\n", inet_ntoa(client_addr.sin_addr));
			int shmid;
        		int semid;
			void *shared_memory = NULL;
			struct sembuf semopen = {0, -1, SEM_UNDO};
        		struct sembuf semclose = {0, 1, SEM_UNDO};
			
			int end_connection = 0;
			
			pid = fork();
			if (pid == 0) //producer
			{
        			int left_num, right_num, cal_result;
				cal_data* sh_cal_data;
				short int cal_error;
				char string_input[32];

				union semun sem_union;
				
				shmid = shmget((key_t)1234, sizeof(cal_data), 0666|IPC_CREAT);
	      			if (shmid == -1)
        			{
                			return 1;
        			}

        			semid = semget((key_t)3477, 1, IPC_CREAT|0666);
        			if(semid == -1)
        			{
                			return 1;
        			}

        			shared_memory = shmat(shmid, NULL, 0);
        			if (shared_memory == (void *)-1)
        			{
                			return 1;
        			}
				
				sem_union.val = 1;
				if ( -1 == semctl( semid, 0, SETVAL, sem_union))
                                {
                                        return 1;
                                }

				sh_cal_data = (cal_data *)shared_memory;
				while(1)
				{
					memset(sh_cal_data, 0x00, sizeof(*sh_cal_data));
                			read(client_sockfd, sh_cal_data, sizeof(*sh_cal_data));
					if (strcmp(sh_cal_data->string_input, "quit\n")==0)
					{
						break;
					}
					
	        	        	cal_result = 0;
        		        	cal_error = 0;
	
                			left_num = ntohl(sh_cal_data->left_num);
		                	right_num = ntohl(sh_cal_data->right_num);
					printf("\n%d %d %c %s\n", left_num, right_num, sh_cal_data->op, sh_cal_data->string_input);

					switch(sh_cal_data->op)
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
					if(semop(semid, &semopen, 1) == -1)
			                {
			                        perror("semop error : ");
			                }
                			sh_cal_data->result = cal_result;
        	        		sh_cal_data->error = cal_error;
					semop(semid, &semclose, 1);

				}
				return 1;
			}
			else if ( pid > 0 ) //consumer
			{
				cal_data* sh_cal_data;
				
				shmid = shmget((key_t)1234, 24, 0666);
	        		if (shmid == -1)
	        		{
 			         	perror("shmget failed : ");
                			exit(0);
				}

			        semid = semget((key_t)3477, 0, 0666);
			        if(semid == -1)
			        {
			                perror("semget failed : ");
			                return 1;
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
					if (strcmp(sh_cal_data->string_input, "quit\n")==0)
					{
						break;
					}
					if(semop(semid, &semopen, 1) == -1)
			                {
			                        perror("semop error : ");
			                }
					sh_cal_data->result = htonl(sh_cal_data->result);
					sh_cal_data->error = htonl(sh_cal_data->error);

					time(&rawtime);
					timeinfo = localtime(&rawtime);
					strftime(sh_cal_data->string_input, sizeof(sh_cal_data->string_input), "%a %b %d %H:%M:%S %Y", timeinfo);
                			
					write(client_sockfd, sh_cal_data, sizeof(*sh_cal_data));
					semop(semid, &semclose, 1);
					
					sleep(10);
		        	}
				return 2;
			}
		}
		else if( pid > 0 )
			close(client_sockfd);
        }
       	close(listen_sockfd);

        return 0;
}

