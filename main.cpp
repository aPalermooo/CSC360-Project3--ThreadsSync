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
using namespace std;

#define MAX_THREADS 25
pthread_t tid[MAX_THREADS];

bool VERBOSE_MODE_ON = false;
static bool running = true;
static int maxSleepTime = 0;

int itemsProduced[MAX_THREADS];
int itemsConsumed[MAX_THREADS];
int countBufferFull;
int countBufferEmpty;

Buffer buffer = Buffer();


void* producer(void* arg);
void* consumer(void* arg);
int displayBuffer(int condition, int head, int tail);
bool isPrime(buffer_item item);

int id;
int numberThread();

unsigned int seed = time(nullptr);




/*
 * 1 = Run time
 * 2 = Max Sleep time
 * 3 = number of producers
 * 4 = number of threads
 * 5 = yes or no verbose
 */
int main(int argc, char* argv[]) {


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

    const char VERBOSE = *argv[5];
    if (VERBOSE == 'y')
        VERBOSE_MODE_ON = true;

    //init threads

    pthread_attr_t attr;
    pthread_attr_init(&attr);

    //create and assign threads
    cout<<"Starting threads..."<<endl;

    if (VERBOSE_MODE_ON) {
        displayBuffer(0,buffer.head, buffer.tail);
    }
    for (int i = 0; i < NUM_PRODUCERS; i++) {
        pthread_create(&tid[i], &attr, producer, 0);
    }
    for (int i = 0; i < NUM_CONSUMERS; i++) {
        pthread_create(&tid[i+NUM_PRODUCERS], &attr, consumer, 0);
    }

    //rejoin all threads
    int totalThreads = NUM_PRODUCERS + NUM_CONSUMERS;
    for (int i = 0; i < totalThreads; i++) {
        pthread_join(tid[i], 0);
    }

    cout<<"Done"<<endl;
    return 0;
}

void* producer(void *args) {
    int sleepTime;
    printf("Hi i'm pro.%i\n", numberThread());
    if (VERBOSE_MODE_ON) { //it is expensive to check if verbose is on every time a thread runs, so it will only check once
        sleepTime = rand_r(&seed) % 3;
        sleep(sleepTime);


        buffer_item producedItem = rand() % 100;
            printf("producer produced %d\n",producedItem);
            if (!buffer.buffer_insert_item(producedItem)) {
                printf("Full");
            }
            displayBuffer(1,buffer.head, buffer.tail);
    }
    else {
        sleepTime = rand_r(&seed) % 3;
        sleep(sleepTime);


        buffer_item producedItem = rand() % 100;
            if (!buffer.buffer_insert_item(producedItem)) {
            }
        }
    pthread_exit(0);
}

void *consumer(void *arg) {
    buffer_item consumedItem;
    printf("Hi i'm con.%i\n", numberThread());
    int sleepTime;

    if (VERBOSE_MODE_ON) {
        sleepTime = rand_r(&seed) % 3;
        sleep(sleepTime);

        if (!buffer.buffer_remove_item(&consumedItem)) {
            printf("Empty\n");
        } else {
            if (isPrime(consumedItem))
                printf("consumer consumed %d\t*****PRIME NUMBER*****\n", consumedItem);
            else
                printf("consumer consumed %d \n", consumedItem);
        }
        displayBuffer(2,buffer.head, buffer.tail);
    } else {

        sleepTime = rand_r(&seed) % 3;
        sleep(sleepTime);

        if (!buffer.buffer_remove_item(&consumedItem)) {

        } else {
            if (isPrime(consumedItem));

            else;

        }
    }
    pthread_exit(0);
}
int numberThread() {
    return id++;
}

int displayBuffer(const int condition, const int head, const int tail) {

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
    printf("(buffers occupied: %d)\n"
           "buffers:\t%5d\t%5d\t%5d\t%5d\t%5d\n"
           "\t\t\t  ----    ----    ----    ----    ----\n"
           "\t\t\t   %s\n\n"
           ,buffer.size,buffer.buffer[0],buffer.buffer[1],buffer.buffer[2],buffer.buffer[3],buffer.buffer[4]
           ,pointerLocations.c_str());
    return 0;
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