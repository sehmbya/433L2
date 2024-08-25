
/*******************************************************************************************
* AppDSP.c
* This is an example of a data processing task that does some real-time digital
* signal processing.
*
* 08/28/2024 Todd Morton
*******************************************************************************************/
/******************************************************************************************
* Include files
*******************************************************************************************/
#include "MCUType.h"
#include "app_cfg.h"
#include "os.h"
#include "CodecSAI.h"
#include "CodecDA7212.h"
#include "FRDM_MCXN947_GPIO.h"
#include "AppDSP.h"
#include "Codec_eDMA.h"
/******************************************************************************************/
static DSP_BLOCK_T dspInBuffer[DSP_NUM_IN_CHANNELS][DSP_NUM_BLOCKS];
static DSP_BLOCK_T dspOutBuffer[DSP_NUM_OUT_CHANNELS][DSP_NUM_BLOCKS];
static INT8U dspSuspendReqFlag = 0;
static OS_SEM dspSuspended;
/*******************************************************************************************
* Private Function Prototypes
*******************************************************************************************/
static void dspTask(void *p_arg);
static CPU_STK dspTaskStk[APP_CFG_DSP_TASK_STK_SIZE];
static OS_TCB dspTaskTCB;

/*******************************************************************************************
* DSPInit()- Initializes all dsp requirements - CODEC,I2S,DMA, and sets initial sample rate
*            and sample size.
*******************************************************************************************/
void DSPInit(void){
    OS_ERR os_err;

    OSTaskCreate(&dspTaskTCB,
                "DSP Task ",
                dspTask,
                (void *) 0,
                APP_CFG_DSP_TASK_PRIO,
                &dspTaskStk[0],
                (APP_CFG_DSP_TASK_STK_SIZE / 10u),
                APP_CFG_DSP_TASK_STK_SIZE,
                0,
                0,
                (void *) 0,
                (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                &os_err);

    OSSemCreate(&dspSuspended, "stream suspended", 0, &os_err);
    DMAInit(&dspInBuffer[0][0], &dspOutBuffer[0][0]);
    SAIInit(32);	//Todo: replace sample size with variable/define. Create settings structure
    SAI_RX_ENABLE();
    SAI_TX_ENABLE();
    CODECInit();

}

/*******************************************************************************************
* dspTask
* Pends on dma isr for ping-pong buffer.
*******************************************************************************************/
static void dspTask(void *p_arg){

    OS_ERR os_err;
    INT8U buffer_index;
    (void)p_arg;
    while(1){

        DB0_TURN_OFF();          /* Turn off debug bit while waiting for ping-pong buffer */
        buffer_index = DMAInPend(0, &os_err);
        DB0_TURN_ON();
        // DSP code goes here.
        // The following code implements a pass through
        dspOutBuffer[DSP_LEFT_CH][buffer_index] = dspInBuffer[DSP_LEFT_CH][buffer_index]; //Left Channel
        dspOutBuffer[DSP_RIGHT_CH][buffer_index] = dspInBuffer[DSP_RIGHT_CH][buffer_index]; //Right Channel

    }
}

/*******************************************************************************************
* DSPSampleSizeSet
* This sets the sample size.
* Note: This does not change the word size of the I2S, DMA, or buffer. They can be changed
*       independently.
*******************************************************************************************/
void DSPSampleSizeSet(INT8U ssize){

    (void)CODECSampleSizeSet(ssize);

}
/*******************************************************************************************
* DSPSampleSizeGet
* Reads current sample size from CODEC
*******************************************************************************************/
INT8U DSPSampleSizeGet(void){

	INT32U ssize;
	ssize = CODECSampleSizeGet();
	return ssize;

}
/*******************************************************************************************
* DSPSampleRateGet
* Reads current sample rate from CODEC
*******************************************************************************************/
INT32U DSPSampleRateGet(void){
	INT32U srate;
	srate = CODECSampleRateGet();
	return srate;

}
/*******************************************************************************************
* DSPSampleRateSet
* To set sample rate you set the rate on the CODEC
*******************************************************************************************/
void DSPSampleRateSet(INT32U srate){

    (void)CODECSampleRateSet(srate);

}
/*******************************************************************************************
* DSPResume
* Resume stream to fill blocks with samples
*******************************************************************************************/
void DSPResume(void){

    dspSuspendReqFlag = 0;
    DMAResume();
    SAIResume();
    CODECEnable();
}
/*******************************************************************************************
* DSPSuspendReqSet
* Set flag to request to suspend dsp stream after ping-pong buffer is full
*******************************************************************************************/
void DSPSuspendReqSet(void){

    dspSuspendReqFlag = 1;

}
/*******************************************************************************************
* DSPSuspendReqGet
* Check to see if there is a request to suspend DSP stream
*******************************************************************************************/
INT8U DSPSuspendReqGet(void){

    return (dspSuspendReqFlag);

}
/*******************************************************************************************
* DSPSuspend
* Disable Codec, SAI, DMA, in order.
*******************************************************************************************/
void DSPSuspend(void){
	OS_ERR os_err;

	CODECDisable();
	SAISuspend();
	DMASuspend();
	OSSemPost(&dspSuspended,OS_OPT_POST_1,&os_err);

}

/****************************************************************************************
 * DSP signal when buffer is full and DSP stream is suspended.
 * At this point it is safe for the shell to access the buffers.
 * 08/28/2024 TDM
 ***************************************************************************************/
void DSPSuspendedPend(OS_TICK tout, OS_ERR *os_err_ptr){
    OSSemPend(&dspSuspended, tout, OS_OPT_PEND_BLOCKING,(void *)0, os_err_ptr);
}

/****************************************************************************************
 * Return a pointer to the requested buffer
 * 04/16/2020 TDM
 ***************************************************************************************/
INT32S *DSPBufferGet(BUFF_ID_T buff_id){
    INT32S *buf_ptr = (void*)0;
    if(buff_id == LEFT_IN){
        buf_ptr = (INT32S *)&dspInBuffer[DSP_LEFT_CH][0];
    }else if(buff_id == RIGHT_IN){
        buf_ptr = (INT32S *)&dspInBuffer[DSP_RIGHT_CH][0];
    }else if(buff_id == RIGHT_OUT){
        buf_ptr = (INT32S *)&dspOutBuffer[DSP_RIGHT_CH][0];
    }else if(buff_id == LEFT_OUT){
        buf_ptr = (INT32S *)&dspOutBuffer[DSP_LEFT_CH][0];
    }else{
    }
    return buf_ptr;
}


