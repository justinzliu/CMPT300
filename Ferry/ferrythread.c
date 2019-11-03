#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

//compile with: gcc -Wall -o x ferrythread.c -lpthread -lrt

//structures
//Message to Vehicles
typedef struct vehicleMsg {
    //noncritical
	int theTruckProbability;
	int theIntervalTime;
	int theSeed;
    long nextArrivalTime;
} vehicleMsg;

/*****global values && mutex/semaphores*****/
int captain_live;
pthread_mutex_t m_vehicleCount;
//protects...
    int vehicle_count;
    pthread_t* vehicle_arr;
	int vehicle_arrSize;
pthread_mutex_t m_carCount;
//protects...
	int car_countinQ;
pthread_mutex_t m_truckCount;
//protects...
	int truck_countinQ;
	
//queues
sem_t s_carWaitQ;
sem_t s_truckWaitQ;
sem_t s_carLoadQ;
sem_t s_truckLoadQ;
sem_t s_carFerryQ;
sem_t s_truckFerryQ;
sem_t s_carExitQ;
sem_t s_truckExitQ;
//vehicle signals to captain
sem_t s_carLoaded;
sem_t s_carUnloaded; 
sem_t s_truckLoaded;
sem_t s_truckUnloaded; 

//misc functions
int timeChange( const struct timeval startTime ){
    //int status;
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
	int random = rand();
	int randomDistributed = (random%range) + minvalue;
	return randomDistributed;
}
//thread functions

void zoomzoomCar (vehicleMsg* vMsg) {
	//initialization
    pthread_t self_id;
    self_id = pthread_self();
    pthread_mutex_lock(&m_vehicleCount);
    if (vehicle_count > vehicle_arrSize-1) {
        vehicle_arrSize *= vehicle_arrSize;
        vehicle_arr = realloc(vehicle_arr,vehicle_arrSize*(sizeof(pthread_t)));
        if (vehicle_arr == NULL) {
            printf("*****ERROR***** failed to allocate memory\n");
            exit(0);
        }
    }
    vehicle_arr[vehicle_count] = self_id;
    vehicle_count++;
    pthread_mutex_unlock(&m_vehicleCount);
    pthread_mutex_lock(&m_carCount);
    car_countinQ++;
    pthread_mutex_unlock(&m_carCount);
    //car arrives and is queued, waiting for captain signal to load
    printf("CAR:  Car with threadID %lu queued\n",self_id);
    sem_wait(&s_carWaitQ);
    if (captain_live == 1) {
    	//captain has gone home, send all the cars home without a ferry ride
    	pthread_exit(NULL);
    }
    //captain signal car to load
    printf("CAR:  Car with threadID %lu leaving the queue to load\n",self_id);
    //tell captain car is loaded, car loads onto ferry
    sem_post(&s_carLoaded);
    printf("CAR:  Car with threadID %lu is onboard the ferry\n",self_id);
    sem_wait(&s_carLoadQ);
    //car is now on ferry, and is in sailing queue
    sem_wait(&s_carFerryQ);
    //car arrives at destination and is unloading
    printf("CAR:  Car with threadID %lu is now unloading\n",self_id);
    //tell captain car is unloaded
    sem_post(&s_carUnloaded);
    //car unloaded, now waiting for captain signal to leave
    printf("CAR:  Car with threadID %lu has unloaded\n",self_id);
    sem_wait(&s_carExitQ);
    //captain signal car to leave
    printf("CAR:  Car with threadID %lu is about to exit\n",self_id);
    pthread_exit(NULL);
}

void honkhonkTruck ( ) {
	//initialization
    pthread_t self_id;
    self_id = pthread_self();
    pthread_mutex_lock(&m_vehicleCount);
    if (vehicle_count > vehicle_arrSize-1) {
        vehicle_arrSize *= vehicle_arrSize;
        vehicle_arr = realloc(vehicle_arr,vehicle_arrSize*(sizeof(pthread_t)));
        if (vehicle_arr == NULL) {
            printf("*****ERROR***** failed to allocate memory\n");
            exit(0);
        }
    }
    vehicle_arr[vehicle_count] = self_id;
    vehicle_count++;
    pthread_mutex_unlock(&m_vehicleCount);
    pthread_mutex_lock(&m_truckCount);
    truck_countinQ++;
    pthread_mutex_unlock(&m_truckCount);
    //truck arrives and is queued, waiting for captain signal to load
    printf("TRUCK:  Truck with threadID %lu queued\n",self_id);
    sem_wait(&s_truckWaitQ);
    if (captain_live == 1) {
    	//captain has gone home, send all the trucks home without a ferry ride
    	pthread_exit(NULL);
    }
    //captain signal truck to load
    printf("TRUCK:  Truck with threadID %lu leaving the queue to load\n",self_id);
    //tell captain car is loaded, car loads onto ferry
    sem_post(&s_truckLoaded);
    printf("TRUCK:  Truck with threadID %lu is onboard the ferry\n",self_id);
    sem_wait(&s_truckLoadQ);
    //car is now on ferry, and is in sailing queue
    sem_wait(&s_truckFerryQ);
    //car arrives at destination and is unloading
    printf("TRUCK:  Truck with threadID %lu is now unloading\n",self_id);
    //tell captain car is unloaded
    sem_post(&s_truckUnloaded);
    //car unloaded, now waiting for captain signal to leave
    printf("TRUCK:  Truck with threadID %lu has unloaded\n",self_id);
    sem_wait(&s_truckExitQ);
    //captain signal car to leave
    printf("TRUCK:  Truck with threadID %lu is about to exit\n",self_id);
    pthread_exit(NULL);
}

