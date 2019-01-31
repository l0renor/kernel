/*********************************************************/
/** Global variabels and definitions                     */
/*********************************************************/

#include <stdlib.h>

#define CONTEXT_SIZE    13  /*  for the 13 general purpose registers: r0 to r12 */ 
#define STACK_SIZE      100 /* 100  about enough space*/


#define TRUE    1
#define FALSE   !TRUE

#define RUNNING 1
#define INIT    !RUNNING

#define FAIL    0
#define SUCCESS 1
#define OK              1

#define DEADLINE_REACHED        0
#define NOT_EMPTY               0

#define SENDER          +1
#define RECEIVER        -1


typedef int             exception;
typedef int             bool;
typedef unsigned int    uint;
typedef int 		action;

struct  l_obj;         // Forward declaration



// Task Control Block, TCB
typedef struct
{
        uint    Context[CONTEXT_SIZE];        
        uint    *SP;
        void    (*PC)();
        uint    SPSR;     
        uint    StackSeg[STACK_SIZE];
        uint    DeadLine;
} TCB;


// Message items
typedef struct msgobj {
        char            *pData;
        exception       Status;
        struct l_obj    *pBlock;
        struct msgobj   *pPrevious;
        struct msgobj   *pNext;
} msg;

// Mailbox structure
typedef struct {
        msg             *pHead;
        msg             *pTail;
        int             nDataSize;
        int             nMaxMessages;
        int             nMessages;
        int             nBlockedMsg;
} mailbox;


// Generic list item
typedef struct l_obj {
         TCB            *pTask;
         uint           nTCnt;
         msg            *pMessage;
         struct l_obj   *pPrevious;
         struct l_obj   *pNext;
} listobj;


// Generic list
typedef struct _list {
         listobj        *pHead;
         listobj        *pTail;
} list;


// Function prototypes


// Task administration
int             init_kernel( void );
exception	create_task( void (* body)(), uint d );
void            terminate( void );
void            run( void );

// Communication
mailbox*	create_mailbox( uint nMessages, uint nDataSize );
int             no_messages( mailbox* mBox );

exception       send_wait( mailbox* mBox, void* pData );
exception       receive_wait( mailbox* mBox, void* pData );

exception	send_no_wait( mailbox* mBox, void* pData );
int             receive_no_wait( mailbox* mBox, void* pData );


// Timing
exception	wait( uint nTicks );
void            set_ticks( uint no_of_ticks );
uint            ticks( void );
uint		deadline( void );
void            set_deadline( uint nNew );
void            TimerInt( void );

//Interrupt
extern void     isr_off( void );
extern void     isr_on( void );
extern void     SaveContext( void );	// Stores DSP registers in TCB pointed to by Running
extern void     LoadContext( void );	// Restores DSP registers from TCB pointed to by Running

//Own helper functions
static void     insertionSort( list* l );
static void     sortedInsert( list* l, listobj* o );
static list*    create_list( void );
static void     idle_function( void );

