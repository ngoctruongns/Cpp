#include <stdio.h>
#include "type.h"

int main()
{
    uint32 sent_samples[4];
    uint32 total_samples[4];
    uint8 module_id, rate;

    module_id = 0;
    sent_samples[module_id] = 6800;
    total_samples[module_id] = 18950;

    if (sent_samples[module_id] > 0)
    {
        rate = (200 *(total_samples[module_id] -  sent_samples[module_id])) / total_samples[module_id];
    }
    printf("Rate = %d \n", rate);

    uint64_t dataPayload = 0;
    printf("size of uint64_t is %zu bytes \n", sizeof(uint64_t));
    uint8 data_loss_get_rate[4] = {04, 12, 00, 127}; // 2D 4C 48 80
    for (uint8 i = 0; i < 4; i++) 
    {
        dataPayload |= data_loss_get_rate[i] << 8*i;
        printf("Data loss %d: %llx \n", i, dataPayload);
    }
    printf("Data loss 1: %llx \n", dataPayload);
    dataPayload *= 1000;
    printf("Data loss 2: %llu \n", dataPayload);
    printf("Data loss rate: %llx \n", (uint32)(dataPayload / 1000));
    return 0;
}