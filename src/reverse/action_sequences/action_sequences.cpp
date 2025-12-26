#include "reverse/action_sequences/action_sequences.h"
#include "reverse/input_manager.h"
#include "reverse/action_map.h"
#include "action_requests.h"


namespace reverse {
	void populateActionPointers(std::span<ActionRequest> seq) {
		EnterCriticalSection((CRITICAL_SECTION*)reverse::g_inputManager);
		for (ActionRequest& req : seq) {
			req.action = getActionById(g_inputManager->actionMap, g_inputManager->actionMapSize, req.actionId);
		}
		LeaveCriticalSection((CRITICAL_SECTION*)reverse::g_inputManager);
	}


	void pushSequenceToBuffer(std::span<ActionRequest> seq, uint8_t playerIndex) {
		EnterCriticalSection((CRITICAL_SECTION*)reverse::g_inputManager);
		void* actionRequestsBuffer = original_getActionRequestsBuffer(reinterpret_cast<void*>(&g_inputManager->actionBufferManagerStuctStart), &playerIndex);
		auto q1 = actionRequestsBuffer;
		auto q2 = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(actionRequestsBuffer) + 0x10);
		auto q3 = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(actionRequestsBuffer) + 0x20);

		for (ActionRequest& req : seq) {
			if (req.actionState == 1) {
				original_pushToActionRequestQueue(q1, &req);
			}
			else if (req.actionState == 2) {
				original_pushToActionRequestQueue(q2, &req);
			}
			else if (req.actionState == 4) {
				original_pushToActionRequestQueue(q3, &req);
			}
		}
		LeaveCriticalSection((CRITICAL_SECTION*)reverse::g_inputManager);
	}
}