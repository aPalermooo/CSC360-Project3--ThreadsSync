//
// Created by cinnamon on 10/1/24.
//

#include "buffer.h"
#include <pthread.h>
#include <semaphore.h>
#include <iostream>
#include <unistd.h>
#include <random>
using namespace std;

#define MAX_THREADS 10


//TEMP
const int NUM_PRODUCERS = 5;
const int NUM_CONSUMERS = 5;

void* producer(void* arg);
void* consumer(void* arg);
int displayBuffer(int condition, int head, int tail);
bool isPrime(buffer_item item);

sem_t displayMutex;

unsigned int seed = time(NULL);
Buffer buffer = Buffer();

int main() {
    sem_init(&displayMutex, 0, 0);

    //init threads
    pthread_t tid[MAX_THREADS];
    pthread_attr_t attr;
    pthread_attr_init(&attr);

    //create and assign threads
    cout<<"Starting threads..."<<endl;
    displayBuffer(0,buffer.head, buffer.tail);
    for (int i = 0; i < NUM_PRODUCERS; i++) {
        // cout<<"Creating producer:"<<i<<endl;
        pthread_create(&tid[i], &attr, producer, 0);
    }
    for (int i = 0; i < NUM_CONSUMERS; i++) {
        // cout<<"Creating Consumer:"<<i<<endl;
        pthread_create(&tid[i+NUM_PRODUCERS], &attr, consumer, 0);
    }

    //rejoin all threads
    for (int i = 0; i < MAX_THREADS; i++) {
        // cout<<"Joining thread:"<<i<<endl;
        pthread_join(tid[i], 0);
    }
    // buffer.display();

    cout<<"Done"<<endl;
    return 0;
}

void* producer(void*args) {
    int sleepTime;

    sleepTime = rand_r(&seed) % 3;
    sleep(sleepTime);


    buffer_item producedItem = rand_r(&seed) % 100;
    sem_wait(&displayMutex);
    printf("producer produced %d\n",producedItem);
    if (!buffer.buffer_insert_item(producedItem)) {
        printf("Full");
    }
    displayBuffer(1,buffer.head, buffer.tail);
    pthread_exit(0);
}

void *consumer(void *arg) {
    buffer_item consumedItem;
    int sleepTime;

    sleepTime = rand_r(&seed) % 3;
    sleep(sleepTime);

    sem_wait(&displayMutex);
    if (!buffer.buffer_remove_item(&consumedItem)) {
        printf("Empty\n");
    } else {
        if (isPrime(consumedItem))
            printf("consumer consumed %d\t*****PRIME NUMBER*****\n", consumedItem);
        else
        printf("consumer consumed %d \n", consumedItem);
    }
    displayBuffer(2,buffer.head, buffer.tail);
    pthread_exit(0);
}

int displayBuffer(const int condition, const int head, const int tail) {
    {
        printf("(buffers occupied: %d)\n"
               "buffers:\t%5d\t%5d\t%5d\t%5d\t%5d\n"
               "\t\t\t  ----    ----    ----    ----    ----\n"
               "\t\t\t   "
               ,buffer.size,buffer.buffer[0],buffer.buffer[1],buffer.buffer[2],buffer.buffer[3],buffer.buffer[4]);

        int i= 0 ;
        const char* spacer = "        ";
        if (head > tail) {
            for (i = 0; i < tail; i++) {
                printf(spacer);
            }
            printf("W");
            while (i < head) {
                printf(spacer);
                i++;
            }
            printf("R\n");
        } else {
            for (i = 0; i < head; i++) {
                printf(spacer);
            }
            printf("R");
            while (i < tail) {
                printf(spacer);
                i++;
            }
            printf("W\n");
        }
        printf("\n");
    }
    sem_post(&displayMutex);
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