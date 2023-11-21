#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct _cal_data
{
	int left_num;
	int right_num;
	char op;
	int result;
	short int error;
	int write_done;
} cal_data;

int main(int argc, char **argv)
{
	int shmid;
	int semid;
	int left_num, right_num, cal_result;
	short int cal_error;

	cal_data* sh_cal_data;
	void *shared_memory = NULL;

	struct sembuf semopen = {0, -1, SEM_UNDO};
	struct sembuf semclose = {0, 1, SEM_UNDO};
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
		int local_var=0;
		if(semop(semid, &semopen, 1) == -1)
		{
			perror("semop error : ");
		}
		if (sh_cal_data->write_done == 1)
		{
			for (int i=0; i<sizeof(cal_data); i++)
			{	
				printf("%02x ", *((unsigned char*) sh_cal_data+i));
			}
			left_num = sh_cal_data->left_num;
			right_num = sh_cal_data->right_num;
			switch(sh_cal_data->op)
			{
				case '+':
					cal_result = left_num + right_num;
					break;
				case '-':
					cal_result = left_num - right_num;
					break;
				case 'x':
					cal_result = left_num * right_num;
					break;
				case '/':
					if(right_num == 0)
					{	cal_error=2;
						break;
					}
					cal_result = left_num / right_num;
					break;
				default:
					cal_error = 1;
			}	
			sh_cal_data->result = cal_result;
			
			printf("\n%d %c %d = %d\n", left_num, sh_cal_data->op, right_num, cal_result);
			sh_cal_data->write_done *= -1;
		}
		semop(semid, &semclose, 1);
	}
	return 1;
}

