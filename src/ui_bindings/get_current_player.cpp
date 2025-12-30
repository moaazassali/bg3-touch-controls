#include "game_bindings.h"
#include "utils.h"


using getPlayerCount_t = int64_t(*)(void* dataContext);
getPlayerCount_t original_getPlayerCount = nullptr;


bool findGetPlayerCountFunction(uintptr_t start, uintptr_t end)
{
	constexpr const char* pattern_getPlayerCount = "48 8B 49 18 48 83 C1 10 E9";
	uintptr_t matchAddr = findPattern(pattern_getPlayerCount, start, end);
	if (!matchAddr) {
		printPatternNotFoundError(pattern_getPlayerCount, "getPlayerCount");
		return false;
	}
	original_getPlayerCount = (getPlayerCount_t)(matchAddr);
	return true;
}


using getPlayerListHead_t = void* (*)(void* listContainer);
getPlayerListHead_t original_getPlayerListHead = nullptr;


bool findGetPlayerListHeadFunction(uintptr_t start, uintptr_t end)
{
	constexpr const char* pattern_getPlayerListHead = "E8 ?? ?? ?? ?? 41 8B D6 48 8B C8";
	uintptr_t matchAddr = findPattern(pattern_getPlayerListHead, start, end);
	if (!matchAddr) {
		printPatternNotFoundError(pattern_getPlayerListHead, "getPlayerListHead");
		return false;
	}
	// Calculate the target function address from the CALL instruction
	int32_t relativeOffset = *reinterpret_cast<int32_t*>(matchAddr + 1);
	uintptr_t targetAddr = matchAddr + 5 + relativeOffset;
	original_getPlayerListHead = (getPlayerListHead_t)(targetAddr);
	return true;
}


using getCurrentPlayer_t = void* (*)(void* a1, uint8_t a2);
getCurrentPlayer_t original_getCurrentPlayer = nullptr;


bool findGetCurrentPlayerFunction(uintptr_t start, uintptr_t end)
{
	constexpr const char* pattern_getCurrentPlayer = "40 53 48 83 EC 20 48 83 C1 10";
	uintptr_t matchAddr = findPattern(pattern_getCurrentPlayer, start, end);
	if (!matchAddr) {
		printPatternNotFoundError(pattern_getCurrentPlayer, "getCurrentPlayer");
		return false;
	}
	original_getCurrentPlayer = (getCurrentPlayer_t)(matchAddr);
	return true;
}


void* getDataContext();


void* getCurrentPlayer()
{
	// This is what the syncGameStateToUI function does to get the current player:
	// v353 = dataContext + 200;
	// v355 = 0;
	// LODWORD(v573) = sub_7FF64A537E30(v570 + 200);
	// if (v573 > 0):
	// in a loop:
	//	v356 = sub_7FF6496D30C0(*(v353 + 24));
	//	v357 = sub_7FF649693200(v356, v355);
	// v357 is the current player
	// I think the loop is for multiplayer or local co-op, but we only care about player v355 = 0
	if (!original_getPlayerCount ||
		!original_getPlayerListHead ||
		!original_getCurrentPlayer) {
		return nullptr;
	}

	void* dataContext = getDataContext();
	if (!dataContext) {
		return nullptr;
	}

	void* playerListContainer = (void*)((uintptr_t)dataContext + 200);
	if (original_getPlayerCount(playerListContainer) <= 0) {
		return nullptr;
	}

	void* playerListPtr = *(void**)((uintptr_t)playerListContainer + 24);
	if (!playerListPtr) {
		return nullptr;
	}

	void* playerListHead = original_getPlayerListHead(playerListPtr);
	if (!playerListHead) {
		return nullptr;
	}

	void* currentPlayer = original_getCurrentPlayer(playerListHead, 0);
	if (!currentPlayer) {
		return nullptr;
	}

	return currentPlayer;
}