#include <stdio.h> 
#include <iostream>  
#include <pthread.h>  
#include <string> 
#include <unistd.h>   
#include <pthread.h>
#include <sys/types.h>
#include <semaphore.h>
#include <time.h>
using namespace std;
//this code is heavly influenced by the code of Alejandro Garcia
//https://d2l.ucalgary.ca/d2l/le/content/426489/Home?itemIdentifier=D2L.LE.Content.ContentObject.ModuleCO-5253103

//variable for input, to be used to set the number of chopsticks
int inp;
//counter for time spent hungry
long timeHungry=0;
struct WaiterMonitor{
   //How many chopsticks are available - this should be user input between 5 and 10 (inclusive)
    int chopsticks_available;
	
    //Mutex-semaphore used to restrict threads entering a method in this monitor
    //Keep in mind many threads may be inside a method in this monitor, but at most ONE should be executing (the rest should be waiting)
    sem_t mutex_sem;
	
    //Mutex-semaphore and counter used to keep track of how many threads are waiting inside a method in this monitor (besides those waiting for a condition)
    sem_t next_sem;
    int next_count;

    //Semaphore and counter to keep track of threads waiting for the 'at least one chopstick available' condition
        //These should be the philosopher threads waiting on the right chopstick
    sem_t condition_can_get_1_sem;
    int condition_can_get_1_count;

    //Semaphore and counter to keep track of threads waiting for the 'at least two chopsticks available' condition
        //These should be the philosopher threads waiting on the left chopstick
    sem_t condition_can_get_2_sem;
    int condition_can_get_2_count;

    void init()
    {   //linking the number of chop sticks to the input from the user
        chopsticks_available=inp;
        //Mutex to gain access to (any) method in this monitor is initialized to 1
        sem_init(&mutex_sem, 0, 1);
        
        //'Next' semaphore - the semaphore for threads waiting inside a method of this monitor - is initialized to 0 (meaning no threads are initially in a method, as is logical)
        sem_init(&next_sem, 0, 0);
        next_count = 0;

        //Condition semaphores are intialized to 0 (meaning no threads are initially waiting on these conditions, as is logical)
        sem_init(&condition_can_get_1_sem, 0, 0);
        condition_can_get_1_count = 0;
        sem_init(&condition_can_get_2_sem, 0, 0);
        condition_can_get_2_count = 0;
    }


    void condition_wait(sem_t &condition_sem, int &condition_count)
    {
		//condition count is the number of threads waiting on the condition, increment it since the thread calling this method is about to wait
        condition_count++;
		//If there is a waiting thread INSIDE a method in this monitor, they get priority, so post to that semaphore
        if (next_count > 0)
            sem_post(&next_sem);
		//Otherwise, post to the general entry semaphore (the mutex, that is)
        else
            sem_post(&mutex_sem);
		//Wait for this condition to be posted to (Note that as soon as someone posts to this condition, they will halt as this thread has priority!)
        sem_wait(&condition_sem);
		//If I reach here, I have finished waiting :)
        condition_count--;
    }

    void condition_post(sem_t &condition_sem, int &condition_count)
    {
		//If there are any threads waiting on the condition I want to post...
        if (condition_count > 0)
        {
			//...Then they have priority (they were waiting before me), I shall wait in the next_sem gang
            next_count++;
			//Post to the condition_sem gang so they can continue
            sem_post(&condition_sem);
			//Wait for someone to post to next_sem
            sem_wait(&next_sem);
            next_count--;
        }
    }


