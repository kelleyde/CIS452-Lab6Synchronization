#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/sem.h>
#define SIZE 16

int main(int argc, char *argv[])
{
    // create and initialize semaphore
    int semid = semget(IPC_PRIVATE, 1, 0600);
    semctl(semid, 0, SETVAL, 1);
    
    // Sembuf struct
    struct sembuf lock = {
        0,
        -1,
        SEM_UNDO
    };

    struct sembuf unlock = {
        0,
        1,
        SEM_UNDO
    };

    int status;
    long int i, loop, temp, *sharedMemoryPointer;
    int sharedMemoryID;
    pid_t pid;
    loop = atoi(argv[1]);
    sharedMemoryID = shmget(IPC_PRIVATE, SIZE, IPC_CREAT|S_IRUSR|S_IWUSR);
    if(sharedMemoryID < 0) {
        perror ("Unable to obtain shared memory\n");
        exit (1);
    }
    sharedMemoryPointer = shmat(sharedMemoryID, 0, 0);
    if(sharedMemoryPointer == (void*) -1) {
        perror ("Unable to attach\n");
        exit (1);
    }
    sharedMemoryPointer[0] = 0;
    sharedMemoryPointer[1] = 1;
    pid = fork();
    if(pid < 0){
        printf("Fork failed\n");
    }
    if(pid == 0) { // Child
        // Lock when child is processing
        semop(semid, &lock, 1);
        for(i=0; i<loop; i++) {
        // swap the contents of sharedMemoryPointer[0] andsharedMemoryPointer[1];
            temp = sharedMemoryPointer[0];
            sharedMemoryPointer[0] = sharedMemoryPointer[1];
            sharedMemoryPointer[1] = temp;
        }
        // Unlock when child is done
        semop(semid, &unlock, 1);

        if(shmdt(sharedMemoryPointer) < 0) {
            perror ("Unable to detach\n");
            exit (1);
        }
        exit(0);
    }
    else{
        // Lock when parent is processing
        semop(semid, &lock, 1);
        for(i=0; i<loop; i++) {
        // swap the contents of sharedMemoryPointer[1] and sharedMemoryPointer[0]
            temp = sharedMemoryPointer[0];
            sharedMemoryPointer[0] = sharedMemoryPointer[1];
            sharedMemoryPointer[1] = temp;
        }
        // Unlock when parent is done
        semop(semid, &unlock, 1);
        wait(&status);
        printf("Values: %li\t%li\n", sharedMemoryPointer[0],
        sharedMemoryPointer[1]);
        if(shmdt(sharedMemoryPointer) < 0) {
            perror ("Unable to detach\n");
            exit (1);
        }
        if(shmctl(sharedMemoryID, IPC_RMID, 0) < 0) {
            perror ("Unable to deallocate\n");
            exit(1);
        }
    }
    return 0;

    // Remove semaphore
    semctl(semid, 0, IPC_RMID);
}
