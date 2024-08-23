/*****************************************************************************************
* A simple demo program for uCOS-III. In this case it is Cesium3
* It tests multitasking, the timer, and task semaphores.
* This version is written for the MCXN947FRDM board.
* If uCOS is working the green LED should toggle every 100ms and the red LED
* should toggle every 1 second.
* Version 2024.1
* 04/26/2024, Todd Morton
*****************************************************************************************/
#include "MCUType.h"
#include "FRDM_MCXN947ClkCfg.h"
#include "FRDM_MCXN947_GPIO.h"
#include "os.h"

#include "app_cfg.h"
/*****************************************************************************************
* Allocate task control blocks
*****************************************************************************************/
static OS_TCB appTaskStartTCB;
static OS_TCB appTask1TCB;
static OS_TCB appTask2TCB;

/*****************************************************************************************
* Allocate task stack space.
*****************************************************************************************/
static CPU_STK appTaskStartStk[APP_CFG_TASK_START_STK_SIZE];
static CPU_STK appTask1Stk[APP_CFG_TASK1_STK_SIZE];
static CPU_STK appTask2Stk[APP_CFG_TASK2_STK_SIZE];

/*****************************************************************************************
* Task Function Prototypes. 
*   - Private if in the same module as startup task. Otherwise public.
*****************************************************************************************/
static void  appStartTask(void *p_arg);
static void  appTask1(void *p_arg);
static void  appTask2(void *p_arg);

/*****************************************************************************************
* Variables used for timestamp analysis. Made global for the Global Variable view in
* MCUX. Can be removed if you are not using these for debugging.
*****************************************************************************************/
static CPU_TS cycCnt;
static CPU_TS cycCntDiff;

/*****************************************************************************************
* main()
*****************************************************************************************/
void main(void) {

    OS_ERR  os_err;

    FRDM_MCXN947InitBootClock();
    CPU_IntDis();               /* Disable all interrupts, OS will enable them  */

    OSInit(&os_err);                    /* Initialize uC/OS-III                         */
//    assert(os_err == OS_ERR_NONE);

    OSTaskCreate(&appTaskStartTCB,                  /* Address of TCB assigned to task */
                 "Start Task",                      /* Name you want to give the task */
                 appStartTask,                      /* Address of the task itself */
                 (void *) 0,                        /* p_arg is not used so null ptr */
                 APP_CFG_TASK_START_PRIO,           /* Priority you assign to the task */
                 &appTaskStartStk[0],               /* Base address of task�s stack */
                 (APP_CFG_TASK_START_STK_SIZE/10u), /* Watermark limit for stack growth */
                 APP_CFG_TASK_START_STK_SIZE,       /* Stack size */
                 0,                                 /* Size of task message queue */
                 0,                                 /* Time quanta for round robin */
                 (void *) 0,                        /* Extension pointer is not used */
                 (OS_OPT_TASK_NONE),                /* Options */
                 &os_err);                          /* Ptr to error code destination */

//    assert(os_err == OS_ERR_NONE);

    OSStart(&os_err);               /*Start multitasking(i.e. give control to uC/OS)    */
    while(1){                       /* Error Trap - should never get here               */
    }
}

/*****************************************************************************************
* STARTUP TASK
* This should run once and be deleted. Could restart everything by creating.
*****************************************************************************************/
static void appStartTask(void *p_arg) {

    OS_ERR os_err;

    (void)p_arg;                        /* Avoid compiler warning for unused variable   */

    OS_CPU_SysTickInitFreq(SystemCoreClock);
    OSStatTaskCPUUsageInit(&os_err);
    GpioLEDGREENInit();
    GpioLEDREDInit();
    GpioDBugBitsInit();


    OSTaskCreate(&appTask1TCB,                  /* Create Task 1                    */
                "App Task1 ",
                appTask1,
                (void *) 0,
                APP_CFG_TASK1_PRIO,
                &appTask1Stk[0],
                (APP_CFG_TASK1_STK_SIZE / 10u),
                APP_CFG_TASK1_STK_SIZE,
                0,
                0,
                (void *) 0,
                (OS_OPT_TASK_NONE),
                &os_err);
//    assert(os_err == OS_ERR_NONE);

    OSTaskCreate(&appTask2TCB,    /* Create Task 2                    */
                "App Task2 ",
                appTask2,
                (void *) 0,
                APP_CFG_TASK2_PRIO,
                &appTask2Stk[0],
                (APP_CFG_TASK2_STK_SIZE / 10u),
                APP_CFG_TASK2_STK_SIZE,
                0,
                0,
                (void *) 0,
                (OS_OPT_TASK_NONE),
                &os_err);
//    assert(os_err == OS_ERR_NONE);

    OSTaskDel((OS_TCB *)0, &os_err);
//    assert(os_err == OS_ERR_NONE);
}

/*****************************************************************************************
* TASK #1
* Uses OSTimeDelay to signal the Task2 semaphore every second.
* It also toggles the green LED every 100ms.
*****************************************************************************************/
static void appTask1(void *p_arg){

    INT8U timcntr = 0;                              /* Counter for one second flag      */
    OS_ERR os_err;
    (void)p_arg;
    
    while(1){
    
        DB1_TURN_OFF();                             /* Turn off debug bit while waiting */
    	OSTimeDly(100,OS_OPT_TIME_PERIODIC,&os_err);     /* Task period = 100ms   */
//        assert(os_err == OS_ERR_NONE);
        DB1_TURN_ON();                          /* Turn on debug bit while ready/running*/
        GREEN_TOGGLE();
        timcntr++;
        if(timcntr == 10){                     /* Signal Task2 every second             */
            (void)OSTaskSemPost(&appTask2TCB,OS_OPT_POST_NONE,&os_err);
//            assert(os_err == OS_ERR_NONE);
            timcntr = 0;
        }else{
        }
    }
}

/*****************************************************************************************
* TASK #2
* Pends on its semaphore and toggles the red LED every second
*****************************************************************************************/
static void appTask2(void *p_arg){

    OS_ERR os_err;

    (void)p_arg;

    while(1) {                                  /* wait for Task 1 to signal semaphore  */

        DB2_TURN_OFF();                         /* Turn off debug bit while waiting     */
        OSTaskSemPend(0,                        /* No timeout                           */
                      OS_OPT_PEND_BLOCKING,     /* Block until posted                   */
                      &cycCnt,                  /* timestamp destination. Make NULL pointer if not using timestamp */
                      &os_err);
//        assert(os_err == OS_ERR_NONE);
        //calculate the task switch time using timestamp. Remove if not using timestamps.
        cycCntDiff = CPU_TS32_to_uSec(OS_TS_GET() - cycCnt); //confirmed by measurement
        DB2_TURN_ON();                          /* Turn on debug bit while ready/running*/
        RED_TOGGLE();
    }
}
/********************************************************************************/
