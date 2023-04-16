#ifdef __cplusplus
extern "C"
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

    typedef struct
{
    uint8_t task_data_1;
    uint8_t task_data_2;

    struct
    {
        /* smoothing factor with value between 0 and 1 */
        float weight;
        float current_ema;
        float state; // state for ema
    } average;
} task_data_t;

static task_data_t task_data, task_data2;

static float app_ExponentialAverage(task_data_t *task, float reading)
{
    // static float state = 0;

    float output = task->average.weight * task->average.state;
    output += (1 - task->average.weight) * reading;

    task->average.state = output;
    return output;
}

int main(int argc, char **argv)
{

    float signal[] = {
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 9, 10};

    task_data.average.weight = 0.2;
    task_data.average.state = 0;

    for (uint8_t i = 0; i < sizeof(signal) / sizeof(signal[0]); i++)
    {
        task_data.average.current_ema = app_ExponentialAverage(&task_data, signal[i]);

        printf("ema[%d] = %f\n\r", i, task_data.average.current_ema);
    }

    float signal2[] = {
        11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 19, 20};

    // printf("The following is incorrect\n\r");
    printf("The following is correct after fix:\n\r");
    task_data.average.state = 0;
    task_data2.average.weight = 0.2;
    task_data2.average.state = 0;

    for (uint8_t i = 0; i < sizeof(signal) / sizeof(signal[0]); i++)
    {
        task_data.average.current_ema = app_ExponentialAverage(&task_data, signal[i]);
        printf("signal 1\tema[%d] = %f\n\r", i, task_data.average.current_ema);

        task_data2.average.current_ema = app_ExponentialAverage(&task_data2, signal2[i]);
        printf("signal 2\tema[%d] = %f\n\r", i, task_data2.average.current_ema);
    }
}

int offsetNum(int num) // Allocate memory for an integer variable named num on the stack
{
    int offset = 10;     // Allocate memory for an integer variable named offset on the stack
    return num + offset; // Return the result of adding num and offset
}
