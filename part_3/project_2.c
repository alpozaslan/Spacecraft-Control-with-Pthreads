#include "queue.c"
#include <pthread.h>
#include <sys/time.h>
#include <string.h>
#include <stdio.h>

#define LANDING_JOB 1
#define LAUNCH_JOB 2
#define ASSEMBLY_JOB 3
#define UNIT_TIME 2
#define EMERGENCY_JOB 4

#define LANDING_JOB_DURATION 2
#define LAUNCH_JOB_DURATION 4
#define ASSEMBLY_JOB_DURATION 12
#define EMERGENCY_JOB_DURATION 2

int simulationTime = 120; // simulation time
time_t deadline = 0;      // deadline
time_t simulationStartTime = 0;
int n = 30;    // simulation start time
int seed = 10; // seed for randomness
int emergencyFrequency = 5; // frequency of emergency
float p = 0.2; // probability of a ground job (launch & assembly)

void *LandingJob(void *arg);
void *LaunchJob(void *arg);
void *EmergencyJob(void *arg);
void *AssemblyJob(void *arg);
void *ControlTower(void *arg);
void *PadA(void *arg);
void *PadB(void *arg);
void *WriteLog(char *log);
void *PrintCurrentQueues(void *arg);
void *PrintQueue(Queue *queue);

Queue *landingQueue;
Queue *launchQueue;
Queue *assemblyQueue;
Queue *emergencyQueue;
Queue *padAEmergencyQueue;
Queue *padBEmergencyQueue;
Queue *padAQueue;
Queue *padBQueue;

// create a mutex for each queue
pthread_mutex_t landingQueueMutex;
pthread_mutex_t launchQueueMutex;
pthread_mutex_t assemblyQueueMutex;
pthread_mutex_t emergencyQueueMutex;
pthread_mutex_t padAEmergencyQueueMutex;
pthread_mutex_t padBEmergencyQueueMutex;
pthread_mutex_t padAQueueMutex;
pthread_mutex_t padBQueueMutex;

// create log file mutex
pthread_mutex_t logFileMutex;

// pthread sleeper function
int pthread_sleep(int seconds)
{
    pthread_mutex_t mutex;
    pthread_cond_t conditionvar;
    struct timespec timetoexpire;
    if (pthread_mutex_init(&mutex, NULL))
    {
        return -1;
    }
    if (pthread_cond_init(&conditionvar, NULL))
    {
        return -1;
    }
    struct timeval tp;
    // When to expire is an absolute time, so get the current time and add it to our delay time
    gettimeofday(&tp, NULL);
    timetoexpire.tv_sec = tp.tv_sec + seconds;
    timetoexpire.tv_nsec = tp.tv_usec * 1000;

    pthread_mutex_lock(&mutex);
    int res = pthread_cond_timedwait(&conditionvar, &mutex, &timetoexpire);
    pthread_mutex_unlock(&mutex);
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&conditionvar);

    // Upon successful completion, a value of zero shall be returned
    return res;
}

