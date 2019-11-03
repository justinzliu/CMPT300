#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

#define RED "\x1B[31m"
#define RESET "\x1B[0m"

int main() {
	printf("enter the seed for the parent process: ");
	int input;
	scanf("%d", &input);
	srand(input);
	int random = (rand()%5)+5;
	//printf("\n***DEBUGGER*** random begins as %d\n\n",random);
	pid_t children[9]; 
	pid_t grandchildren[36];
	for (int i=0;i<36;i++) {
		//initialize array full of 0. 0 used to check if grandchild exists or not
		grandchildren[i] = 0;
	}
	pid_t* gcArray = grandchildren;
	pid_t parentID = getpid();
	pid_t childID = parentID;
	int Status;
	printf("My process ID is %d\n", parentID);
	for (int i=0; i<random; i++) {
		printf("%d is about to create a child\n", parentID);
		if ((children[i]=fork())>0) {
			printf("Parent %d has created a child with process ID %d\n", parentID, children[i]);	
		}
		else {
			//inside child process
			childID = getpid();
			pid_t grandchildID = childID;
			input += i;
			srand(input);
			int randomc = (rand()%3)+1;
			printf("I am a new child, my process ID is %d, my seed id %d\n", childID, input);
			printf("I am child %d, I will have %d children\n", childID, randomc);
			for (int k=0; k<randomc; k++) {
				printf("I am child %d, I am about to create a child\n", childID);
				pid_t stillchild;
				if (stillchild =fork()>0) {
					printf("I am child %d, I just created a child%d\n", childID,grandchildren[(i*3)+k]);
				}
				else {
					//inside grandchild process
					grandchildID = getpid();
					printf("I am grandchild %d, My grandparent is %d, My parent is %d\n", grandchildID, parentID, childID);
					int newrandom = (rand()%10)+5;
					sleep(newrandom);
					printf("I am grandchild %d with parent %d, I am about to terminate\n", grandchildID, childID);
					exit(0);
				}
			}
			if (childID == grandchildID) {
				//child process only, all grandchildren have been created
				printf("I am the child %d, I have %d children\n", childID, randomc);
				printf("I am waiting for my "RED"children "RESET"to terminate\n");
				for (int k; k<randomc; k++) {
					printf("I am child %d. My child %d has been waited\n",childID,grandchildren[(i*3)+k]);
					if (waitpid(grandchildren[(i*3)+k],&Status,0) < 0) {
						printf("\n*******error killing child*********** %d\n\n", children[i]);
						exit(1);
					}
				}
			}
			//break child generating loop for all child processes
			i = random;
		}
	}
	if (parentID == childID) {
		//parent process only, all children have been created
		for (int i=0; i<random; i++) {
			printf("I am the parent, I am waiting for child %d to terminate\n",children[i]);
			if (waitpid(children[i],&Status,0) < 0) {
				printf("\n*******error killing child*********** %d\n\n", children[i]);
				exit(1);
			}
			else {

				printf("I am process %d. My child %d is dead\n", parentID, children[i]);
			}
		}
	printf("I am the parent, child %d has terminated\n", children[random-1]);
	sleep(5);
	return 0;
	}
}