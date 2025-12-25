#include "game_bindings.h"


struct UnifiedInputEvent {
	int16_t internalInputId;
	int16_t deviceId;
	float scaledValue; // needs verification
	float rawValue; // needs verification
	bool isPressed;
	std::byte _pad_000D[7];
};

using processUnifiedInputEvent_t = bool (*)(void* a1, UnifiedInputEvent* a2);
processUnifiedInputEvent_t original_processUnifiedInputEvent = nullptr;

void* processUnifiedInputEvent_a1 = nullptr;


static bool detour_processUnifiedInputEvent(void* a1, UnifiedInputEvent* a2)
{
	if (a1) {
		processUnifiedInputEvent_a1 = a1;
	}
	return original_processUnifiedInputEvent(a1, a2);
}


bool installProcessUnifiedInputEventHook(uintptr_t start, uintptr_t end)
{
	constexpr const char* pattern_processUnifiedInputEvent = "48 8B C4 48 89 58 18 55 56 57 41 54 41 55 41 56 41 57 48 8D A8 C8 FD";
	uintptr_t matchAddr = findPattern(pattern_processUnifiedInputEvent, start, end);
	return installHook((void*)matchAddr, &detour_processUnifiedInputEvent, (void**)&original_processUnifiedInputEvent, "processUnifiedInputEventHook");
}


void createAndProcessDummyF24KeyUnifiedInputEvent() {
	if (!original_processUnifiedInputEvent || !processUnifiedInputEvent_a1) {
		return;
	}

	UnifiedInputEvent dummyEvent{
		.internalInputId = 0x55, // Tested by sending F24 key up SDL event
		.deviceId = 0, // 0 for keyboard, 1 for mouse, 3 for controller
		.scaledValue = 0.0f,
		.rawValue = 0.0f,
		.isPressed = false,
		._pad_000D = {},
	};
	original_processUnifiedInputEvent(processUnifiedInputEvent_a1, &dummyEvent);
}