    void request_left_chopstick()
    {
        //A thread needs mutex access to enter any of this monitors' method!!!
        sem_wait(&mutex_sem);

        //Okay so we got mutex access...but what if there are less than 2 chopsticks available when I am requesting the left chopstick?...
        while(chopsticks_available < 2)
            //...Then wait for the 'at least two chopsticks available' semaphore per the waiter-implementation specifications!
            condition_wait(condition_can_get_2_sem, condition_can_get_2_count);

        //If we're here, then at least two chopsticks are available, use up one of them
        chopsticks_available--;

        if(chopsticks_available >= 1)
        {
            //If at least a chopstick remains, post to the 'at least one chopstick available' condition
            condition_post(condition_can_get_1_sem, condition_can_get_1_count);

             if(chopsticks_available >= 2)
             {
                //If at least two chopsticks remain, post to the 'at least two chopsticks available' condition
                condition_post(condition_can_get_2_sem, condition_can_get_2_count);
             }
        }

        //Threads waiting for next_sem are waiting INSIDE one of this monitor's methods...they get priority!
        if (next_count > 0)
            sem_post(&next_sem);
        //If no such threads exist... simply open up the general-access mutex!
        else
            sem_post(&mutex_sem);
    }

    void request_right_chopstick()
    {
        sem_wait(&mutex_sem);

        //checking to see if thier is atleast 1 chopstick
        while(chopsticks_available < 1)
            //...Then wait for the 'at least two chopsticks available' semaphore per the waiter-implementation specifications!
            condition_wait(condition_can_get_1_sem, condition_can_get_1_count);
        chopsticks_available--;

        //this just posts the required content
        if(chopsticks_available >= 1)
        {
            //If at least a chopstick remains, post to the 'at least one chopstick available' condition
            condition_post(condition_can_get_1_sem, condition_can_get_1_count);

             if(chopsticks_available >= 2)
             {
                //If at least two chopsticks remain, post to the 'at least two chopsticks available' condition
                condition_post(condition_can_get_2_sem, condition_can_get_2_count);
             }
        }

        if (next_count > 0)
            sem_post(&next_sem);
        else
            sem_post(&mutex_sem);
    }

    void return_chopsticks()
    {
        sem_wait(&mutex_sem);

		//returning the 2 chopsticks
        chopsticks_available+=2;
        //updating the conditions
        condition_post(condition_can_get_1_sem, condition_can_get_1_count);
        condition_post(condition_can_get_2_sem, condition_can_get_2_count);
        if (next_count > 0)
            sem_post(&next_sem);
        else
            sem_post(&mutex_sem);
    }

    void destroy()
    {
        sem_destroy(&mutex_sem);
        sem_destroy(&condition_can_get_1_sem);
        sem_destroy(&condition_can_get_2_sem);
    }
};
struct WaiterMonitor waiter;

void * thread_function(void * arg){

    int id = *((int*)arg);
    srand(time(NULL) + id);

    for(int i = 0; i < 3; i++)
    {
        printf("Philosopher %d is thinking\n", id);
        int think_time = 1 + (rand() % 5);
        sleep(think_time);

        printf("Philospher %d is hungry\n", id);
        //getting the time of when the philosopher is hungery
        time_t hunger=time(NULL);
        time(&hunger);
        waiter.request_left_chopstick();

        printf("Philospher %d has picked up left chopstick\n", id);

        //Get the right chopstick
        waiter.request_right_chopstick();
        time_t eating=time(NULL);
        time(&eating);
        //adding it to the counter
        timeHungry+=(long)eating-hunger;
        printf("Philospher %d has picked up right chopstick\nPhilosopher %d is eating\n", id, id);
        //Eat
        sleep(5);

        printf("Philospher %d is done eating\n", id);

        //Return our chopsticks
        waiter.return_chopsticks();
   
    }
    printf("Philospher %d is completely done eating\n", id);
    pthread_exit(NULL);
}

int main(){
    //the 5 philosophers and threads and there IDs
    pthread_t philosophers[5];
    int ids[5];
    //requesting the number of chopsticks
    cout<<"please enter the number of chopsticks (between 5 and 10)"<<endl;
    cin>>inp;
    //initialising the waiter
    waiter.init();
    //creating the ids and threads
    for(int i=0;i<5;i++){
        ids[i]=i+1;
        pthread_create(&philosophers[i], NULL, thread_function, (void*) &ids[i]);
    }
    //joining the threads
    for(int i=0;i<5;i++){
        pthread_join(philosophers[i], NULL);
    }
    //destorying the everything after its complete
    waiter.destroy();
    
    cout<<"average time sepent hungry: "<< timeHungry/15<<endl;

    return 0;
}