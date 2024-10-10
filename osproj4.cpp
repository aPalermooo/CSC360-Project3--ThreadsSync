/**************************************************************************
 *
 *  Class Name: main.cpp
 *  Purpose:    Explore the classic Producer / Consumer problem that occurs
 *              with competing threads. In this particular case, they are
 *              adding and taking from a small buffer and calculating if
 *              the number is prime.
 *  Author:     Xander Palermo <ajp2s@missouristate.edu>
 *  Date:       9 October 2024
 *
 *  Programming Project #3:     Process Synchronization Using Pthreads
 *  Lecture:                    CSC360 - Operating Systems
 *  Instructor:                 Dr. Siming Liu
 *
 *************************************************************************/

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
#define MAX_RANDOM_NUMBER 100
#define PRODUCER_TAG 1
#define CONSUMER_TAG 2

/**** GLOBAL FLAGS ****/
char verboseMode = 'n';                              //flag for if verbose mode is turned on
static bool running = true;                      //flag for the simulation to turn off
static unsigned int seed = time(nullptr);   //random seed for threads using sleep()

/**** GLOBAL VARS ****/
Buffer buffer = Buffer();                       //buffer used in simulation

int actionsPerformed[MAX_THREADS * 2];          //keeps track of how many times each thread has done its action
atomic<int> countBufferFull;                    //counter for how many times the buffer is full during simulation
atomic<int> countBufferEmpty;                   //counter for how many times the buffer is empty during simulation

//keep track of threads in numberProcess()
sem_t refGeneratorMutex;
int producer_ID(0);
int consumer_ID(0);

/**** FUNCTION HEADERS ****/
//thread functions
void* producer(void *param);
void* consumer(void *param);
int numberProcess(int PROCESS_TYPE);
bool isPrime(buffer_item item);
//display functions
void displayBuffer(const string& TITLE, int head, int tail);
void displayFinalStats (const int &SIMULATION_TIME, const int &MAX_SLEEP_TIME,const int &NUM_PRODUCERS, const int &NUM_CONSUMERS);


/*****************************************
 * main()
 *
 * @brief   begins simulation of threads creating and checking random numbers for whether
 *          or not they are prime
 *
 * Creates a simulation of the Producer / Consumer problem by creating a user set number
 * threads of labeled producers and consumers. The program will then wait for a user set
 * amount of time and terminate the threads, and display the final running statistics of
 * the simulation
 *
 * **parameters are listed in order they are passed in
 * @param int  Run time
 * @param int  Max sleep time of threads
 * @param int  Number of Producers ( MIN:1  MAX:25 )
 * @param int  Number of Consumers ( MIN:1  MAX:25 )
 * @param char 'y' - turns on verbose mode
 *
 * @return       0        successful simulation
 * @return      -1        Invalid Arguments: incorrect number of arguments
 * @return       1        Invalid Arguments: incorrect type / 0 for an argument
 * @return       2        Invalid Arguments: too many threads
 *
 *****************************************/
int main(int argc, char* argv[]) {

    sem_init(&refGeneratorMutex, 0, 1); //initiate mutex to number processes


    ///VALIDATE AND SET USER ARGUMENTS
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
    } //end if

    //formating arguments
    const int MAX_RUN_TIME = atoi(argv[1]);
    const int MAX_SLEEP_TIME = atoi(argv[2]);
    const int NUM_PRODUCERS = atoi(argv[3]);
    const int NUM_CONSUMERS = atoi(argv[4]);
    verboseMode = *argv[5];

    consumer_ID = NUM_PRODUCERS; //consumer actions are saved in memory spaces allocated after producer actions



    if (MAX_RUN_TIME == 0 || MAX_SLEEP_TIME == 0 || NUM_PRODUCERS == 0 || NUM_CONSUMERS == 0) { // if any param is invalid or set to 0 return 1
        printf("%s", invalidArgMsg.c_str());
        return 1;
    } //end if

    if (NUM_PRODUCERS > MAX_THREADS || NUM_CONSUMERS > MAX_THREADS) { // if the user asks for more threads than the program can create return 2
        printf("%s", invalidArgMsg.c_str());
        return 2;
    } //end if


    ///CREATE THREADS FOR SIMULATION
    //init threads
    pthread_t tid[MAX_THREADS];
    pthread_attr_t attr;
    pthread_attr_init(&attr);

    //create and assign threads
    cout<<"Starting threads..."<<endl;

    if (verboseMode == 'y') { //display initial conditions
        displayBuffer("",buffer.head, buffer.tail);
    } //end if

    //creates producer threads
    for (int i = 0; i < NUM_PRODUCERS; i++) {
        pthread_create(&tid[i], &attr, producer, (void*)argv[2]);
    } //end for

    //creates consumer threads
    for (int i = 0; i < NUM_CONSUMERS; i++) {
        pthread_create(&tid[i+NUM_PRODUCERS], &attr, consumer, (void*)argv[2]);
    } //end for


    sleep(MAX_RUN_TIME);
    //wait sim time...
    //...
    //...
    running = false; //signal threads simulation is done

    //rejoin all threads
    int totalThreads = NUM_PRODUCERS + NUM_CONSUMERS;
    for (int i = 0; i < totalThreads; i++) {
        pthread_join(tid[i], nullptr);
    } //end for

    displayFinalStats(MAX_RUN_TIME, MAX_SLEEP_TIME , NUM_PRODUCERS, NUM_CONSUMERS);

    return 0;
} //end main

