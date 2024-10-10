//
// Created by cinnamon on 10/1/24.
//

#include <cstring>

#include "buffer.h"
#include <pthread.h>
#include <semaphore.h>
#include <iostream>
#include <unistd.h>
#include <random>
#include <string>
#include <atomic>
using namespace std;

#define MAX_THREADS 25
pthread_t tid[MAX_THREADS];

char VERBOSE = 'n';
static bool running = true;
static int maxSleepTime = 0;
static unsigned int seed = time(nullptr);

int actionsPerformed[MAX_THREADS * 2];
atomic<int> countBufferFull;
atomic<int> countBufferEmpty;

Buffer buffer = Buffer();


void* producer(void* arg);
void* consumer(void* arg);
void displayBuffer(const string& TITLE, int head, int tail);
void displayFinalStats (const int &SIMULATION_TIME, const int &NUM_PRODUCERS, const int &NUM_CONSUMERS);

sem_t numberMutex;
int pro_id(0);
int con_id(0);
int numberProcess(int PROCESS_TYPE);
bool isPrime(buffer_item item);



/*
 * 1 = Run time
 * 2 = Max Sleep time
 * 3 = number of producers
 * 4 = number of threads
 * 5 = yes or no verbose
 */
int main(int argc, char* argv[]) {

    sem_init(&numberMutex, 0, 1);

    //validate parameters
    string invalidArgMsg = "INVALID ARGUMENTS!!\n"
                            "The format of parameters are as followed:\n"
                               "\tint Run time\n"
                                "\tint Max sleep time of threads\n"
                                "\tint Number of Producers  ( MIN:1  MAX:25 )\n"
                                "\tint Number of Consumers  ( MIN:1  MAX:25 )\n"
                                "\tchar 'y' - Verbose mode\n\n";

    if (argc != 6) {
        printf("%s", invalidArgMsg.c_str());
        return -1;
    }

    const int MAX_RUN_TIME = atoi(argv[1]);
    maxSleepTime = atoi(argv[2]);
    const int NUM_PRODUCERS = atoi(argv[3]);
    const int NUM_CONSUMERS = atoi(argv[4]);

    con_id = NUM_PRODUCERS;

    // if any param is invalid or set to 0 return 1
    if (MAX_RUN_TIME == 0 || maxSleepTime == 0 || NUM_PRODUCERS == 0 || NUM_CONSUMERS == 0) {
        printf("%s", invalidArgMsg.c_str());
        return 1;
    }
    // if the user asks for more threads than the program can create return 2
    if (NUM_PRODUCERS > MAX_THREADS || NUM_CONSUMERS > MAX_THREADS) {
        printf("%s", invalidArgMsg.c_str());
        return 2;
    }

    VERBOSE = *argv[5];


    //init threads
    pthread_attr_t attr;
    pthread_attr_init(&attr);

    //create and assign threads
    cout<<"Starting threads..."<<endl;

    if (VERBOSE == 'y') {
        displayBuffer("",buffer.head, buffer.tail);
    }
    for (int i = 0; i < NUM_PRODUCERS; i++) {
        pthread_create(&tid[i], &attr, producer, 0);
    }
    for (int i = 0; i < NUM_CONSUMERS; i++) {
        pthread_create(&tid[i+NUM_PRODUCERS], &attr, consumer, 0);
    }
    sleep(MAX_RUN_TIME);
    running = false;
    //rejoin all threads
    int totalThreads = NUM_PRODUCERS + NUM_CONSUMERS;
    for (int i = 0; i < totalThreads; i++) {
        pthread_join(tid[i], 0);
    }
    // cout<<"Done"<<endl;
    // cout<<"Total times full:  " << countBufferFull << endl;
    // cout<<"Total times empty:  " << countBufferEmpty << endl;

    displayFinalStats(MAX_RUN_TIME, NUM_PRODUCERS, NUM_CONSUMERS);

    return 0;
}

void* producer(void *args) {
    int sleepTime;
    int process_ID = getpid();
    int ref_ID = numberProcess(1);
    if (VERBOSE == 'y') { //it is expensive to check if verbose is on every time a thread runs, so it will only check once
        string outputHeader;

        while (running) {
            sleepTime = rand_r(&seed) % 3;
            sleep(sleepTime);

            buffer_item producedItem = rand() % 100;
            if (!buffer.buffer_insert_item(producedItem)) {
                countBufferFull++;
                printf("All buffers full. Producer %d waits.\n\n", process_ID);
                continue;
            }
            outputHeader = "Producer " + to_string(process_ID) + " writes " + to_string(producedItem);
            actionsPerformed[ref_ID]++;
            displayBuffer(outputHeader,buffer.head, buffer.tail);
        }
    }
    else {
        while (running) {
            sleepTime = rand_r(&seed) % 3;
            sleep(sleepTime);

            buffer_item producedItem = rand() % 100;
            if (!buffer.buffer_insert_item(producedItem)) {
                countBufferFull++;
                continue;
            }
            actionsPerformed[ref_ID]++;
        }
        }
    printf("PRODUCER-%d ref %d\tProduced %d\n\n",process_ID,ref_ID,actionsPerformed[ref_ID]);
    pthread_exit(0);
}