void ahoyCaptain() {
    printf("Captain  Captain thread started\n");
    //max 5 runs, (max 24 cars) vehicles generated infinitely until runs completed
    for (int i=0; i<4; i++) {
        //6 car max, truck worth 2, max 2 truck
        int ferry_capacity = 6;
        int truck_allowance = 2;
        int car_inFerry = 0;
        int truck_inFerry = 0;
        int vehicle_countinQ = car_countinQ + truck_countinQ;
        //wait until at least 8 vehicles in queue, take remaining vehicles if on last run
        while (vehicle_countinQ < 8) {
            usleep(1000000);
            vehicle_countinQ = car_countinQ + truck_countinQ;
        };
        //Check on-time queue
        pthread_mutex_lock(&m_carCount);
        pthread_mutex_lock(&m_truckCount);
        int c_carCount = car_countinQ;
        int c_truckCount = truck_countinQ;
        pthread_mutex_unlock(&m_carCount);
        pthread_mutex_unlock(&m_truckCount);
        //while trucks still in queue and truck allowance is not exceeded
        while (c_truckCount > 0 && ferry_capacity > 2) {
        	//fill ferry with up to 2 trucks
            printf ("CAPTAIN:    Truck selected for loading\n");
            //signal truck from wait queue
            sem_post(&s_truckWaitQ);
            //wait for signal from truck
            sem_wait(&s_truckLoaded);
            printf ("CAPTAIN:    Captain knows truck is loaded\n"); 
            //update counts
            pthread_mutex_lock(&m_truckCount);
            truck_countinQ--;
            pthread_mutex_unlock(&m_truckCount);
            ferry_capacity -= 2;
            truck_allowance--;
            c_truckCount--;
            truck_inFerry++;
        }
        //while cars still in queue and ferry capacity is not exceeded
        while (c_carCount > 0 && ferry_capacity > 0) {
        	//fill ferry space remaining with cars 
            printf ("CAPTAIN:    Car selected for loading\n");
            //signal car from wait queue
            sem_post(&s_carWaitQ);
            //wait for signal from truck
            sem_wait(&s_carLoaded);
            printf ("CAPTAIN:    Captain knows car is loaded\n");
            //update counts
            pthread_mutex_lock(&m_carCount);
            car_countinQ--;
            pthread_mutex_unlock(&m_carCount);
            ferry_capacity--;
            c_carCount--;
            car_inFerry++;
        }
        //Check late queue if there is still space in ferry
        while (ferry_capacity > 0) {
        	//check late queue
            pthread_mutex_lock(&m_truckCount);
            pthread_mutex_lock(&m_carCount);
            int c_carCount = car_countinQ;
            int c_truckCount = truck_countinQ;
            pthread_mutex_unlock(&m_truckCount);
            pthread_mutex_unlock(&m_carCount);
            //while trucks still in queue and truck allownance is not exceeded and ferry capacity can still take a truck
            while (c_truckCount > 0 && truck_allowance > 0 && ferry_capacity > 1) {
            	//fill ferry space with up to 2 trucks
                printf ("CAPTAIN:    Truck selected for loading\n");
                //signal truck from wait queue
                sem_post(&s_truckWaitQ);
                //wait for signal from truck
                sem_wait(&s_truckLoaded);
                printf ("CAPTAIN:    Captain knows truck is loaded\n"); 
                //update counts
                pthread_mutex_lock(&m_truckCount);
                truck_countinQ--;
                pthread_mutex_unlock(&m_truckCount);
                ferry_capacity -= 2;
                truck_allowance--;
                c_truckCount--;
                truck_inFerry++;
            }
            //while cars still in queue and ferry capacity is not exceeded
            while (c_carCount > 0 && ferry_capacity > 0) {
            	//fill ferry space remaing with cars
                printf ("CAPTAIN:    Car selected for loading\n"); 
                //signal car from wait queue
                sem_post(&s_carWaitQ);
                //wait for signal from car
                sem_wait(&s_carLoaded);
                printf ("CAPTAIN:    Captain knows car is loaded\n");
                //update counts
                pthread_mutex_lock(&m_carCount);
                car_countinQ--;
                pthread_mutex_unlock(&m_carCount);
                ferry_capacity--;
                c_carCount--;
                car_inFerry++;
            }   
        }
        //move all cars/trucks from LoadQ (loaded on ferry) to FerryQ (sailing)
        for (int j=0; j<car_inFerry; j++) {
        	sem_post(&s_carLoadQ);
        }
        for (int j=0; j<truck_inFerry; j++) {
        	sem_post(&s_truckLoadQ);
        }
        printf("CAPTAIN:    Ferry is full, starting to sail\n");
        //sailing takes 2 seconds
        usleep(2000000);
        printf("CAPTAIN:    Ferry has reached the destination port\n");
        for (int j=0; j<truck_inFerry; j++) {
        	sem_post(&s_truckFerryQ);
        	sem_wait(&s_truckUnloaded);
        	printf ("CAPTAIN:    Captain knows truck has unloaded from the ferry\n");
        	sem_post(&s_truckExitQ);
        	printf ("CAPTAIN:    Captain sees a truck leaving the ferry terminal\n");
        }
        for (int j=0; j<car_inFerry; j++) {
        	sem_post(&s_carFerryQ);
        	sem_wait(&s_carUnloaded);
        	printf ("CAPTAIN:    Captain knows car has unloaded from the ferry\n");
        	sem_post(&s_carExitQ);
        	printf ("CAPTAIN:    Captain sees a car leaving the ferry terminal\n");
     	}
       	//captain knows all vehicles have unloaded the ferry, return to dock for next load
    }
    //4 loads completed. terminated when arriving at dock for the 5th time
    //captain sets signal that it is complete
    captain_live = 1;
    pthread_exit(NULL);
}

