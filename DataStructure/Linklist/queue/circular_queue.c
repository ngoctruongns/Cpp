#include <stdio.h>

#define MAX_SIZE 5

typedef struct
{
    int data[MAX_SIZE];
    int front;
    int rear;
} CircularQueue;

void initializeQueue(CircularQueue *queue)
{
    queue->front = -1;
    queue->rear = -1;
}

int isQueueFull(CircularQueue *queue)
{
    if ((queue->front == 0 && queue->rear == MAX_SIZE - 1) || (queue->rear == (queue->front - 1) % (MAX_SIZE - 1)))
    {
        return 1;
    }
    return 0;
}

int isQueueEmpty(CircularQueue *queue)
{
    if (queue->front == -1)
    {
        return 1;
    }
    return 0;
}

void enqueue(CircularQueue *queue, int item)
{
    if (isQueueFull(queue))
    {
        printf("Queue is full. Cannot enqueue.\n");
        return;
    }
    if (queue->front == -1)  // queue empty
    {
        queue->front = queue->rear = 0;
    }
    else if (queue->rear == MAX_SIZE - 1 && queue->front != 0) // at end will back if queue not full
    {
        queue->rear = 0;
    }
    else  
    {
        queue->rear++;
    }
    queue->data[queue->rear] = item;
}

int dequeue(CircularQueue *queue)
{
    if (isQueueEmpty(queue))
    {
        printf("Queue is empty. Cannot dequeue.\n");
        return -1;
    }
    int item = queue->data[queue->front];
    if (queue->front == queue->rear)
    {
        queue->front = queue->rear = -1;
    }
    else if (queue->front == MAX_SIZE - 1)
    {
        queue->front = 0;
    }
    else
    {
        queue->front++;
    }
    return item;
}

void displayQueue(CircularQueue *queue)
{
    if (isQueueEmpty(queue))
    {
        printf("Queue is empty.\n");
        return;
    }
    printf("Queue: ");
    if (queue->rear >= queue->front)
    {
        for (int i = queue->front; i <= queue->rear; i++)
        {
            printf("%d ", queue->data[i]);
        }
    }
    else
    {
        for (int i = queue->front; i < MAX_SIZE; i++)
        {
            printf("%d ", queue->data[i]);
        }
        for (int i = 0; i <= queue->rear; i++)
        {
            printf("%d ", queue->data[i]);
        }
    }
    printf("\n");
}

int main()
{
    CircularQueue queue;
    initializeQueue(&queue);

    enqueue(&queue, 1);
    enqueue(&queue, 2);
    enqueue(&queue, 3);
    enqueue(&queue, 4);
    enqueue(&queue, 5);
    enqueue(&queue, 6);

    displayQueue(&queue);

    dequeue(&queue);
    dequeue(&queue);
    dequeue(&queue);
    dequeue(&queue);
    displayQueue(&queue);
    dequeue(&queue);

    displayQueue(&queue);

    enqueue(&queue, 6);
    enqueue(&queue, 7);

    displayQueue(&queue);

    return 0;
}
