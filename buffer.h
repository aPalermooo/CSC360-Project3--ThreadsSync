/**************************************************************************
 *
 *  Class Name: Buffer.h
 *  Purpose:    A circular buffer that interacts with osproj4.cpp
 *              Stores items that producers generate until a consumer
 *              can process the number
 *  Author:     Xander Palermo <ajp2s@missouristate.edu>
 *  Date:       9 October 2024
 *
 *  Programming Project #3:     Process Synchronization Using Pthreads
 *  Lecture:                    CSC360 - Operating Systems
 *  Instructor:                 Dr. Siming Liu
 *
 *************************************************************************/


#ifndef _BUFFER_H_DEFINED_
#define _BUFFER_H_DEFINED_
#include <iostream>
#include <semaphore.h>

typedef int buffer_item;

#define BUFFER_SIZE (5)
#define NULL_ITEM (-1)
/***************************************************************
 *
 * @brief a circular buffer to store items between two threads
 *
 * A circular buffer that allows threads to store items and access
 * items within to communicate with each other. Supports adding and
 * removing items. When the buffer has cycled through all of its
 * memory locations, it will wrap back to the beginning of its allocated
 * memory.
 *
 *****************************************************************/
class Buffer {

    public:
    // CLASS DATA MEMBERS //
    int size;                               // the number of items in the buffer
    buffer_item buffer[BUFFER_SIZE]{};      // the place where items are stored
    int head;                               // location of the oldest item
    int tail;                               // location of where the next item will go

    sem_t freeMutex{};                      // semaphore that keeps track if a thread is accessing the buffer
    sem_t empty{};                          // semaphore that keeps track if there is something
    sem_t full{};                           // semaphore that keeps track if something can be added to the buffer

    /*****************************************
     * Buffer Constructor
     *
     * @brief constructor for a Buffer
     *
     * creates a buffer object and sets all
     * initial pointer values to 0 and fills
     * the buffer with a value to signify
     * empty (-1)
     *
     ********************************************/
    Buffer() : size(0), head(0), tail(0) {
        for (int i = 0; i < BUFFER_SIZE; i++) { // NOLINT(*-loop-convert)
            buffer[i] = NULL_ITEM;
        }
        sem_init(&freeMutex, 0, 1);
        sem_init(&empty, 0, BUFFER_SIZE);
        sem_init(&full, 0, 0);
    }


    /*****************************************
     * Buffer Insert Item
     *
     * @brief  adds an item to the buffer
     *
     * stores a given integer into the next
     * available buffer slot, but only if the buffer
     * is not full. If the buffer is on its last
     * space of memory, the next value will
     * replace the first memory slot in the buffer,
     * wrapping back to the beginning of the buffer
     * in memory
     *
     * @pre         the buffer is not full
     *
     * @param item  the item to be inserted into the buffer
     *
     * @return      true if the item was successfully inserted into the buffer
     *              false if the item was unable to be inserted into the buffer
     *                  (such as in the case of the buffer being full)
     *
     *****************************************/
    bool buffer_insert_item( buffer_item item ) {
        if (size == BUFFER_SIZE) // the buffer is full
            return false;
        sem_wait(&empty); // If there is room in the buffer
        {
            sem_wait(&freeMutex); // If there is no one is accessing the buffer
            {
                buffer[tail] = item;
                size++;
                tail = (tail + 1) % BUFFER_SIZE;
            }
            sem_post(&freeMutex);
        }
        sem_post(&full);
        return true;

    }


    /*****************************************
    * Buffer Remove Item
    *
    * @brief  removes the oldest item from the buffer
    *
    * removes the oldest integer that was placed into the buffer
    * and replaces it with a null value (-1), but only if the buffer
    * is not empty.
    *
    * @pre          the buffer is not empty
    *
    * @param item   REFERENCE to location the removed item will be stored to
    *
    * @return       true if the item was successfully removed from the buffer
    *               false if the item was unable to be removed from the buffer
    *                   (such as in the case of the buffer being empty)
    *
    *****************************************/
    bool buffer_remove_item( buffer_item *item ) {
        if (size == 0) //the buffer is empty
            return false;

        sem_wait(&full); //If there is something in the buffer
        {
            sem_wait(&freeMutex); //If there is no one accessing the buffer
            {
                *item = buffer[head];
                buffer[head] = NULL_ITEM;
                size--;
                head = (head + 1) % BUFFER_SIZE;
            }
            sem_post(&freeMutex);
        }
        sem_post(&empty);
        return true;
    }
};

#endif // _BUFFER_H_DEFINED_