/*****************************************
* producer()
*
* @brief thread task to generate random numbers and add them
*        to a global buffer
*
* When initated, a thread will identify itself and begin to generate
* random numbers to save to the buffer. If the buffer is full,
* the thread will output that it was unsuccessful, or if verbose
* mode is turned off, do nothing
* A producer will keep track of every time it completes an action
* as well as how many times the buffer is full when it trys to access
* it.
*
* @param param    maximum amount of time the thread will sleep for
*
* @return 0       when signal simulation ends
*****************************************/
void* producer(void *param) {


    //creates variable for max sleeping time from user argument
    int sleepTime;
    const char * argv = static_cast<char *>(param);
    int maxSleepTime = atoi(argv);

    //identify thread
    const int PROCESS_ID = getpid();
    const int REF_ID = numberProcess(PRODUCER_TAG);

    if (verboseMode == 'y') { //it is meaningless to check if verbose is on every time a thread does its action, so it will only check once

        string outputHeader; // describes the threads action for output

        while (running) { //until signalled to stop by main()
            //generates sleep time and waits
            sleepTime = rand_r(&seed) % maxSleepTime;
            sleep(sleepTime);

            //when wakes up, attempts to put a number into the buffer
            buffer_item producedItem = rand() % MAX_RANDOM_NUMBER; //random number to be put in buffer

            if (!buffer.buffer_insert_item(producedItem)) { //buffer full
                countBufferFull++;
                printf("All buffers full. Producer %d waits.\n\n", PROCESS_ID);
                continue; //unsuccessful
            } //end if

            //successful in putting an item in the buffer
            outputHeader = "Producer " + to_string(PROCESS_ID) + " writes " + to_string(producedItem);
            actionsPerformed[REF_ID]++;
            displayBuffer(outputHeader,buffer.head, buffer.tail);
        } //end while
    } //end if

    else { //verbose mode off - identical, but removes printf statements and related calls

        while (running) { //until signalled to stop by main()
            //generates sleep time and waits
            sleepTime = rand_r(&seed) % maxSleepTime;
            sleep(sleepTime);

            //when wakes up, attempts to put a number into the buffer
            buffer_item producedItem = rand() % 100; //random number to be put in buffer

            if (!buffer.buffer_insert_item(producedItem)) { //buffer full
                countBufferFull++;
                continue; //unsuccessful
            } //end if

            //successful in putting an item in the buffer
            actionsPerformed[REF_ID]++;
        } //end while
    } //end else
    pthread_exit(nullptr);
} //end producer

