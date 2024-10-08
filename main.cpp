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

sem_t display;

unsigned int seed = time(NULL);
Buffer buffer = Buffer();

int main() {
    sem_init(&display, 0, 1);

    //init threads
    pthread_t tid[MAX_THREADS];
    pthread_attr_t attr;
    pthread_attr_init(&attr);

    //create and assign threads
    // cout<<"Creating thread:"<<endl;
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
    buffer.display();

    cout<<"Done"<<endl;
    return 0;
}

void* producer(void*args) {
    int sleepTime;

    sleepTime = rand_r(&seed) % 3;
    sleep(sleepTime);


    buffer_item producedItem = rand() % 100;
    printf("producer produced %d\n",producedItem);
    if (!buffer.buffer_insert_item(producedItem)) {
        printf("Full");
    }
    pthread_exit(0);
}

void *consumer(void *arg) {
    buffer_item consumedItem;
    int sleepTime;

    sleepTime = rand_r(&seed) % 3;
    sleep(sleepTime);

    if (!buffer.buffer_remove_item(&consumedItem)) {
        printf("Empty\n");
    } else {
        printf("consumer consumed %d \n", consumedItem);
    }

    pthread_exit(0);
}
