#include "application.h"
#include <cstdio>
#include "csp/csp4cmsis.h"
#include "cmsis_os2.h"
#include "FreeRTOS.h"

static osThreadAttr_t csp_thread_config(void);
static void main_app_task(void * arg);

void csp_app_init(void){
	osThreadAttr_t attr = csp_thread_config();

	if(osThreadNew(main_app_task,NULL,&attr) == NULL){
		printf("ERROR: main_app_task creation has failed");
	}

}

static osThreadAttr_t csp_thread_config(void){
	osThreadAtrr_t attr = {
			attr.name = "MainApp",
			attr.stack_mem = mainAppStack,
			attr.stack_size = sizeof(mainAppStack),
			attr.cb_mem     = &mainAppControlBlock,
			attr.cb_size    = sizeof(mainAppControlBlock),
			attr.priority   = MAIN_APP_PRIORITY,
	};
	return attr;
}
static void main_app_task(void* arg){

	// to inform the compiler that the pointer is not being used
	(void)arg;

	const ui32_t tick_time = 1000U;

	osDelay(tick_time);

	printf("Launching CSP Static Network \n");

	static Channel<Message> chan_A;
	static Channel<Message> chan_B;

	static Sender sA(chan_A.writer(), 1);
	static Sender sB(chan_B.writer(), 2);
	static Receiver r1(chan_A.reader(), chan_B.reader());

	Run(InParallel(sA, sB, r1), ExecutionMode::StaticNetwork, NETWORK_PRIORITY);
	printf("MainApp_Task: network started. Terminating. \n");
	osThreadExit();
}