/*****************************************
* consumer
*
* @brief thread task to generate to take numbers from the
*        buffer and optionally calculate if they are prime
*
* When initiated, a thread will identify itself and begin to
* take items from the buffer. If the buffer is empty, the thread
* will output it was unsuccessful, if verbose mode is turned off.
* If verbose mode is turned on, it will also alert the user if
* the item the consumer pulled is a prime number.
* A consumer will keep track of every time it completes an action
* as well as how many times the buffer is empty when it trys to access
* it.
*
* @param param  maximum amount of time the thread will sleep for
*
* @return 0     when signal simulation ends
*****************************************/
void *consumer(void *param) {

    //creates variable for max sleeping time from user argument
    int sleepTime;
    const char * argv = static_cast<char *>(param);
    int maxSleepTime = atoi(argv);

    //identify thread
    const int PROCESS_ID = getpid();
    const int REF_ID = numberProcess(CONSUMER_TAG);

    buffer_item consumedItem; //item the consumer pulls from the buffer

    if (verboseMode == 'y') { //it is meaningless to check if verbose is on every time a thread does its action, so it will only check once

        string outputHeader; // describes the threads action for output

        while (running) { //until signalled to stop by main()
            //generates sleep time and waits
            sleepTime = rand_r(&seed) % maxSleepTime;
            sleep(sleepTime);


            if (!buffer.buffer_remove_item(&consumedItem)) { //buffer is empty
                countBufferEmpty++;
                printf("All buffers empty. Consumer %d waits.\n\n", PROCESS_ID);
                continue; //unsuccessful

            } else { //consumer pulled item from buffer
                //calculate if number is prime
                if (isPrime(consumedItem))
                    outputHeader = "Consumer " + to_string(PROCESS_ID) + " reads " + to_string(consumedItem) + "\t*****PRIME NUMBER*****";
                else
                    outputHeader = "Consumer " + to_string(PROCESS_ID) + " reads " + to_string(consumedItem);

                //successful in removing an item from the buffer
                actionsPerformed[REF_ID]++;
            } //end else

            displayBuffer(outputHeader,buffer.head, buffer.tail);

        } //end while

    } else { //verbose mode off - identical, but removes printf statements and related calls
        while (running) { //until signalled to stop by main()
            //generates sleep time and waits
            sleepTime = rand_r(&seed) % maxSleepTime;
            sleep(sleepTime);

            if (!buffer.buffer_remove_item(&consumedItem)) { //buffer full
                countBufferEmpty++;
                continue; //unsuccessful
            } else { //consumer pulled item from buffer
                //successful in removing an item from the buffer
                actionsPerformed[REF_ID]++;
            } //end else
        } //end while
    } //end if
    pthread_exit(nullptr);
} //end consumer


/*****************************************
 * numberProcess()
 *
 * @brief identifies the thread calling it by
 *        giving it a unique ID to its type
 *
 * Processes that give 1 as an argument identify as a producer
 * Processes that give 2 as an argument identify as a consumer
 * For each type the function will issue a unique ID for the
 * thread type, but are not unique to the other type.
 *
 *
 * @param PROCESS_TYPE Identifies process type
 *
 * @return ref_ID the identifying number of the process within its type
 *****************************************/
int numberProcess(const int PROCESS_TYPE) {


    sem_wait(&refGeneratorMutex);       // start critical
        int ref_ID = -1; //pessimistic
        if (PROCESS_TYPE == PRODUCER_TAG) { //thread identifies as producer
            ref_ID = producer_ID;
            producer_ID += 1;
        } //end if

        if (PROCESS_TYPE == CONSUMER_TAG) { //thread identifies as consumer
            ref_ID = consumer_ID;
            consumer_ID += 1;
        } //end if
    sem_post(&refGeneratorMutex);      // end critical
    return ref_ID;
} //end numberProcess


/*****************************************
 * displayBuffer()
 *
 * @brief when verbose mode is on, is called to display the
 *        current state of the buffer
 *
 * When verbose mode is turned on, this function is summoned after
 * every action that is performed on the buffer to log the changes
 * made to it.
 * Will display the last action performed on the buffer, the number
 * of memory spaces currently occupied in the buffer, the current
 * values stored in the buffer, and the location of the next read
 * and write actions to happen to the buffer
 *
 * @pre there exists a buffer with a size of 5
 *
 * @param TITLE **REFERENCE** the description of the last action performed on the buffer
 * @param head Location of where the next read action will occur
 * @param tail Location of where the next write action will occur
 *
 * @return void
 *****************************************/
void displayBuffer(const string &TITLE, const int head, const int tail) {
    //LOCATE WHERE HEAD AND TAIL ARE IN THE BUFFER
    int i= 0 ;
    const string SPACE = "        ";
    string pointerLocations;
    if (head > tail) { // W is before R
        for (i = 0; i < tail; i++) {
            pointerLocations.append(SPACE);
        } //end for
        pointerLocations.append("W");
        while (i < head) {
            pointerLocations.append(SPACE);
            i++;
        } //end while
        pointerLocations.append("R");
    } else { //R is before W
        for (i = 0; i < head; i++) {
            pointerLocations.append(SPACE);
        } //end for
        pointerLocations.append("R");
        while (i < tail) {
            pointerLocations.append(SPACE);
            i++;
        } //end while
        pointerLocations.append("W");

    } //end else

    //DISPLAY TO CONSOLE
    printf("%s\n"
        "(buffers occupied: %d)\n"
           "buffers:\t%5d\t%5d\t%5d\t%5d\t%5d\n"
           "\t\t\t  ----    ----    ----    ----    ----\n"
           "\t\t\t   %s\n\n"
           ,TITLE.c_str(),buffer.size,buffer.buffer[0],buffer.buffer[1],buffer.buffer[2],buffer.buffer[3],buffer.buffer[4]
           ,pointerLocations.c_str());
} //end displayBuffer