int main(int argc, char **argv)
{
    // -p (float) => sets p
    // -t (int) => simulation time in seconds
    // -s (int) => change the random seed
    for (int i = 1; i < argc; i++)
    {
        if (!strcmp(argv[i], "-p"))
        {
            p = atof(argv[++i]);
        }
        else if (!strcmp(argv[i], "-t"))
        {
            simulationTime = atoi(argv[++i]);
        }
        else if (!strcmp(argv[i], "-s"))
        {
            seed = atoi(argv[++i]);
        }
        else if (!strcmp(argv[i], "-n"))
        {
            n = atoi(argv[++i]);
        }
    }

    srand(seed); // feed the seed

    // your code goes here

    // simulation start time
    simulationStartTime = time(NULL);

    // open the log.txt and write the header for the columns EventID, Status, Request Time, End Time, Turnaround Time, Pad
    FILE *logFile = fopen("log.txt", "w");
    fprintf(logFile, "EventID, Status, Request Time, End Time, Turnaround Time, Pad\n");
    fclose(logFile);

    // add current time to the simulationTime
    deadline = time(NULL) + simulationTime;

    landingQueue = ConstructQueue(1000);
    launchQueue = ConstructQueue(1000);
    assemblyQueue = ConstructQueue(1000);
    emergencyQueue = ConstructQueue(1000);
    padAEmergencyQueue = ConstructQueue(1000);
    padBEmergencyQueue = ConstructQueue(1000);
    padAQueue = ConstructQueue(1000);
    padBQueue = ConstructQueue(1000);

    pthread_t landingThread;
    pthread_t launchThread;
    pthread_t assemblyThread;
    pthread_t emergencyThread;
    pthread_t controlTowerThread;
    pthread_t padAThread;
    pthread_t padBThread;
    pthread_t printCurrentQueuesThread;

    // initialize mutexes
    pthread_mutex_init(&landingQueueMutex, NULL);
    pthread_mutex_init(&launchQueueMutex, NULL);
    pthread_mutex_init(&assemblyQueueMutex, NULL);
    pthread_mutex_init(&emergencyQueueMutex, NULL);
    pthread_mutex_init(&padAEmergencyQueueMutex, NULL);
    pthread_mutex_init(&padBEmergencyQueueMutex, NULL);
    pthread_mutex_init(&padAQueueMutex, NULL);
    pthread_mutex_init(&padBQueueMutex, NULL);
    pthread_mutex_init(&logFileMutex, NULL);

    pthread_create(&landingThread, NULL, LandingJob, NULL);
    pthread_create(&launchThread, NULL, LaunchJob, NULL);
    pthread_create(&assemblyThread, NULL, AssemblyJob, NULL);
    pthread_create(&emergencyThread, NULL, EmergencyJob, NULL);
    pthread_create(&controlTowerThread, NULL, ControlTower, NULL);
    pthread_create(&padAThread, NULL, PadA, NULL);
    pthread_create(&padBThread, NULL, PadB, NULL);
    pthread_create(&printCurrentQueuesThread, NULL, PrintCurrentQueues, NULL);

    // join threads
    pthread_join(landingThread, NULL);
    pthread_join(launchThread, NULL);
    pthread_join(assemblyThread, NULL);
    pthread_join(emergencyThread, NULL);
    pthread_join(controlTowerThread, NULL);
    pthread_join(padAThread, NULL);
    pthread_join(padBThread, NULL);
    pthread_join(printCurrentQueuesThread, NULL);

    // destroy queues
    DestructQueue(landingQueue);
    DestructQueue(launchQueue);
    DestructQueue(assemblyQueue);
    DestructQueue(emergencyQueue);
    DestructQueue(padAEmergencyQueue);
    DestructQueue(padBEmergencyQueue);
    DestructQueue(padAQueue);
    DestructQueue(padBQueue);

    return 0;
}

// the function that creates plane threads for landing
void *LandingJob(void *arg)
{
    while (time(NULL) < deadline)
    {
        // sleep for UNIT_TIME seconds
        pthread_sleep(UNIT_TIME);
        // create a landing job with probability 1-p
        if (rand() % 100 < 100 - p * 100)
        {
            Job j;
            j.ID = rand() % 1000;
            j.type = LANDING_JOB;
            j.duration = LANDING_JOB_DURATION;
            j.arrivalTime = time(NULL) - simulationStartTime;
            // lock the landing queue
            pthread_mutex_lock(&landingQueueMutex);
            // print the job id type and the time for debugging
            // printf("id: %d type: %d time: %d\n", j.ID, j.type, time(NULL));
            Enqueue(landingQueue, j);
            // unlock the landing queue
            pthread_mutex_unlock(&landingQueueMutex);
        }
    }
    return NULL;
}

// the function that creates plane threads for departure
void *LaunchJob(void *arg)
{
    while (time(NULL) < deadline)
    {
        // sleep for UNIT_TIME seconds
        pthread_sleep(UNIT_TIME);
        // create a landing job with probability p/2
        if (rand() % 100 < (p / 2) * 100)
        {
            Job j;
            j.ID = rand() % 1000;
            j.type = LAUNCH_JOB;
            j.duration = LAUNCH_JOB_DURATION;
            j.arrivalTime = time(NULL) - simulationStartTime;
            // lock the landing queue
            pthread_mutex_lock(&launchQueueMutex);
            // print the job id type and the time for debugging
            // printf("id: %d type: %d time: %d\n", j.ID, j.type, time(NULL));
            Enqueue(launchQueue, j);
            // unlock the landing queue
            pthread_mutex_unlock(&launchQueueMutex);
        }
    }
    return NULL;
}

// the function that creates plane threads for emergency landing
void* EmergencyJob(void *arg){
    while (time(NULL) < deadline)
    {
        // sleep for UNIT_TIME seconds
        pthread_sleep(emergencyFrequency * UNIT_TIME);
        // create 2 emergency landing job
        for (int i = 0; i < 2; i++)
        {
            Job j;
            j.ID = rand() % 1000;
            j.type = EMERGENCY_JOB;
            j.duration = EMERGENCY_JOB_DURATION;
            j.arrivalTime = time(NULL) - simulationStartTime;
            // lock the emergency queue
            pthread_mutex_lock(&emergencyQueueMutex);
            
            Enqueue(emergencyQueue, j);
            // unlock the emergency queue
            pthread_mutex_unlock(&emergencyQueueMutex);
        }
    }
    return NULL;
}

