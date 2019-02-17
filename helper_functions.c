void insertion_sort(list* l) 
{ 
  list* sorted = create_list(); 
  listobj* current = l->pHead->pNext; 
  l->pHead->pNext = NULL;
  while (current->pTask != NULL)
  {   
    listobj* next = current->pNext; 
    current->pPrevious = current->pNext = NULL;
    sorted_insert(sorted, current); 
    current = next; 
  }  
  //@TODO: Free memory space taken by original l
  l = sorted;
} 

// Only call if l is already sorted!
void sorted_insert(list* l, listobj* o)
{ 
  listobj* first = l->pHead->pNext; /* skip head */
  
  // SPECIAL CASE: Empty List.
  if (first->pTask == NULL) /* check for tail */
  {
    o->pNext = l->pTail;
    o->pPrevious = l->pHead;
    l->pHead->pNext = o;
    l->pTail->pPrevious = o;
  }
  // SPECIAL CASE: New object has lower deadline than in list.
  else if (o->pTask->DeadLine < first->pTask->DeadLine)
  { 
    o->pNext = first;
    o->pPrevious = l->pHead;
    l->pHead->pNext = o;
    first->pPrevious = o;
  } 
  // All other cases.
  else 
  { 
    listobj* current = first;
    // Iterate through list until current followers deadline is smaller than the one of the new object.
    while (current->pNext->pTask->DeadLine <= o->pTask->DeadLine && current->pNext->pTask != NULL)
    {
      current = current->pNext;
    }
    // Changing the pointers.
    listobj* follower = current->pNext;
    o->pNext = follower;
    o->pPrevious = current;
    follower->pPrevious = o;
    current->pNext = o;
  } 
}

static listobj* create_listobj( TCB* t )
{       
  listobj* o = ( listobj* ) calloc( 1, sizeof( listobj ) );
  if(o != NULL){
    o->pTask = t;
    o->nTCnt = 0;
    o->pMessage = NULL;
    o->pPrevious = NULL;
    o->pNext = NULL;
  }
  return o;
}

static list* create_list( void )
{
  list* l = ( list* ) calloc( 1, sizeof( list ) );
  if(l != NULL)
  {
    l->pHead = create_listobj(NULL);
    l->pTail = create_listobj(NULL);
    l->pHead->pNext = l->pTail;
    l->pTail->pPrevious = l->pHead;
  }
  return l;
}

static void idle_function( void )
{
  while(1)
  {
    //SWYM
  }
}
static msg* create_message(char *pData,exception Status)
{
  msg *m = ( msg* ) calloc( 1, sizeof( msg ) );
  if ( m != NULL ) {
    m->pData = pData;
    m->Status = Status;
    m->pBlock = NULL;
    m->pPrevious = NULL;
    m->pNext = NULL;
  }
  return m;
}

static msg* pop_mailbox_head(mailbox* mBox)
{
  msg* result =  mBox->pHead->pNext;
  //change pointers
  mBox->pHead->pNext = result->pNext;
  result->pPrevious = mBox->pHead;
  return result;
}


static void push_mailbox_tail( mailbox* mBox,msg* m)
{
  m->pNext = mBox->pTail;
  m->pPrevious = mBox->pTail->pPrevious;
  mBox->pTail->pPrevious->pNext = m;
  mBox->pTail->pPrevious = m;
}


static void remove_from_list( list* l, listobj* o)
{
  listobj* current = l->pHead;
  while(current!= o){
    current = current->pNext;
  }
  current->pPrevious->pNext = current->pNext;
  current->pNext->pPrevious= current->pPrevious;
}

static void remove_running_task_from_mailbox( mailbox* mBox )
{
  msg* current = mBox->pHead->pNext;
  while ( current->pBlock->pTask != RunningTask )
  {
    current = current->pNext;
  }
  current->pNext->pPrevious = current->pPrevious;
  current->pPrevious->pNext = current->pNext;
  free(current);
}
static void schedule(){
  RunningTask = ReadyList->pHead->pNext->pTask;
}

////deprecated
//static void add_to_list( list* l, listobj* o ) {
//  listobj* last = l->pTail->pPrevious;
//  last->pNext = o;
//  o->pPrevious =last;
//  o->pNext = l->pTail;
//  l->pTail->pPrevious = o;
//}