/*****************************************
 * displayFinalStats()
 *
 * @brief when the simulation is done, is invoked by main() to
 *        display important and/or notable statistics about the
 *        specific instance of the simulation
 *
 * When the simulation is over, will print the following items to the console:
 *      -Simulation Time
 *      -Maximum Sleep Time of a Thread
 *      -Number of Producers
 *      -Number of Consumers
 *      -Size of the Buffer
 *      -Number of Items Produced
 *      -How many items each thread produced (that was a producer)
 *      -Number of Items consumed
 *      -How many items each thread consumed (that was a consumer)
 *      -Number of items left in the buffer when the simulation terminated
 *      -Number of times the buffer was empty when a consumer tried to access it during the simulation
 *      -Number of times the buffer was full when a producer tried to access it during the simulation
 *
 * @pre the simulation has completed
 *
 * @param SIMULATION_TIME **REFERENCE** the user defined amount of time the simulation ran for
 * @param MAX_SLEEP_TIME  **REFERENCE** the user defined amount of time threads are allowed to sleep for
 * @param NUM_PRODUCERS   **REFERENCE** the user defined amount of Producers existed in the simulation
 * @param NUM_CONSUMERS   **REFERENCE** the user defined amount of Consumers existed in the simulation
 *
 * @return void
 *****************************************/
void displayFinalStats (const int &SIMULATION_TIME, const int &MAX_SLEEP_TIME, const int &NUM_PRODUCERS, const int &NUM_CONSUMERS) {
    //SUM TOTAL OF PRODUCED AND CONSUMED ITEMS
    int totalProduced (0), totalConsumed (0);
    int index;
    for (index = 0; index < NUM_PRODUCERS; index++) {
        totalProduced += actionsPerformed[index];
    } //end for
    for (index = 0; index < NUM_CONSUMERS; index++) {
        totalConsumed += actionsPerformed[index+NUM_PRODUCERS];
    } //end for

    //FORMAT DISPLAY MESSAGE
    string finalMessage =   "PRODUCER / CONSUMER SIMULATION COMPLETE \n"
                            "========================================\n"
                            "Simulation Time:\t\t\t\t\t\t" + to_string(SIMULATION_TIME) + "\n"
                            "Maximum Thread Sleep Time:\t\t\t\t" + to_string(MAX_SLEEP_TIME) + "\n"
                            "Number of Producer Threads:\t\t\t\t" + to_string(NUM_PRODUCERS) + "\n"
                            "Number of Consumer Threads:\t\t\t\t" + to_string(NUM_CONSUMERS) + "\n"
                            "Size of Buffer:\t\t\t\t\t\t\t" + to_string(buffer.size) + "\n"
                            "\n"
                            "Total Number of Items Produced:\t\t\t" + to_string(totalProduced) + "\n";
    for (index = 0; index < NUM_PRODUCERS; index++) {
        finalMessage +=     "\tThread " + to_string(index) + ":\t\t\t\t\t\t\t" + to_string(actionsPerformed[index]) + "\n";
    } //end for
    finalMessage +=         "\n"
                            "Total Number of Items Consumed:\t\t\t" + to_string(totalConsumed) + "\n";
    for (; index < NUM_CONSUMERS + NUM_PRODUCERS; index++) {
        finalMessage +=     "\tThread " + to_string(index) + ":\t\t\t\t\t\t\t" + to_string(actionsPerformed[index]) + "\n";
    } //end for

    finalMessage +=        "\n"
                           "Number Of Items Remaining in Buffer:\t" + to_string(buffer.size) + "\n"
                           "Number Of Times Buffer was Full:\t\t" + to_string(countBufferFull) + "\n"
                           "Number Of Times Buffer was Empty:\t\t" + to_string(countBufferEmpty) + "\n";

    //DISPLAY
    cout << finalMessage;
} //end displayFinalStats

/*****************************************
 * isPrime
 *
 * @brief evaluates if the given item is prime
 *
 * The program cycles through potential factors of
 * a number until it gets to the square root of the
 * number, which can be used in the proof that the
 * number is prime or not
 *
 * ** This function was referenced from the article:
 * https://www.geeksforgeeks.org/c-program-to-check-prime-number/
 * **
 *
 * @param item the integer to be distinguished if it is prime or not
 *
 * @return true     if item is prime
 * @return false    if item is not prime
 *
 *****************************************/
bool isPrime(buffer_item item) {
    if (item == 1 || item == 0) return false;
    for (int i = 2; i * i <= item; i++) {
        if (item % i == 0)
            return false;
    } //end for
    return true;
}