// the function that creates plane threads for emergency landing
void *AssemblyJob(void *arg)
{
    while (time(NULL) < deadline)
    {
        // sleep for UNIT_TIME seconds
        pthread_sleep(UNIT_TIME);
        // create a landing job with probability p/2
        if (rand() % 100 < (p / 2) * 100)
        {
            Job j;
            j.ID = rand() % 1000;
            j.type = ASSEMBLY_JOB;
            j.duration = ASSEMBLY_JOB_DURATION;
            j.arrivalTime = time(NULL) - simulationStartTime;
            // lock the landing queue
            pthread_mutex_lock(&assemblyQueueMutex);
            // print the job id type and the time for debugging
            // printf("id: %d type: %d time: %d\n", j.ID, j.type, time(NULL));
            Enqueue(assemblyQueue, j);
            // unlock the landing queue
            pthread_mutex_unlock(&assemblyQueueMutex);
        }
    }
    return NULL;
}

// the function that controls the air traffic
void *ControlTower(void *arg)
{
    while (time(NULL) < deadline)
    {
        // check if there is a emergency job
        // lock the emergency queue
        pthread_mutex_lock(&emergencyQueueMutex);
        // if there is a emergency job
        if (emergencyQueue->size > 0)
        {
            // lock the padA emergency queue
            pthread_mutex_lock(&padAEmergencyQueueMutex);

            Enqueue(padAEmergencyQueue, Dequeue(emergencyQueue));
            
            // unlock the emergency queue and padA emergency queue
            pthread_mutex_unlock(&padAEmergencyQueueMutex);
        }
        if (emergencyQueue->size > 0)
        {
            // lock the padB emergency queue
            pthread_mutex_lock(&padBEmergencyQueueMutex);

            Enqueue(padBEmergencyQueue, Dequeue(emergencyQueue));
            
            // unlock the emergency queue and padB emergency queue
            pthread_mutex_unlock(&padBEmergencyQueueMutex);
        }
        // unlock the emergency queue
        pthread_mutex_unlock(&emergencyQueueMutex);


        // lock the landing, launch, and assembly queues
        pthread_mutex_lock(&launchQueueMutex);
        pthread_mutex_lock(&assemblyQueueMutex);

        // if the launch or assembly queue has less than 3 jobs, then empty the landing queue
        if (launchQueue->size < 3 && assemblyQueue->size < 3)
        {
            // unlock the launch and assembly queues
            pthread_mutex_unlock(&launchQueueMutex);
            pthread_mutex_unlock(&assemblyQueueMutex);

            // lock the landing queue
            pthread_mutex_lock(&landingQueueMutex);
            // empty the landing queue
            while (!isEmpty(landingQueue))
            {
                // lock padAQueue and padBQueue
                pthread_mutex_lock(&padAQueueMutex);
                pthread_mutex_lock(&padBQueueMutex);
                if (padAQueue->duration <= padBQueue->duration)
                {
                    Enqueue(padAQueue, Dequeue(landingQueue));
                }
                else
                {
                    Enqueue(padBQueue, Dequeue(landingQueue));
                }
                // unlock padAQueue and padBQueue
                pthread_mutex_unlock(&padAQueueMutex);
                pthread_mutex_unlock(&padBQueueMutex);
            }
            // unlock the landing queue
            pthread_mutex_unlock(&landingQueueMutex);

            // lock the padAQueue and launchQueue
            pthread_mutex_lock(&padAQueueMutex);
            pthread_mutex_lock(&launchQueueMutex);

            if (!isEmpty(launchQueue))
            {
                Enqueue(padAQueue, Dequeue(launchQueue));
            }
            // unlock the padAQueue and launchQueue
            pthread_mutex_unlock(&padAQueueMutex);
            pthread_mutex_unlock(&launchQueueMutex);

            // lock the padBQueue and assemblyQueue
            pthread_mutex_lock(&padBQueueMutex);
            pthread_mutex_lock(&assemblyQueueMutex);

            if (!isEmpty(assemblyQueue))
            {
                Enqueue(padBQueue, Dequeue(assemblyQueue));
            }
            // unlock the padBQueue and assemblyQueue
            pthread_mutex_unlock(&padBQueueMutex);
            pthread_mutex_unlock(&assemblyQueueMutex);
        }
        else // Take one job from each queue
        {
            // lock padAQueue and padBQueue
            pthread_mutex_lock(&padAQueueMutex);
            pthread_mutex_lock(&padBQueueMutex);

            // if launchQueue is not empty, take the first job from launchQueue and put it in padAQueue
            if (!isEmpty(launchQueue))
            {
                Enqueue(padAQueue, Dequeue(launchQueue));
            }
            // unlock launchQueue
            pthread_mutex_unlock(&launchQueueMutex);

            // if assemblyQueue is not empty, take the first job from assemblyQueue and put it in padBQueue
            if (!isEmpty(assemblyQueue))
            {
                Enqueue(padBQueue, Dequeue(assemblyQueue));
            }
            // unlock assemblyQueue
            pthread_mutex_unlock(&assemblyQueueMutex);

            // if landingQueue is not empty, take the first job from landingQueue and put it in shortest pad
            // lock landingQueue
            pthread_mutex_lock(&landingQueueMutex);
            if (!isEmpty(landingQueue))
            {
                if (padAQueue->duration <= padBQueue->duration)
                {
                    Enqueue(padAQueue, Dequeue(landingQueue));
                }
                else
                {
                    Enqueue(padBQueue, Dequeue(landingQueue));
                }
            }

            // unlock padAQueue, padBQueue, and landingQueue
            pthread_mutex_unlock(&padAQueueMutex);
            pthread_mutex_unlock(&padBQueueMutex);
            pthread_mutex_unlock(&landingQueueMutex);
        }
    }

    return NULL;
}

