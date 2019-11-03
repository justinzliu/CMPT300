#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */
#include <unistd.h>
#include <sys/time.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <time.h>
#include <string.h>
 
//compile with: gcc -Wall -o x Q1.c -lrt -lpthread

int timeChange( const struct timeval startTime ){
    struct timeval nowTime;
    long int elapsed;
    int elapsedTime;
    gettimeofday(&nowTime, NULL);
    elapsed = (nowTime.tv_sec - startTime.tv_sec) * 1000000
            + (nowTime.tv_usec - startTime.tv_usec);
    elapsedTime = elapsed / 1000;
    return elapsedTime;
}

int uniformDistribution(int minvalue, int maxvalue) {
	long range = maxvalue - minvalue + 1;
	//time may not be a big enough difference for srand to generate a random time, so use address of a local variable (always changing)
	int random = rand();
	int randomDistributed = (random%range) + minvalue;
	return randomDistributed;
}

void* create_sharedMemory (size_t size) {
	//only allow reads or writes
	int prot = PROT_READ | PROT_WRITE;
	//memory will be shared by all processes, but processes cannot access it's memory address
	int vis = MAP_ANONYMOUS | MAP_SHARED;
	return mmap(NULL,size,prot,vis,0,0);
}

int sem_waitCHECKED (sem_t* sem) {
	if (sem_wait(sem) < 0) {
		perror("[sem_wait] FAILED\n");
		return -1;
	}
	return 0;
}

int sem_postCHECKED (sem_t* sem) {
	if (sem_post(sem) < 0) {
		perror("[sem_post] FAILED\n");
		return -1;
	}
	return 0;
}

int sem_closeCHECKED (sem_t* sem) {
	if (sem_close(sem) < 0) {
		perror("[sem_close] FAILED\n");
		return -1;
	}
	return 0;
}

int sem_unlinkCHECKED (const char* name) {
	if (sem_unlink(name) < 0) {
		perror("[sem_unlink] FAILED\n");
		return -1;
	}
	return 0;
}

int timeChange( const struct timeval startTime );
int uniformDistribution(int minvalue, int maxvalue);
void* create_sharedMemory (size_t size);
int sem_waitCHECKED (sem_t* sem);
int sem_postCHECKED (sem_t* sem);
int sem_closeCHECKED (sem_t* sem);
int sem_unlinkCHECKED (const char* name);

