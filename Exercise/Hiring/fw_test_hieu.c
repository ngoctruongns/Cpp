#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#define MAX_SIGNAL 20

typedef struct
{
    float weight; /* smoothing factor with value between 0 and 1 */
    float current_ema;
} task_data_t;

static task_data_t task_data[MAX_SIGNAL];

static int app_ExponentialAverage(task_data_t *task, float reading, int signal_index)
{
    if (signal_index < MAX_SIGNAL && signal_index >= 0)
    {
        float weight = task[signal_index].weight;
        task[signal_index].current_ema =
            (1 - weight) * reading + task[signal_index].current_ema * weight;
        return 1;
    }
    else
        return 0;
}

int main(int argc, char **argv)
{

    float signal[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 9, 10};
    task_data[0].current_ema = 0;
    task_data[0].weight = 0.2;
    for (uint8_t i = 0; i < sizeof(signal) / sizeof(signal[0]); i++)
    {
        app_ExponentialAverage(task_data, signal[i], 0);
        printf("ema[%d] = %f\n", i, task_data[0].current_ema);
    }

    float signal2[] = {11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 19, 20};

    printf("The following is incorrect\n");
    task_data[1].weight = task_data[0].weight;
    task_data[1].current_ema = task_data[0].current_ema;
    task_data[2].weight = task_data[0].weight;
    task_data[2].current_ema = task_data[0].current_ema;

    for (uint8_t i = 0; i < sizeof(signal) / sizeof(signal[0]); i++)
    {
        app_ExponentialAverage(task_data, signal[i], 1);
        printf("signal 1\tema[%d] = %f\n", i, task_data[1].current_ema);

        app_ExponentialAverage(task_data, signal2[i], 2);
        printf("signal 2\tema[%d] = %f\n", i, task_data[2].current_ema);
    }
}