void *PadA(void *arg)
{
    while (time(NULL) < deadline)
    {
        // lock padAEmergencyQueue
        pthread_mutex_lock(&padAEmergencyQueueMutex);
        if(padAEmergencyQueue->size > 0) {

            int sleepTime = padAEmergencyQueue->head->data.duration;
            // unlock padAEmergencyQueue
            pthread_mutex_unlock(&padAEmergencyQueueMutex);

            pthread_sleep(sleepTime);

            pthread_mutex_lock(&padAEmergencyQueueMutex);
            Job j = Dequeue(padAEmergencyQueue);
            pthread_mutex_unlock(&padAEmergencyQueueMutex);

            // create the log string
            time_t end_time = time(NULL) - simulationStartTime;
            char log[100];
            sprintf(log, "%-5d %5d %10d %10d %10d %10c\n", j.ID, j.type, j.arrivalTime, end_time, end_time - j.arrivalTime, 'A');
            WriteLog(log);
            continue;
        }
        else {
            // unlock padAEmergencyQueue
            pthread_mutex_unlock(&padAEmergencyQueueMutex);
        }
        // lock padAQueue
        pthread_mutex_lock(&padAQueueMutex);
        if (isEmpty(padAQueue))
        {
            pthread_mutex_unlock(&padAQueueMutex);
            // sleep for UNIT_TIME seconds
            pthread_sleep(UNIT_TIME);
        }
        else
        {
            int sleepTime = padAQueue->head->data.duration;
            // unlock padAQueue
            pthread_mutex_unlock(&padAQueueMutex);

            pthread_sleep(sleepTime);

            pthread_mutex_lock(&padAQueueMutex);
            Job j = Dequeue(padAQueue);
            pthread_mutex_unlock(&padAQueueMutex);

            // create the log string
            time_t end_time = time(NULL) - simulationStartTime;
            char log[100];
            sprintf(log, "%-5d %5d %10d %10d %10d %10c\n", j.ID, j.type, j.arrivalTime, end_time, end_time - j.arrivalTime, 'A');
            WriteLog(log);
        }
    }
    return NULL;
}

