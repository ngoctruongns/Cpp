#include "queue.h"
#include <stdio.h>


#define MAX_SIZE 5
uint8 QUEUE_TEST_ARR[MAX_SIZE];

void queueInit(queue *q, uint8 *buf, uint16 size)
{
    q->first_ = buf;
    q->size_ = size;
    q->last_ = q->first_ + size;
    q->inPtr_ = q->first_;
    q->outPtr_ = q->first_;
}

uint8 queueIsAvailable(queue *q)
{
    uint8 ret = q->size_ + q->outPtr_ - q->inPtr_;
    if (ret > q->size_) ret -= q->size_;
    return ret;
}

uint16 queueGetCount(queue *q) {
    if (q->inPtr_ >= q->outPtr_) {
        return q->inPtr_ - q->outPtr_;
    } else {
        return (q->last_ - q->outPtr_) + (q->inPtr_ - q->first_);
    }
}

bool queueIsEmpty(queue *q)
{
    return (q->inPtr_ == q->outPtr_);
}

void queueReset(queue *q)
{
    q->inPtr_ = q->first_;
    q->outPtr_ = q->first_;
}

void queuePush(queue *q, uint8 val)
{
    uint8* next = q->inPtr_ + 1;
    if (next == q->last_) next = q->first_;
    if (next!= q->outPtr_) //queue not full
    {
        *(q->inPtr_) = val;
        q->inPtr_ = next;
    }
    else
    {
        printf("Queue if full, not push more. \n");
    }
}

uint8 queuePop(queue *q)
{
    if (q->outPtr_ != q->inPtr_)
    {
        uint8 ret = *(q->outPtr_);
        q->outPtr_++;
        if (q->outPtr_ == q->last_) q->outPtr_ = q->first_;
        return ret;
    }
    return 0;
}

void queueDisplay(queue *q) {
    if (queueIsEmpty(q)) {
        printf("Queue is empty.\n");
        return;
    }

    printf("Queue elements: ");
    uint8 *current = q->outPtr_;
    while (current != q->inPtr_) {
        printf("%d ", *current);
        current++;
        if (current == q->last_) {
            current = q->first_;
        }
    }
    printf("\n");
}

int main()
{
    queue *queue1;
    queueInit(queue1, QUEUE_TEST_ARR, MAX_SIZE);
    printf("Size of queue type: %d \n", sizeof(queue1));
    // printf("queue type: %p \n", queue1->first_);
    // printf("queue type: %p \n", queue1->last_);
    // printf("queue type: %p \n", queue1->inPtr_);
    // printf("queue type: %p \n", queue1->outPtr_);
    // printf("queue type: %d \n", queue1->size_);

    queuePush(queue1, 1);
    queuePush(queue1, 2);
    queuePush(queue1, 3);
    queuePush(queue1, 4);
    queuePush(queue1, 5);
    queuePush(queue1, 6);

    queueDisplay(queue1); // In ra: Queue elements: 1 2 3

    queuePop(queue1);

    queueDisplay(queue1); // In ra: Queue elements: 2 3

    queuePush(queue1, 7);
    queuePush(queue1, 8);

    queueDisplay(queue1); // In ra: Queue elements: 2 3 4

    return 0;


    

    return 0;
}