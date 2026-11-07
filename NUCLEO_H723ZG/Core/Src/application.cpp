#include "application.h"
#include <cstdio>
#include "csp/csp4cmsis.h"
#include "cmsis_os2.h"
#include "FreeRTOS.h"

using namespace csp;

static osThreadAttr_t csp_thread_config(void);
static void main_app_task(void * arg);

constexpr int MESSAGES = 100;

struct Message{ int sender; int seq;};

// sender inherits the CSProcessStatic
class Sender : public CSProcessStatic<256>{
	Chanout<Message> out;
	int id;
public:
	// assuming this is the constructor
	Sender(Chanout<Message> o, int i) : out(o), id(i) {}
	void run() override {
		for(int i =0; i< MESSAGES; i++) out << Message {id,i};
	}
};

// receiver is also inherits from CSProcessStatic
class Receiver : public CSProcessStatic<512> {
    Chanin<Message> inA, inB;
public:
    Receiver(Chanin<Message> a, Chanin<Message> b) : inA(a), inB(b) {}
    void run() override {
        Message a{}, b{};
        int next[2] = {0, 0}, errors = 0, timeouts = 0;
        while (next[0] < MESSAGES || next[1] < MESSAGES) {
            RelTimeoutGuard timeout(Milliseconds(100));
            Alternative alt(inA | a, inB | b, timeout);
            switch (alt.fairSelect()) {
                case 0: if (a.sender != 1 || a.seq != next[0]) ++errors; ++next[0]; break;
                case 1: if (b.sender != 2 || b.seq != next[1]) ++errors; ++next[1]; break;
                default: ++timeouts; break;
            }
        }
        printf("csp_app: %d messages, %d errors, %d timeouts: %s\r\n",
               next[0] + next[1], errors, timeouts, errors == 0 ? "PASS" : "FAIL");
    }
};


void csp_app_init(void){
	osThreadAttr_t attr = csp_thread_config();

	if(osThreadNew(main_app_task,NULL,&attr) == NULL){
		printf("ERROR: main_app_task creation has failed");
	}

}

static osThreadAttr_t csp_thread_config(void){

	// temp for now but why does this work
	static uint32_t mainAppStack[1024];
	static StaticTask_t mainAppControlBlock;
	static const osPriority_t MAIN_APP_PRIORITY = osPriorityNormal;

	osThreadAttr_t attr = {
			.name = "MainApp",
			.cb_mem = &mainAppControlBlock,
			.cb_size    = sizeof(mainAppControlBlock),
			.stack_mem = mainAppStack,
			.stack_size = sizeof(mainAppStack),
			.priority   = MAIN_APP_PRIORITY
	};

	return attr;
}

static void main_app_task(void* arg){

	// to inform the compiler that the pointer is not being used
	(void)arg;

	const uint32_t tick_time = 1000U;

	osDelay(tick_time);

	printf("Launching CSP Static Network \n");

	// need to figure this bit out too
	static Channel<Message> chan_A,chan_B;

	static Sender sA(chan_A.writer(), 1), sB(chan_B.writer(), 2);
	static Receiver r1(chan_A.reader(), chan_B.reader());

	Run(InParallel(sA, sB, r1), ExecutionMode::StaticNetwork);
	printf("MainApp_Task: network started. Terminating. \n");
	osThreadExit();
}
