#include "action_requests.h"
#include "utils.h"


bool installProcessUnifiedInputEventHook(uintptr_t start, uintptr_t end);


bool installActionRequestsModule(uintptr_t start, uintptr_t end) {
	printModuleInstallHeader("Action Requests");
	bool success = true;
	success &= installProcessUnifiedInputEventHook(start, end);
	printModuleInstallResult(success);
	return success;
}


void createAndProcessDummyF24KeyUnifiedInputEvent();


void triggerActionRequestQueueProcessing() {
	// We need to trigger the function that processes a unified input event
	// so that it calls the action request queue processing code.
	createAndProcessDummyF24KeyUnifiedInputEvent();
}