int main() {
	//main spawns all processes
	//Semaphores
		//mutex protection for access to shared memory entires
	sem_t* m_sharedMemory;
	const char* m_name0 = "/sharedMemory";
	if ((m_sharedMemory = sem_open(m_name0,O_CREAT,0600,1)) == SEM_FAILED) {
		printf("MAIN PROCESS: ERROR OPENING %s\n\n",m_name0);
		return 1;
	}
		//semophore queue for thieves until Smaug allows them to enter
	sem_t* s_thiefQ;
	const char* s_name0 = "/queue_thief";
	if ((s_thiefQ = sem_open(s_name0,O_CREAT,0600,0)) == SEM_FAILED) {
		printf("MAIN PROCESS: ERROR OPENING %s\n\n",s_name0);
		return 1;
	}
		//semophore queue for hunters until Smaug allows them to enter
	sem_t* s_hunterQ;
	const char* s_name1 = "/queue_hunter";
	if ((s_hunterQ = sem_open(s_name1,O_CREAT,0600,0)) == SEM_FAILED) {
		printf("MAIN PROCESS: ERROR OPENING %s\n\n",s_name1);
		return 1;
	}
		//semophore queue for Smaug while in combat with thief or hunter
	sem_t* s_smaugQ;
	const char* s_name2 = "/queue_smaug";
	if ((s_smaugQ = sem_open(s_name2,O_CREAT,0600,0)) == SEM_FAILED) {
		printf("MAIN PROCESS: ERROR OPENING %s\n\n",s_name2);
		return 1;
	}
		//semophore queue for cave entrance
	sem_t* s_caveQ;
	const char* s_name3 = "/queue_cave";
	if ((s_caveQ = sem_open(s_name3,O_CREAT,0600,0)) == SEM_FAILED) {
		printf("MAIN PROCESS: ERROR OPENING %s\n\n",s_name3);
		return 1;
	}
	//semophore queue for Smaug to pay/receive reward
	sem_t* s_treasureQ;
	const char* s_name4 = "/queue_treasure";
	if ((s_treasureQ = sem_open(s_name4,O_CREAT,0600,0)) == SEM_FAILED) {
		printf("MAIN PROCESS: ERROR OPENING %s\n\n",s_name4);
		return 1;
	}
	//semophore queue for Smaug to sleep
	sem_t* s_sleepQ;
	const char* s_name5 = "/queue_sleep";
	if ((s_sleepQ = sem_open(s_name5,O_CREAT,0600,0)) == SEM_FAILED) {
		printf("MAIN PROCESS: ERROR OPENING %s\n\n",s_name5);
		return 1;
	}
	//semophore queue to tell Smaug somone in valley
	sem_t* s_inValleyQ;
	const char* s_name6 = "/queue_inValley";
	if ((s_inValleyQ = sem_open(s_name6,O_CREAT,0600,0)) == SEM_FAILED) {
		printf("MAIN PROCESS: ERROR OPENING %s\n\n",s_name6);
		return 1;
	}
	sem_t* s_smaugSmellsQ;
	const char* s_name7 = "/queue_smaugSmells";
	if ((s_smaugSmellsQ = sem_open(s_name7,O_CREAT,0600,0)) == SEM_FAILED) {
		printf("MAIN PROCESS: ERROR OPENING %s\n\n",s_name7);
		return 1;
	}

    int size = 20;
    int pid_index;
    pid_t* pid = (pid_t*)malloc(size*sizeof(pid_t));
    if (pid == NULL) {
        printf("*****ERROR***** failed to allocate memory");
        return 1;
    }
	//Shared Memory
		//sharedMemory[0] = smaug state (either 0 (dead) or 1 (live))
		//sharedMemory[1] = pid_index
		//sharedMemory[2] = treausre count
		//sharedMemory[3] = thieves queued
		//sharedMemory[4] = thieves defeated
		//sharedMemory[5] = hunters queued
		//sharedMemory[6] = hunters defeated
	int* sharedMemory = create_sharedMemory(7*(sizeof(int)));
	int smaug_record[7] = {1,1,30,0,0,0,0};
	memcpy(sharedMemory,smaug_record,sizeof(smaug_record)); 
	int maximumHunterInterval;
	int maximumThiefInterval;
	int winProb;
	int status;
	srand(time(0)); //set seed for randomizers

	printf("Please enter integer values for the follow variables\nEnter the Maximum Hunter Interval (in milliseconds): ");
    scanf("%d",&maximumHunterInterval );
    printf("Enter the Maximum Thief Interval (in milliseconds): ");
    scanf("%d",&maximumThiefInterval );
    printf("Enter the percent probability that the Thief or Hunter will win (0 <= probability <= 100): ");
    scanf("%d",&winProb);
    printf("\n");

    /*
    //DEBUGGING VALUES
	int maximumHunterInterval = 3000;
	int maximumThiefInterval = 3000;
	int winProb = 35;
	*/

	if ((pid[0] = fork()) > 0) {
		//MAIN PROCESS: smaug just created
		pid_t PID;
		int treasure_count;
		int eTime, result; 
		struct timeval startTime;
		gettimeofday(&startTime, NULL);
		int thief_nextArrivalTime = uniformDistribution(0,maximumThiefInterval);
		int hunter_nextArrivalTime = uniformDistribution(0,maximumHunterInterval);
		while (sharedMemory[0]) {
		//generate thieves and hunters
			eTime = timeChange(startTime);
			if (eTime > thief_nextArrivalTime || eTime > hunter_nextArrivalTime) {
				pid_index = sharedMemory[1];
				//dynamically increase array size
				if (pid_index > size-2) {
				    size *= 2;
				    //printf("******DEBUG****** pid array just resized to: %d\n", size);
				    pid = realloc(pid,size*(sizeof(pid_t)));
				    if (pid == NULL) {
				        printf("*****ERROR***** failed to allocate memory\n");
				        return 1;
				    }
				}
				if (eTime > thief_nextArrivalTime) {
					//time for thief to arrive
					sem_waitCHECKED(m_sharedMemory);
					if ((pid[sharedMemory[1]] = fork()) > 0) {
						//MAIN PROCESS: thief just created
						sharedMemory[1]++; //pindex++
					}
					else if (pid[pid_index] < 0) {
						printf("\nMAIN PROCESS: ERROR CREATING PROCESS THIEF\n\n");
					}
					else {
						//THIEF PROCESS
						PID = getpid();
						printf("Thief %d is travelling to the valley\n", PID);
						sem_postCHECKED(s_smaugSmellsQ); //signal Smaug thief has arrived
						sharedMemory[3]++; //thief queue incremented
						sem_postCHECKED(m_sharedMemory);
						sem_waitCHECKED(s_thiefQ); //queued outside valley
						if (!sharedMemory[0]) {
							//Smaug has no money, defeated 3 thieves, defeated 4 treasure hunters, and/or accumulated 80 jewels already
							exit(0);
						}
						printf("Thief %d wandering in the valley\n", PID);
						sem_postCHECKED(s_inValleyQ); //tell Smaug thief in valley
						sem_waitCHECKED(m_sharedMemory);
						sharedMemory[3]--; //thief queue decremented
						sem_postCHECKED(m_sharedMemory);
						sem_waitCHECKED(s_caveQ); //queued at cave entrance
						printf("Thief %d is playing with Smaug\n", PID);
						result = uniformDistribution(1,100);
						//printf("\n********DEBUG******* Smaug rolled %d\n", result);
						if (result > winProb) {
							//Smaug won, Thief lost
							printf("Smaug has defeated a thief\n");
							sem_postCHECKED(s_smaugQ); //Smaug gets result of game
							sem_waitCHECKED(s_treasureQ); //Thief waiting to bribe Smaug
							printf("Thief %d has been defeated and pays the price\n", PID);
							sem_waitCHECKED(m_sharedMemory);
							sharedMemory[2] += 20; //treasure increased
							sharedMemory[4]++; //thieves defeated count increment
							printf("Smaug has added to his treasure, he now has %d jewels\n", sharedMemory[2]);
							sem_postCHECKED(m_sharedMemory);
						}
						else {
							//Thief won, Smaug lost
							printf("Smaug has been defeated by a thief\n");
							sem_postCHECKED(s_smaugQ); //Smaug gets result of game
							sem_waitCHECKED(s_treasureQ); //Thief waiting to get treasure
							printf("Thief %d has won and receives treasure\n", PID);
							sem_waitCHECKED(m_sharedMemory);
							treasure_count = sharedMemory[2];
							treasure_count -= 8; //treasure decrease
							sharedMemory[2] = treasure_count;
							if (treasure_count < 0) {
								treasure_count = 0;
								sharedMemory[2] = 0;
							}
							printf("Smaug has lost some treasure, he now has %d jewels\n", treasure_count);
							sem_postCHECKED(m_sharedMemory);
						}
						printf("Thief %d leaves\n", PID);
						sem_postCHECKED(s_sleepQ); //Smaug knows thief has left
						exit(0); 
					}
				}
				if (eTime > hunter_nextArrivalTime) {
					//time for treasure hunter to arrive
					sem_waitCHECKED(m_sharedMemory);
					if ((pid[sharedMemory[1]] = fork()) > 0) {
						//MAIN PROCESS: treasure hunter just created
						sharedMemory[1]++; //pindex++
					}
					else if (pid[pid_index] < 0) {
						printf("\nMAIN PROCESS: ERROR CREATING PROCESS TREASURE HUNTER\n\n");
					}
					else {
						//TREASURE HUNTER PROCESS
						PID = getpid();
						printf("Treasure hunter %d is travelling to the valley\n", PID);
						sem_postCHECKED(s_smaugSmellsQ); //signal Smaug treasure hunter has arrived
						sharedMemory[5]++; //treasure hunter queue incremented
						sem_postCHECKED(m_sharedMemory);
						sem_waitCHECKED(s_hunterQ); //queued outside valley
						if (!sharedMemory[0]) {
							//Smaug has no money, defeated 3 thieves, defeated 4 treasure hunters, and/or accumulated 80 jewels already
							exit(0);
						}
						printf("Treasure hunter %d wandering in the valley\n", PID);
						sem_postCHECKED(s_inValleyQ); //tell Smaug treasure hunter in valley
						sem_waitCHECKED(m_sharedMemory);
						sharedMemory[5]--; //treasure hunter queue decremented
						sem_postCHECKED(m_sharedMemory);
						sem_waitCHECKED(s_caveQ); //queued at cave entrance
						printf("Treasure hunter %d is fighting Smaug\n", PID);
						result = uniformDistribution(1,100);
						//printf("\n********DEBUG******* Smaug rolled %d\n", result);
						if (result > winProb) {
							//Smaug won, treasure hunter lost
							printf("Smaug has defeated a treasure hunter\n");
							sem_postCHECKED(s_smaugQ); //Smaug gets result of battle
							sem_waitCHECKED(s_treasureQ); //treasure hunter waiting to bribe Smaug
							printf("Treasure hunter %d has been defeated and pays the price\n", PID);
							sem_waitCHECKED(m_sharedMemory);
							sharedMemory[2] += 5; //treasure increased
							sharedMemory[6]++; //treasure hunter defeated count increment
							printf("Smaug has added to his treasure, he now has %d jewels\n", sharedMemory[2]);
							sem_postCHECKED(m_sharedMemory);
						}
						else {
							//Treasure hunter won, Smaug lost
							printf("Smaug has been defeated by a treasure hunter\n");
							sem_postCHECKED(s_smaugQ); //Smaug gets result of battle
							sem_waitCHECKED(s_treasureQ); //treasure hunter waiting to get treasure
							printf("Treasure hunter %d has won and receives treasure\n", PID);
							sem_waitCHECKED(m_sharedMemory);
							treasure_count = sharedMemory[2];
							treasure_count -= 10; //treasure decrease
							sharedMemory[2] = treasure_count;
							if (treasure_count < 0) {
								treasure_count = 0;
								sharedMemory[2] = 0;
							}
							printf("Smaug has lost some treasure, he now has %d jewels\n", treasure_count);
							sem_postCHECKED(m_sharedMemory);
						}
						printf("Treasure hunter %d leaves\n", PID); //Smaug knows treasure hunter has left
						sem_postCHECKED(s_sleepQ);
						exit(0); 
					}
				}
				result = uniformDistribution(0,maximumThiefInterval);
				thief_nextArrivalTime = eTime + result;
				result = uniformDistribution(0,maximumHunterInterval);
				hunter_nextArrivalTime = eTime + result;
				//printf("\n********DEBUG******* thief_nextArrivalTime is: %d and hunter_nextArrivalTime is: %d\n", thief_nextArrivalTime, hunter_nextArrivalTime);
			}
		}
	}
	else if (pid[0] < 0) {
		printf("\nMAIN PROCESS: ERROR CREATING PROCESS SMAUG\n\n");
	}
	else {	
		//SMAUG PROCESS	
		while (sharedMemory[0]) {
			sem_waitCHECKED(s_smaugSmellsQ);
			//There is a visitor
			if (sharedMemory[4] < 3 && sharedMemory[6] < 4 && sharedMemory[2] < 80) {
				//Smaug has not yet defeated 3 thieves or 4 treasure hunters or accumlated 80 jewels
				if (sharedMemory[2] > 0) {
					//Smaug still has some treasure
					printf("Smaug has been woken up\n");
					printf("Smaug takes a deep breath\n");
					if (sharedMemory[3] > 0) {
						//Smaug chooses to play with thief
						printf("Smaug smells a thief\n");
						sem_postCHECKED(s_thiefQ);
						sem_waitCHECKED(s_inValleyQ);
						sem_postCHECKED(s_caveQ);
						printf("Smaug is playing with a thief\n");
						sem_waitCHECKED(s_smaugQ);
						printf("Smaug has finished a game\n");
						sem_postCHECKED(s_treasureQ);
						sem_waitCHECKED(s_sleepQ);
					}
					else if (sharedMemory[5] > 0) {
						//Smaug chooses to play with treasure hunter
						printf("Smaug smells a teasure hunter\n");
						sem_postCHECKED(s_hunterQ);
						sem_waitCHECKED(s_inValleyQ);
						sem_postCHECKED(s_caveQ);
						printf("Smaug is fighting a treasure hunter\n");
						sem_waitCHECKED(s_smaugQ);
						printf("Smaug has finished a battle\n");
						sem_postCHECKED(s_treasureQ);
						sem_waitCHECKED(s_sleepQ);
					}
				printf("Smaug is going to sleep\n\n");
				}
				else {
					//Smaug no longer has treasure
					printf("Smaug has no more treasure\n");
					sharedMemory[0]--;
				}
			}
			else {
				//Smaug has defeated 3 thieves and/or 4 treasure hunters and/or accumlated 80 jewels
				sharedMemory[0]--;
			}
		}
		/*
		printf("********DEBUG*********** smaug_live is: %d\n", sharedMemory[0]);
		printf("********DEBUG*********** pid_index is: %d\n", sharedMemory[1]);
		printf("********DEBUG*********** smaug_treasure is: %d\n", sharedMemory[2]);
		printf("********DEBUG*********** smaug_thievesDefeated: %d\n", sharedMemory[4]);
		printf("********DEBUG*********** smaug_huntersDefeated: %d\n\n", sharedMemory[6]);
		*/
		exit(0);
	}
	sem_waitCHECKED(m_sharedMemory);
	//release all thieves from queue
	for (int i=0; i<sharedMemory[3]; i++) {
		sem_postCHECKED(s_thiefQ);
	}
	//release all treasure hunters from queue
	for (int i=0; i<sharedMemory[5]; i++) {
		sem_postCHECKED(s_hunterQ);
	}
	//wait on all processes to prevent zombies
	printf("\n");
	for (int i=0; i<sharedMemory[1]; i++) {
		if (waitpid(pid[i],&status,0) < 0) {
			printf("\nMAIN PROCESS: ERROR KILLING PROCESS ID: %d\n", pid[i]);
			return 1;
		}
		else {
			printf("MAIN PROCESS: KILLED PROCESS %d\n", pid[i]);
		}
	}
	//sem_close or sum_unlink all semaphores
	if (sem_closeCHECKED(m_sharedMemory) != 0 || sem_unlinkCHECKED(m_name0) != 0) {
		perror("MAIN PROCESS: [sem_close] for m_sharedMemory FAILED\n");
		return 1;
	}
	if (sem_closeCHECKED(s_thiefQ) != 0 || sem_unlinkCHECKED(s_name0) != 0) {
		perror("MAIN PROCESS: [sem_close] for s_thiefQ FAILED\n");
		return 1;
	}
	if (sem_closeCHECKED(s_hunterQ) != 0 || sem_unlinkCHECKED(s_name1) != 0) {
		perror("MAIN PROCESS: [sem_close] for s_hunterQ FAILED\n");
		return 1;
	}
	if (sem_closeCHECKED(s_smaugQ) != 0 || sem_unlinkCHECKED(s_name2) != 0) {
		perror("MAIN PROCESS: [sem_close] for s_smaugQ FAILED\n");
		return 1;
	}
	if (sem_closeCHECKED(s_caveQ) != 0 || sem_unlinkCHECKED(s_name3) != 0) {
		perror("MAIN PROCESS: [sem_close] for s_caveQ FAILED\n");
		return 1;
	}
	if (sem_closeCHECKED(s_treasureQ) != 0 || sem_unlinkCHECKED(s_name4) != 0) {
		perror("MAIN PROCESS: [sem_close] for s_treasureQ FAILED\n");
		return 1;
	}
	if (sem_closeCHECKED(s_sleepQ) != 0 || sem_unlinkCHECKED(s_name5) != 0) {
		perror("MAIN PROCESS: [sem_close] for s_sleepQ FAILED\n");
		return 1;
	}
	if (sem_closeCHECKED(s_inValleyQ) != 0 || sem_unlinkCHECKED(s_name6) != 0) {
		perror("MAIN PROCESS: [sem_close] for s_inValleyQ FAILED\n");
		return 1;
	}
	if (sem_closeCHECKED(s_smaugSmellsQ) != 0 || sem_unlinkCHECKED(s_name7) != 0) {
		perror("MAIN PROCESS: [sem_close] for s_smaugSmellsQ FAILED\n");
		return 1;
	}
	printf("MAIN PROCESS EXITS 0\n");
	return 0;
}