void myfuncVehicle (vehicleMsg* vMsg) {
	//initialization
    pthread_t makeCar, makeTruck;
    int status, min, max, result;
    struct timeval startTime;
    gettimeofday(&startTime, NULL);
    printf("CREATEVEHICLE:  Vehicle creation thread has been started\n");
    srand(vMsg->theSeed);
    while (captain_live == 0) {
        int eTime = timeChange(startTime);
        if (eTime > vMsg->nextArrivalTime) {
            printf("CREATEVEHICLE:  Elapsed time %d msec\n",eTime);
            min = 0;
            max = 100;
            result = uniformDistribution(min,max);
            if (result <= vMsg->theTruckProbability) {
                //make a truck
                status = pthread_create(&makeTruck, NULL, (void*) &honkhonkTruck, NULL);
                if (status != 0) {
                    printf("***ERROR*** creating thread\n");
                }
                else {
                    printf("CREATEVEHICLE:  Created a truck thread\n");
                }
            }
            else {
                //make a car
                status = pthread_create(&makeCar, NULL, (void*) &zoomzoomCar, NULL);
                    if (status != 0) {
                        printf("***ERROR*** creating thread\n");
                    }
                    else {
                        printf("CREATEVEHICLE:  Created a car thread\n");
                    }
            }
        min = 1000;
        max = vMsg->theIntervalTime;
        result = uniformDistribution(min,max);
        vMsg->nextArrivalTime = eTime + result;
        printf("CREATEVEHICLE:  Next arrival time %ld msec\n",vMsg->nextArrivalTime);
        }
    }
    //captain is done all the runs for the day
    //captain tells vehicle thread it will not ferry anymore vehicles
    pthread_exit(NULL);
}


//declarations
void myfuncVehicle (vehicleMsg* vMsg);
void ahoyCaptain();
void zoomzoomCar ();
void honkhonkTruck ();
int timeChange( const struct timeval startTime  );
int uniformDistribution(int minvalue, int maxvalue);

