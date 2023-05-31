
/*
@startuml
activate AMP_OFF
AMP_OFF -> GPIO_EXP: Enable AUDIO_AMPL_PWR_EN
AMP_OFF -> GPIO_EXP: Enable AUDIO_AMP_SDZ
activate GPIO_EXP
GPIO_EXP --> AMP_OFF: ASYNC_MESSAGE_ALL_DONE
deactivate GPIO_EXP
AMP_OFF -> AMP_OFF: AMP_POR_TIMEOUT
deactivate AMP_OFF
AMP_OFF -> I2C: 
activate AMP_OFF

@enduml
*/

/*
@startmindmap
* Type
** Basic
*** MessageId
**** uint16
*** Delay
**** uint32
*** *Message
**** void
*** MessageQueue
**** uintptr
** *Task
*** TaskData
**** (*handler)(Task, MessageId, Message)
@endmindmap
*/

/*
@startmindmap
* llist_t
** *head
*** llist_node_t
**** LLIST_NODE
***** *PREV
****** LLIST_NODE
***** *NEXT
****** LLIST_NODE
***** *item
****** void
** *tail
*** llist_node_t
**** LLIST_NODE
***** *PREV
****** LLIST_NODE
***** *NEXT
****** LLIST_NODE
***** *item
****** void

@endmindmap
*/

/*
@startmindmap

* Base
** machine
*** Machine
**** nextState_
***** machineState
**** currentState_
***** machineState
**** machineHandler_
***** TaskData
**** nextEvent_
***** uint32
** (*machineState)(machine*)

@endmindmap
*/

/*
@startmindmap
* async_helper
** async_item_t
*** task
**** TaskData
*** id
**** MessageId
*** payload
**** Message
*** done
**** bool
** *async_items_t
*** llist_t
** task_pair_t
*** self
**** TaskData
*** caller
**** Task
** async_item_private_t
*** task
**** Task
*** handles
**** llist_t
*** count_total
**** uint8
*** count_done
**** uint8
@endmindmap
*/

/*
@startmindmap
* timer
** mTimer
*** id
**** MessageId
*** interval_
**** uint32
*** loop_
**** int32
*** timerAbstractHandler_
**** TaskData
*** handler_
**** handlerTimer
***** (*handlerTimer)(void)

@endmindmap
*/