#include "reverse/action_map.h"
#include "reverse/action.h"


namespace reverse {
	Action* getActionById(ActionMapNode** actionMap, uint32_t actionMapSize, uint32_t actionId) {
		if (!actionMap || actionMapSize <= 0) {
			return nullptr;
		}

		size_t bucketIndex = actionId % actionMapSize;
		ActionMapNode* currentNode = actionMap[bucketIndex];

		// handle collisions using linked list traversal
		while (currentNode) {
			if (currentNode->action.actionId == actionId) {
				return &currentNode->action;
			}
			currentNode = currentNode->next;
		}

		return nullptr;
	}
}