int main() {

    //initialize semaphores/mutexes (does not go below 0)
    sem_init(&s_carWaitQ,0,0);
    sem_init(&s_truckWaitQ,0,0);
    sem_init(&s_carLoadQ,0,0);
    sem_init(&s_truckLoadQ,0,0);
    sem_init(&s_carFerryQ,0,0);
    sem_init(&s_truckFerryQ,0,0);
    sem_init(&s_carExitQ,0,0);
    sem_init(&s_truckExitQ,0,0);
    sem_init(&s_carLoaded,0,0);
    sem_init(&s_carUnloaded,0,0);
    sem_init(&s_truckLoaded,0,0);
    sem_init(&s_truckUnloaded,0,0);

    pthread_mutex_init(&m_vehicleCount,NULL);
    pthread_mutex_init(&m_carCount,NULL);
    pthread_mutex_init(&m_truckCount,NULL);

    //global variables
    vehicle_count = 0;
    vehicle_arrSize = 24;
    car_countinQ = 0;
    truck_countinQ = 0;
  	captain_live = 0;

    int main_status;
    int truckProbability;
    int intervalTime;
    int seed;

    //user inputs1: probability of a truck
    printf("Please enter integer values for the follow variables\nEnter the percent probability that the next vehicle is a truck (0 <= probability <= 100): ");
    scanf("%d",&truckProbability );
    if (truckProbability == 100) {
    	printf("*****WARNING***** this will cause an infinite loop (ferry cannot take more than two trucks, and only trucks are generated)");
    }
    while (truckProbability < 0 || truckProbability > 100) {
    	printf("*****INVALID ENTRY*****\nEnter the percent probability that the next vehicle is a truck: ");
    	scanf("%d",&truckProbability);
    }
    //user input2: min time before next car/truck thread
    printf("\nEnter the maximum length (in milliseconds) of the interval between vehicles (1000 <= length <= 5000): ");
    scanf("%d",&intervalTime);
    //not sure if interval was 1000 < K < 5000 or 1000 <= K <= 5000. main instructions are the former, vehiclethread instructions are the latter. Assuming the latter
    while (intervalTime < 1000 || intervalTime > 5000) {
    	printf("*****INVALID ENTRY*****\nEnter the maximum length (in milliseconds) of the interval between vehicles (1000 < length < 5000): ");
    	scanf("%d",&intervalTime);
    }
    //user input3:
    printf("\nEnter the seed for random number generation (2 < seed < RAND_MAX): ");
    scanf("%d",&seed);
    while (seed <= 2 || seed >= RAND_MAX) {
    	printf("*****INVALID ENTRY*****\nEnter the seed for random number generation (2 < seed < RAND_MAX): ");
    	scanf("%d",&seed);
    }
    printf("\n");
    
    vehicleMsg vMsg;   
    vMsg.theTruckProbability = truckProbability;
	vMsg.theIntervalTime = intervalTime;
	vMsg.theSeed = seed;
    vMsg.nextArrivalTime = 0;
    vehicle_arr = (pthread_t*)malloc(vehicle_arrSize*sizeof(pthread_t));
    if (vehicle_arr == NULL) {
        printf("*****ERROR***** failed to allocate memory");
        return 1;
    }

    //first Vehicle created
    pthread_t Vehicle;
    main_status = pthread_create(&Vehicle, NULL, (void*) &myfuncVehicle, &vMsg);
    if (main_status != 0) {
    	printf("***ERROR*** creating thread\n");
    }
    pthread_t Captain;
    main_status = pthread_create (&Captain, NULL, (void*) &ahoyCaptain, NULL);
    if (main_status != 0) {
   		printf("***ERROR*** creating thread\n");
    }
    //wait until vehicle terminates
    main_status = pthread_join(Vehicle, NULL);
    if (main_status != 0) {
   		printf("***ERROR*** joining thread\n");
    }
    //wait until captain terminates
    main_status = pthread_join(Captain,NULL);
    if (main_status != 0) {
   		printf("***ERROR*** joining thread\n");
    }
    //send away all vehicle threads that will not be taken on ferry today
    for (int k = 0; k<car_countinQ; k++) {
        sem_post(&s_carWaitQ);
    }
    for (int k = 0; k<truck_countinQ; k++) {
        sem_post(&s_truckWaitQ);
    }
    //check all ferried vehicle threads and wait for them to terminate
    for (int k=0; k<vehicle_count; k++) {
    	main_status = pthread_join(vehicle_arr[k],NULL);
	    if (main_status != 0) {
	   		printf("*****ERROR***** vehicle thread failed to join\n");
	    }
	}

    //clean up
    free(vehicle_arr);

    sem_destroy(&s_carWaitQ);
    sem_destroy(&s_truckWaitQ);
    sem_destroy(&s_carLoadQ);
    sem_destroy(&s_truckLoadQ);
    sem_destroy(&s_carFerryQ);
    sem_destroy(&s_truckFerryQ);
    sem_destroy(&s_carExitQ);
    sem_destroy(&s_truckExitQ);
    sem_destroy(&s_carLoaded);
    sem_destroy(&s_carUnloaded);
    sem_destroy(&s_truckLoaded);
    sem_destroy(&s_truckUnloaded);
	
	pthread_mutex_destroy(&m_vehicleCount);
    pthread_mutex_destroy(&m_carCount);
    pthread_mutex_destroy(&m_truckCount);

    return 0;
}