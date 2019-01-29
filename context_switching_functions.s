        ;
        ; modified on 25th January
        ; these modifications are in SaveContext and LoadContext
        ;
        ; The main modification is that we have introduced sections that
        ; mimic the hardware stacking and hardware unstacking that
        ; happens during the servicing of interrupts
        ; Without this mimicing, task switching will not work correctly,
        ; because Stack overflows are possible.
        ; So this modification fixes a serious design bug
        ;
        ; Then at the end of SaveContext, we choose to not enable interrupts
        ; The idea begind this choice is:
        ;   A SaveContext(); call is eventually followed by a LoadContext(); call
        ;   or the LoadContext portion of the Sys Tick ISR.
        ;   it is sufficient that interreupts are enabled then
 
 
        MODULE  ?context_switching_functions

        PUBLIC  SaveContext
        PUBLIC  LoadContext
        PUBLIC  SysTick_Handler
        PUBLIC  isr_on
        PUBLIC  isr_off

        EXTERN  RunningTask
        EXTERN  TimerInt

        SECTION .text:CODE:REORDER(1)
        THUMB


SysTick_Handler
               
        TST     lr,  #0x04     ; if LR = 0xFFFFFFF9 then SP = MSP.   main was interrupted
                               ; if LR = 0xFFFFFFFD then SP = PSP. a task was interrupted
        BEQ     trigger_hardware_unstack  
                               ; proceed to end ISR if IRQ arrived while main was running

                               ;now proceed to save the context of current task 
        CPSID   I              ; first disable interrupts
    
        MRS     r2,  psp       ; hardware has already pushed 
                               ; r0, r1, r2, r3, r12, LR(r14), PC(r15), xPSR  (r0 at top)
        LDMIA   r2!, {r0-r1}   ; retrieve r0, r1 which were put on stack by hardware
        LDR     r3,  =RunningTask  
        LDR     r3,  [r3]      ; point r3 to RunningTask->Context[0]
        STMIA   r3!, {r0-r1}   ; store retrieved r0, r1 in RunningTask->Context[0], RunningTask->Context[1]
        MOV     r1,  r2        ; point r1 to StackSeg[?] where r2 is stored
        MOV     r0,  r3        ; point r0 to RunningTask->Context[2]
        LDMIA   r1!, {r2,r3}   ; retrieve r2, r3 put on stack by hardware
        STMIA   r0!, {r2,r3}   ; store retrieved r2, r3 in RunningTask->Context[2], RunningTask->Context[3]
          ; LDMIA   r1!, {r12}      ; substituted by following two lines because assembler gave a warning
        LDR     r12, [r1]
        ADD     r1,  r1, #4    ; point r1 to StackSeg[?] where LR is stored
        STMIA   r0!, {r4-r12}  ; store r4 - r11, and retrieved r12 in RunningTask->Context[4 to 12]        
        LDMIA   r1!, {r2-r4}   ; retrieve LR, PC, xPSR into r2, r3, r4
        MRS     r2,  psp        ; replace r2 by the PSP
        STMIA   r0!, {r2-r4}   ; store PSP, PC, xPSR in RunningTask->SP , ->PC , ->SPSR
        
        MOV     r0,  LR
        SUB     SP, SP, #4
        STR     r0,  [SP]      ; push LR to (main) stack
        
        BL      TimerInt       ; call Kernel C function TimerInt();
                               ; among other things, this might update RunningTask
                               ; (and unfortunately) changes LR
                               
        LDR     r0,  [SP]      ;
        ADD     SP,  SP, #4
        MOV     LR,  r0        ; catch the LR from the main stack
        
        LDR     r0,  =RunningTask  ;Load the context of the task to run
        LDR     r0,  [r0]
        ADD     r0,  r0, #8     ; make r0 point to RunningTask->Context[2]
        LDMIA   r0!, {r2-r12}   ; restore r2 through r12
      
        LDR     r1,  [r0]       ; make r1 point to RunningTask->SP
        MSR     psp,  r1        ; update psp
        
        SUB     r0,  r0, #52    ; these four lines are not really
                                ; needed because the hardware
        LDR     r1,  [r0, #4]   ; shall overwrite r0, r1 when ISR ends
        LDR     r0,  [r0]       ; but these lines help to debug and 
                                ; check if that is happening correctly

        CPSIE   I              ; enable interupts    

trigger_hardware_unstack
        BX      LR             ; triggers hardware unstacking and end of ISR


SaveContext
        CPSID   I              ; disable interrupts
        
        STMDB   SP!, {r0,LR}   ; save r0, LR on stack
                               ; this LR points to where execution should continue after
                               ; SaveContext(); is finished
        
        LDR     r0,  =RunningTask  
        LDR     r0,  [r0]      ; point r0 to RunningTask->Context[0]
        ADD     r0,  r0, #4    ;          to RunningTask->Context[1]
        STMIA   r0!, {r1-r12}  ; store r1-r12 in RunningTask->Context[1 to 12]
        
        MOV     r8,  r0        ; r8 points to RunningTask->SP
        LDMIA   SP!, {r0,LR} 
        
        MOV     r4,  r12
        MOV     r5,  LR        ; unfortunately not the original LR value before
                               ; SaveContext() was called. No way to get that
                               ; from inside SaveContext()
        MOV     r6,  LR        ; "PC" where execution should resume upon subsequent reentry into the task
        STR     r6,  [r8, #4]  ;      and store so that LoadContext() can retrieve this from RunningTask->PC
        MRS     r7,  PSR
                              ; now r0 to r3 have original r0 to r3
                              ;     r4 has original r12, r5 has (inexact) LR
                              ;     r6 has the appropriate PC, and r7 has PSR  
        
        STMDB   SP!, {r0-r7}  ; imitates the harwdare stacking that happens when an interrupt request is handled
        
        STR     SP,  [r8]     ; store SP at  RunningTask->SP
        
        SUB     r8,  r8, #52  ; point r8 to  RunningTask->Context[0]
        STR     r0,  [r8]     ; save  r0 at  RunningTask->Context[0]
       
        BX      LR


LoadContext
        CPSID   I               ; disable interrupts
        
        MRS     r0, CONTROL     ; 
        ORRS    r0, r0, #2      ; set the second least significant bit to 1
        MSR     CONTROL, r0     ; set SP = PSP
        ISB

        LDR     r0,  =RunningTask
        LDR     r0,  [r0]
        ADD     r0,  r0, #8     ; make r0 point to RunningTask->Context[2]
        LDMIA   r0!, {r2-r12}   ; restore r2 through r12
                                ; and now r0 points to RunningTask->SP
        LDR     r1,  [r0]
        MOV     SP,  r1         ; set SP to point to RunningTask->SP
           ; MOV     psp, r1      ;COMMENTED OUT - set PSP = retrieved SP, just in case

        ADD     r0,  r0, #4     ; and now r0 points to RunningTask->PC       
        LDR     r1,  [r0, #4]   ; catch RunningTask->SPSR
        CMP     r1,  #0         ;
        BNE     put_PSR
        MOV     r1,  #0x21000000; default psr value
put_PSR MSR     PSR, r1         ; copy Context->SPSR

        LDR     LR,  [r0]       ; put RunningTask->PC in LR
        CMP     LR,  #0         ; check if not initialized
        BEQ     trap           
        SUB     r0,  r0, #56    ; make r0 point to RunningTask->Context[0]   
        LDR     r1,  [r0, #4]   ; retrieve r1
        LDR     r0,  [r0]       ; retrieve r0
        
        ADD     SP,  SP, #32    ; imitates hardware unstacking upon completing an interrupt service

        CPSIE   I              ; enable interupts
        
        MOV     PC, LR
trap
        B       .        
        

isr_on
        CPSIE   I
        BX      lr
        
        
isr_off
        CPSID   I
        BX      lr

        END