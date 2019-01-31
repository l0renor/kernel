#include "kernel_functions.h"
#include <limits.h>
extern void __save_context(void);
extern void LoadContext(void);
extern void SelectPSP(void);
extern void __get_PSP_from_TCB(void);

TCB *RunningTask;
bool in_startup;
list *readyList;
list *blockedList; 
list *sleepList;

void run(void);

list * createList(){
  list *newList =  calloc(1,sizeof(list));
  newList->pHead = calloc(1,sizeof(listobj));
  newList->pTail = calloc(1,sizeof(listobj));
  //Head
  newList->pHead->pTask = NULL;
  newList->pHead->nTCnt = 0;
  newList->pHead->pMessage = NULL;
  newList->pHead->pPrevious = NULL;
  newList->pHead->pNext = newList->pTail;
  //Tail
  newList->pTail->pTask = NULL;
  newList->pTail->nTCnt = 0;
  newList->pTail->pMessage = NULL;
  newList->pTail->pPrevious = newList->pHead;
  newList->pTail->pNext = NULL;
  return newList;
}


exception create_task( void(* body)(), uint d )
{
  //Allocate memory for TCB
  isr_off(); /* protextion of calloc */
  ptr_tcb = (TCB *)calloc(1,sizeof(TCB));
  if ( ptr_tcb == NULL ) {
    //F: perhaps a real solution in the future...
    return FAIL;
  }
  
  //Set deadline in TCB
  ptr_tcb->DeadLine = d;
  
  //Set the TCB's PC to point to the task body
  ptr_tcb->PC = body;
  
  //Set TCB's SP to point to the stack segment
  //First element on stack is omitted for safety reasons
  ptr_tcb->StackSeg[STACK_SIZE-2] = 0x21000000; /* PSR */
  ptr_tcb->StackSeg[STACK_SIZE-3] = (uint)body; /* PC */
  ptr_tcb->StackSeg[STACK_SIZE-4] = 0; /* LR */
  //Skip some Stack elements for r12, r3, r2, r1, r0.
  ptr_tcb->SP = &( ptr_tcb->StackSeg[STACK_SIZE-9] );
    
  //IF start-up mode THEN
  if ( in_startup )
  {
    //Insert new task in Readylist
    p 
    //Return status
  //ELSE
    //Disable interrupts
    //Save context
    //IF "first execution" THEN
      //Set: "not first execution any more"
      //Insert new task in Readylist
      //Load Context
    //ENDIF
  //ENDIF
  }
  //Return status
  return FAIL;
  
  
}
void idle_function(void){
    while(1){}
  }
exception init_kernel(void){
  exception result = OK;
  in_startup = TRUE;
  TimerInt();
  isr_off();
  list *readyList = createList();
  if(readyList == NULL){
  result = FAIL;
  }
  list *blockedList =  createList();
  if(blockedList == NULL){
  result = FAIL;
  }
  list *sleepList =  createList();
  if(blockedList == NULL){
  result = FAIL;
  }
  isr_on();
  
  exception idleTaskException = create_task(idle_function, UINT_MAX);
  
  return result && idleTaskException;
  
}
void TimerInt(void)
{
  static unsigned int ticks, first = 1;
  if (first == 1) 
  {
    first = 0; 
    ticks = 1;
  }
  else 
  {
    ticks = ticks + 1;
  }
}
void insertionSort(list* listToSort) 
{ 
   
    struct Node* sorted = NULL; 
    
    // Traverse the given doubly linked list and 
    // insert every node to 'sorted' 
    struct Node* current = *head_ref; 
    while (current != NULL) { 
  
        // Store next for next iteration 
        struct Node* next = current->next; 
  
        // removing all the links so as to create 'current' 
        // as a new node for insertion 
        current->prev = current->next = NULL; 
  
        // insert current in 'sorted' doubly linked list 
        sortedInsert(&sorted, current); 
  
        // Update current 
        current = next; 
    } 
  
    // Update head_ref to point to sorted doubly linked list 
    *head_ref = sorted; 
} 
void sortedInsert(list* sortedList, listobj* newNode)
{ 
    struct Node* current;
    
    listobj *head_ref = sortedList->*pHead;
    
  
    // if list is empty 
    if (*head_ref->pTask == NULL) 
        *head_ref -> = newNode; 
  
    // if the node is to be inserted at the beginning 
    // of the doubly linked list 
    else if ((*head_ref)->data >= newNode->data) { 
        newNode->next = *head_ref; 
        newNode->next->prev = newNode; 
        *head_ref = newNode; 
    } 
  
    else { 
        current = *head_ref; 
  
        // locate the node after which the new node 
        // is to be inserted 
        while (current->next != NULL &&  
               current->next->data < newNode->data) 
            current = current->next; 
  
        /*Make the appropriate links */
  
        newNode->next = current->next; 
  
        // if the new node is not inserted 
        // at the end of the list 
        if (current->next != NULL) 
            newNode->next->prev = newNode; 
  
        current->next = newNode; 
        newNode->prev = current; 
    } 
} 

