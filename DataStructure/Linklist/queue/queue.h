#ifndef EARABLE_CORE_QUEUE_H
#define EARABLE_CORE_QUEUE_H

#include <stdbool.h>

typedef unsigned char          uint8;

typedef unsigned short         uint16;
typedef unsigned int           uint32;

typedef signed char            int8;
typedef short                  int16;
typedef int                    int32;
/**
 * @brief Queue is implemented over buffer
 * @note We using 4 pointer to create a Ring-Buffer
 * @note This design can be a template with datatype uint8
 * developer can refer it to change data type
 */
typedef struct Queue
{
    uint16 size_;        // Size of Queue
    uint8* first_;      // Pointer is used to point to first member in buffer, fixed position
    uint8* last_;       // Pointer is used to point to last member in buffer, fixed position
    uint8* inPtr_;      // Pointer represents input data to be pushed into the queue
    uint8* outPtr_;     // Pointer represents output data to be poped out the queue
}queue;

/**
 * @brief Queue is implemented over buffer
 * @param *q Pointer refer to a queue instance
 * @param *buf Pointer refer to a buffer to implement queue
 * @param size Size of queue want to init
 * @note This design can be a template with datatype uint8
 * developer can refer it to change data type
 */
void queueInit(queue *q, uint8 *buf, uint16 size);

/**
 * @brief Check if queue is Available
 * @param *q Pointer refer to a queue instance
 * @return amount of data available
 */
uint8 queueIsAvailable(queue *q);

/**
 * @brief Reset a Queue
 * @param *q Pointer refer to a queue instance
 */
void queueReset(queue *q);

/**
 * @brief Push a member to Queue. In this case member is uint8
 * @param *q Pointer refer to a queue instance
 * @param val Value of member
 */
void queuePush(queue *q, uint8 val);

/**
 * @brief pop a member from Queue. In this case member is uint8
 * @param *q Pointer refer to a queue instance
 * @return data type of member. In this case member is uint8
 */
uint8 queuePop(queue *q);

/**
 * @brief Check if queue is Empty
 * @param *q Pointer refer to a queue instance
 * @return TRUE if queue is EMPTY and FALSE if queue NOT EMPTY
 */
bool queueIsEmpty(queue *q);

#endif // EARABLE_CORE_QUEUE_H
