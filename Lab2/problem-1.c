#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

#define MAX_MOVIES 1682
#define MAX_USERS 943
#define SHM_KEY 0x124

typedef struct
{
    int sumRating[MAX_MOVIES];
    int count[MAX_MOVIES];
} ShareData;

void read_and_process_file(char *fileName, ShareData *data)
{
    FILE *file = fopen(fileName, "r");
    if (file == NULL)
    {
        perror("Error opening file");
        exit(1);
    }

    int userId, movieId, rating, timeStamp;
    while (fscanf(file, "%d\t%d\t%d\t%d", &userId, &movieId, &rating, &timeStamp) != EOF)
    {
        int index = movieId - 1;

        data->sumRating[index] += rating;
        data->count[index]++;
    }

    fclose(file);
}

int main(int argc, char *argv[])
{
    int shmid;

    shmid = shmget(SHM_KEY, sizeof(ShareData), 0666 | IPC_CREAT);
    if (shmid < 0)
    {
        perror("Shared-memory failed");
        exit(1);
    }

    ShareData *data = (ShareData *)shmat(shmid, NULL, 0);
    if (data == (void *)-1)
    {
        perror("shmat failed");
        exit(1);
    }

    memset(data, 0, sizeof(ShareData));

    pid_t pid1 = fork();
    if (pid1 == 0)
    {
        read_and_process_file("movie-100k_1.txt", data);
        exit(0);
    }
    else if (pid1 > 0){
    }
    else{
    	perror("Process fork failed\n");
	exit(1);
    }

    pid_t pid2 = fork();
    if (pid2 == 0)
    {
        read_and_process_file("movie-100k_2.txt", data);
        exit(0);
    }
    else if (pid2 > 0){
    }
    else{
    	perror("Process fork failed\n");
    	exit(1);
    }

    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);

    float avg_ratings[MAX_MOVIES];

    for(int i = 0; i < MAX_MOVIES;i++){
    	if (data->count[i] > 0){
		avg_ratings[i] = (float)data->sumRating[i]/(float)data->count[i];
	}
    }

    for (int i = 0; i < MAX_MOVIES; i++)
    {
          printf("ITEM %d has %.3f rating\n", i, avg_ratings[i]);
    }

    shmdt(data);
    shmctl(shmid, IPC_RMID, NULL);

    return 0;
}
