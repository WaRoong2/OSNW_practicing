#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

union semun
{
	int val;
};

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

	cal_data* sh_cal_data;
	void *shared_memory = NULL;
	union semun sem_union;

	struct sembuf semopen = {0, -1, SEM_UNDO};
	struct sembuf semclose = {0, 1, SEM_UNDO};

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

	sh_cal_data = (cal_data *)shared_memory;
	memset(sh_cal_data, 0x00, sizeof(int));
	sh_cal_data->write_done = -1;
	sem_union.val = 1;
	if ( -1 == semctl( semid, 0, SETVAL, sem_union))
	{
		return 1;
	}

	while(1)
	{
		if(semop(semid, &semopen, 1) == -1)
		{
			return 1;
		}
		if (sh_cal_data->write_done == -1)
		{
			scanf("%d %d %c", &(sh_cal_data->left_num), &(sh_cal_data->right_num), &(sh_cal_data->op));
			sh_cal_data->write_done *= -1;
		}
		semop(semid, &semclose, 1);
		if(semop(semid, &semopen, 1) == -1)
		{
			return 1;
		}
		for (int i=0; i<sizeof(cal_data); i++)
		{
			printf("%02x ", *((unsigned char*) sh_cal_data+i));
		}
		printf("\n%d %c %d = %d\n", sh_cal_data->left_num, sh_cal_data->op, sh_cal_data->right_num, sh_cal_data->result);
		semop(semid, &semclose, 1);
	}
	return 1;
}

