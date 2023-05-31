#include <iostream>
using namespace std;

typedef unsigned char          uint8;

typedef unsigned short         uint16;
typedef unsigned int           uint32;

typedef signed char            int8;
typedef short                  int16;
typedef int                    int32;

typedef uint16 MessageId;
/*!
Message delay type.
*/
typedef uint32 Delay;
/*!
Message type.
*/
typedef const void *Message;
/*!
Task type.
*/
typedef struct TaskData *Task;
/*!
TaskData type.
*/
typedef struct TaskData { void (*handler)(Task, MessageId, Message);} TaskData;

typedef struct
{
    TaskData task;
    uint16 change;
} ToggleTask;
int add( int x, int y)
{
    return (x+y);
}

static void MyHandler(Task t, MessageId id, Message payload)
{
	/* id and payload unused and both 0 */
	uint16 change = ((ToggleTask *) t)->change;
    cout << "Call handler" << endl;
    cout << "Change: " << change << endl;
}
static ToggleTask toggle = { { MyHandler }, 15 };

int main()
{
    TaskData abc;
    Task ptr;

    abc.handler = &MyHandler;
    ptr = &abc;
    int (*ptr1)(int, int);
    ptr1 = &add;
    ptr1(5,6);
    cout << "ptr1: " << ptr1 << endl;
    cout << "*ptr1: " << (*ptr1) << endl;
    int *pint ; 
    cout << "size of pointer: " << sizeof(pint) << endl;
    cout << "*ptr1: " << *ptr1 << endl;


    cout << "ptr: " << ptr << endl;
    cout << "&ptr: " << &ptr << endl;

    cout << "&abc: " << &abc << endl;
    cout << "&abc.handler: " << &abc.handler << endl;
    cout << "abc.handler: "<< abc.handler << endl;
    cout << "&MyHandler: "<< &MyHandler << endl;

    cout << "&toggle: "<< &toggle << endl;
    cout << "&toggle.task: "<< &toggle.task << endl;
    cout << "&toggle.change: "<< &toggle.change << endl;
    MyHandler(&toggle.task, 0, 0);

    return 0;
}