void *PadB(void *arg)
{
    while (time(NULL) < deadline)
    {
        // lock padBEmergencyQueue
        pthread_mutex_lock(&padBEmergencyQueueMutex);
        if(padBEmergencyQueue->size > 0) {
            int sleepTime = padBEmergencyQueue->head->data.duration;
            // unlock padBQueue
            pthread_mutex_unlock(&padBEmergencyQueueMutex);

            pthread_sleep(sleepTime);

            pthread_mutex_lock(&padBEmergencyQueueMutex);
            Job j = Dequeue(padBEmergencyQueue);
            pthread_mutex_unlock(&padBEmergencyQueueMutex);

            // create the log string
            time_t end_time = time(NULL) - simulationStartTime;
            char log[100];
            sprintf(log, "%-5d %5d %10d %10d %10d %10c\n", j.ID, j.type, j.arrivalTime, end_time, end_time - j.arrivalTime, 'A');
            WriteLog(log);
            continue;
        }
        else {
            // unlock padBEmergencyQueue
            pthread_mutex_unlock(&padBEmergencyQueueMutex);
        }
        // lock padBQueue
        pthread_mutex_lock(&padBQueueMutex);
        if (isEmpty(padBQueue))
        {
            pthread_mutex_unlock(&padBQueueMutex);
            // sleep for UNIT_TIME seconds
            pthread_sleep(UNIT_TIME);
        }
        else
        {
            int sleepTime = padBQueue->head->data.duration;
            // unlock padBQueue
            pthread_mutex_unlock(&padBQueueMutex);
            pthread_sleep(sleepTime);

            pthread_mutex_lock(&padBQueueMutex);
            Job j = Dequeue(padBQueue);
            pthread_mutex_unlock(&padBQueueMutex);

            // create the log string
            time_t end_time = time(NULL) - simulationStartTime;
            char log[100];
            sprintf(log, "%-5d %5d %10d %10d %10d %10c\n", j.ID, j.type, j.arrivalTime, end_time, end_time - j.arrivalTime, 'B');
            WriteLog(log);
        }
    }
    return NULL;
}

// open log.txt, protect it with mutex, and write the log to it
void *WriteLog(char *log)
{
    // lock the log file
    pthread_mutex_lock(&logFileMutex);
    // open the log file
    FILE *fp = fopen("log.txt", "a");
    // write the log to the log file
    fprintf(fp, "%s", log);
    // close the log file
    fclose(fp);
    // unlock the log file
    pthread_mutex_unlock(&logFileMutex);

    return NULL;
}

void *PrintCurrentQueues(void *arg)
{
    while (time(NULL) < deadline)
    {
        // sleep for 1 seconds
        pthread_sleep(1);
        int current_time = time(NULL) - simulationStartTime;
        if (n <= current_time)
        {
            // lock the landing queue
            pthread_mutex_lock(&landingQueueMutex);
            // print the landing queue
            printf("At %d sec landing: ", current_time);
            PrintQueue(landingQueue);
            // unlock the landing queue
            pthread_mutex_unlock(&landingQueueMutex);

            // lock the launch queue
            pthread_mutex_lock(&launchQueueMutex);
            // print the launch queue
            printf("At %d sec launch: ", current_time);
            PrintQueue(launchQueue);
            // unlock the launch queue
            pthread_mutex_unlock(&launchQueueMutex);

            // lock the assembly queue
            pthread_mutex_lock(&assemblyQueueMutex);
            // print the assembly queue
            printf("At %d sec assembly: ", current_time);
            PrintQueue(assemblyQueue);
            // unlock the assembly queue
            pthread_mutex_unlock(&assemblyQueueMutex);

            // lock the padA queue
            pthread_mutex_lock(&padAQueueMutex);
            // print the padA queue
            printf("At %d sec padA: ", current_time);
            PrintQueue(padAQueue);
            // unlock the padA queue
            pthread_mutex_unlock(&padAQueueMutex);

            // lock the padB queue
            pthread_mutex_lock(&padBQueueMutex);
            // print the padB queue
            printf("At %d sec padB: ", current_time);
            PrintQueue(padBQueue);
            // unlock the padB queue
            pthread_mutex_unlock(&padBQueueMutex);

            // lock the padA emergency queue
            pthread_mutex_lock(&padAEmergencyQueueMutex);
            // print the padA emergency queue
            printf("At %d sec padA emergency: ", current_time);
            PrintQueue(padAEmergencyQueue);
            // unlock the padA emergency queue
            pthread_mutex_unlock(&padAEmergencyQueueMutex);

            // lock the padB emergency queue
            pthread_mutex_lock(&padBEmergencyQueueMutex);
            // print the padB emergency queue
            printf("At %d sec padB emergency: ", current_time);
            PrintQueue(padBEmergencyQueue);
            // unlock the padB emergency queue
            pthread_mutex_unlock(&padBEmergencyQueueMutex);
        }
    }
    return NULL;
}

void *PrintQueue(Queue *q)
{
    if (isEmpty(q))
    {
        printf("empty\n");
    }
    else
    {
        NODE *curr = q->head;
        while (curr != NULL)
        {
            printf("%d ", curr->data.ID);
            curr = curr->prev;
        }
        printf("\n");
    }
}