#include <stdio.h>
typedef struct Machine machine;
typedef void (*machineState)(machine*);
struct Machine
{
    /**
     * @brief pointer to track current State
     */
    machineState currentState_;
    int index;
}; 

void machineStateAdd(machine *t)
{
    t->index ++;

}

int main() {
    machine *mac;
    mac->currentState_ = NULL;
    mac->index = 0;
    printf("index: %d \n", mac->index);
    
    mac->currentState_ = (machineState)machineStateAdd;
    mac->currentState_(mac);    
    printf("index: %d \n", mac->index);
    return 0;
}