void *consumer(void *arg) {
    buffer_item consumedItem;
    int sleepTime;
    int process_ID = getpid();
    int ref_ID = numberProcess(2);
    if (VERBOSE == 'y') {
        string outputHeader;
        while (running) {
            sleepTime = rand_r(&seed) % 3;
            sleep(sleepTime);


            if (!buffer.buffer_remove_item(&consumedItem)) {
                countBufferEmpty++;
                printf("All buffers empty. Consumer %d waits.\n\n", process_ID);
                continue;
            } else {
                if (isPrime(consumedItem))
                    outputHeader = "Consumer " + to_string(process_ID) + " reads " + to_string(consumedItem) + "\t*****PRIME NUMBER*****";
                else
                    outputHeader = "Consumer " + to_string(process_ID) + " reads " + to_string(consumedItem);

                actionsPerformed[ref_ID]++;
            }
            displayBuffer(outputHeader,buffer.head, buffer.tail);
        }
    } else {
        while (running) {
            sleepTime = rand_r(&seed) % 3;
            sleep(sleepTime);

            if (!buffer.buffer_remove_item(&consumedItem)) {
                countBufferEmpty++;
                continue;
            } else {
                    actionsPerformed[ref_ID]++;
            }
        }
    }
    // printf("CONSUMER-%d ref %d\tProduced %d\n\n",process_ID,ref_ID,actionsPerformed[ref_ID]);
    pthread_exit(0);
}

int numberProcess(const int PROCESS_TYPE) {
    sem_wait(&numberMutex);
    int ref_ID = -1;
    if (PROCESS_TYPE == 1) {
        ref_ID = pro_id;
        pro_id += 1;
    }
    if (PROCESS_TYPE == 2) {
        ref_ID = con_id;
        con_id += 1;
    }
    sem_post(&numberMutex);
    return ref_ID;
}

void displayBuffer(const string& TITLE, const int head, const int tail) {

    //locate where the R and W pointers on the buffer
    int i= 0 ;
    string spacer = "        ";
    string pointerLocations;
    if (head > tail) {
        for (i = 0; i < tail; i++) {
            pointerLocations.append(spacer);
        }
        pointerLocations.append("W");
        while (i < head) {
            pointerLocations.append(spacer);
            i++;
        }
        pointerLocations.append("R");
    } else {
        for (i = 0; i < head; i++) {
            pointerLocations.append(spacer);
        }
        pointerLocations.append("R");
        while (i < tail) {
            pointerLocations.append(spacer);
            i++;
        }
        pointerLocations.append("W");

    }

    //display to console
    printf("%s\n"
        "(buffers occupied: %d)\n"
           "buffers:\t%5d\t%5d\t%5d\t%5d\t%5d\n"
           "\t\t\t  ----    ----    ----    ----    ----\n"
           "\t\t\t   %s\n\n"
           ,TITLE.c_str(),buffer.size,buffer.buffer[0],buffer.buffer[1],buffer.buffer[2],buffer.buffer[3],buffer.buffer[4]
           ,pointerLocations.c_str());
}

void displayFinalStats (const int &SIMULATION_TIME, const int &NUM_PRODUCERS, const int &NUM_CONSUMERS) {
    int totalProduced (0), totalConsumed (0);
    int index;
    for (index = 0; index < NUM_PRODUCERS; index++) {
        totalProduced += actionsPerformed[index];
    }
    for (index = 0; index < NUM_CONSUMERS; index++) {
        totalConsumed += actionsPerformed[index+NUM_PRODUCERS];
    }
    string finalMessage =   "PRODUCER / CONSUMER SIMULATION COMPLETE \n"
                            "========================================\n"
                            "Simulation Time:\t\t\t\t\t\t" + to_string(SIMULATION_TIME) + "\n"
                            "Maximum Thread Sleep Time:\t\t\t\t" + to_string(maxSleepTime) + "\n"
                            "Number of Producer Threads:\t\t\t\t" + to_string(NUM_PRODUCERS) + "\n"
                            "Number of Consumer Threads:\t\t\t\t" + to_string(NUM_CONSUMERS) + "\n"
                            "Size of Buffer:\t\t\t\t\t\t\t" + to_string(buffer.size) + "\n"
                            "\n"
                            "Total Number of Items Produced:\t\t\t" + to_string(totalProduced) + "\n";
    for (index = 0; index < NUM_PRODUCERS; index++) {
        finalMessage +=     "\tThread " + to_string(index) + ":\t\t\t\t\t\t\t" + to_string(actionsPerformed[index]) + "\n";
    }
    finalMessage +=         "\n"
                            "Total Number of Items Consumed:\t\t\t" + to_string(totalConsumed) + "\n";
    for (; index < NUM_CONSUMERS + NUM_PRODUCERS; index++) {
        finalMessage +=     "\tThread " + to_string(index) + ":\t\t\t\t\t\t\t" + to_string(actionsPerformed[index]) + "\n";
    }

    finalMessage +=        "\n"
                           "Number Of Items Remaining in Buffer:\t" + to_string(buffer.size) + "\n"
                           "Number Of Times Buffer was Full:\t\t" + to_string(countBufferFull) + "\n"
                           "Number Of Times Buffer was Empty:\t\t" + to_string(countBufferEmpty) + "\n";

    // for (int entry : actionsPerformed) {
    //     cout << entry << "\n";
    // }

    cout << finalMessage;
}

bool isPrime(buffer_item item) {
    if (item == 1 || item == 0) return false;

    int sqrt = 0;
    for (int i = 2; i * i <= item; i++) {
        if (item % i == 0)
            return false;
    }
    return true;
}

