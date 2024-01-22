#include <stdio.h>
#include <stdint.h>

typedef struct example_struct
{
  uint8_t  member1;
  uint32_t member2;
} ExampleStruct_t;

typedef struct example_struct1
{
  uint8_t  member1;
  uint32_t member2;
  uint16_t member3;
} NewStruct_t;

typedef struct 
{
  uint32_t member2;
  uint8_t  member1;
  uint16_t member3;
  uint8_t  member4;
} CleanStruct_t;

typedef struct 
{
  uint32_t member2;
  uint8_t  member1;
  uint8_t  member4;
  uint16_t member3;
} BestStruct_t;

int main(void) 
{
  printf("Size of example struct: %lu bytes\n", sizeof(ExampleStruct_t));
  printf("Size of example struct: %lu bytes\n", sizeof(NewStruct_t));
  printf("Size of example struct: %lu bytes\n", sizeof(CleanStruct_t));
  printf("Size of example struct: %lu bytes\n", sizeof(BestStruct_t));
  
  return 0;
}
