#include <iostream>

#include "game_bindings.h"
#include "sdl_bindings/sdl_bindings.h"
#include "utils.h"


// =============================================================
//                           STEP 1
// =============================================================
// We want to register our custom binding string "TouchUI_UndoMultiTargetCommand".
// We are choosing offset 3328 (0xD00) because we know that the size of a1 from registerHotBarUIBindingHandlers_t
// is 3328 bytes. There is a malloc call of size 3328 before the call to registerHotBarUIBindingHandlers_t.
// So, we want to choose a new offset that is outside the bounds of the existing struct to avoid overwriting any existing bindings.


// This function registers the bindings for the HotBar UI element.
// This refers to the bindings that can be referenced in the UI xaml files, which could be commands or properties.
// For example, 'DecreaseRowsCommand' and 'NumberOfRows' are bindings used in the HotBar.xaml UI file.
// a1 is a heap-allocated struct for the HotBar UI element.
// It is not the only struct associated with the HotBar UI, but it is the one used for registering bindings.
using registerHotBarUIBindings_t = uint64_t(*)(void* a1);
constexpr const uintptr_t offset_registerHotBarUIBindings = 0x170BF50;
registerHotBarUIBindings_t original_registerHotBarUIBindings = nullptr;

// The function used in registerHotBarUIBindings to register each individual binding.
// a1 is the same a1 from registerHotBarUIBindings.
// a2 is unknown, seems to be overwritten and returned back.
// a3 is the name of the binding to register.
// a4 is the offset/index/id of the binding, where the actual binding handler is stored in the a1 struct from registerHotBarUIBindings.
using registerUIBinding_t = void* (*)(void* a1, void* a2, const char* a3, int a4);
constexpr const uintptr_t offset_registerUIBinding = 0x1441170;
registerUIBinding_t original_registerUIBinding = nullptr;


static uint64_t detour_registerHotBarUIBindings(void* a1)
{
	auto original_return = original_registerHotBarUIBindings(a1);
	// Since a2 in registerUIBinding_t seems to just be overwritten and returned back, we can pass anything. 
	void* a2 = nullptr;
	original_registerUIBinding(a1, &a2, "TouchUI_UndoMultiTargetCommand", 0xD00);
	return original_return;
}


bool installRegisterHotBarUIBindingsHook(uintptr_t start, uintptr_t end)
{
	original_registerUIBinding = (registerUIBinding_t)(start + offset_registerUIBinding);
	uintptr_t targetAddr = start + offset_registerHotBarUIBindings;
	return installHook((LPVOID)targetAddr, (LPVOID)detour_registerHotBarUIBindings, (LPVOID*)&original_registerHotBarUIBindings, "registerHotBarUIBindings");
}


// =============================================================
//                           STEP 2
// =============================================================
// We will now increase the size of the struct allocated for a1 in registerHotBarUIBindingHandlers_t
// We will increase the size of that struct by 8 bytes to hold our new binding handler to to 3336/0xD08 bytes
// and use the new 8-byte space at offset 3328 to store our custom handler later.
bool installIncreaseHotBarUIBindingHandlersStructSizePatch(uintptr_t start, uintptr_t end)
{
	constexpr const char* pattern = "8B F8 4C 8B 25 9F D4 C8 04 4D 8B 6E 78 49 8B 46 20 48 89 45 50 4D 8B 76 28 B9 00 0D 00 00";
	constexpr const char* patternName = "increaseHotBarUIBindingHandlersStructSize";
	constexpr std::size_t offset = 26;

	uintptr_t address = findPattern(pattern, start, end);
	if (address == 0) {
		std::cout << "Pattern not found: increaseHotBarUIBindingHandlersStructSize" << std::endl;
		return false;
	}

	bool patched = asmPatchAtOffset(address, offset, std::vector<uint8_t>{0x08}, patternName);
	if (!patched) {
		std::cout << "Failed to patch " << patternName << std::endl;
		return false;
	}

	std::cout << "Successfully installed " << patternName << " patch." << std::endl;
	return true;
}



// =============================================================
//                           STEP 3
// =============================================================
// Now we want to define our custom handler function for the "TouchUI_UndoMultiTargetCommand" binding.
// We will then store the pointer to this function at offset 3328 (0xD00) of the a1 struct from registerHotBarUIBindingHandlers_t,
// which corresponds to the new space we created in the previous step and the same offset we used when registering the binding string.

// This registers the function handlers for the UI bindings from the HotBar UI element.
// a1 is a heap-allocated struct that holds the handlers for the HotBar UI element.
using registerHotBarUIBindingHandlers_t = void* (*)(void* a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6, uint64_t a7);
constexpr const uintptr_t offset_registerHotBarUIBindingHandlers = 0x170CDD0;
registerHotBarUIBindingHandlers_t original_registerHotBarUIBindingHandlers = nullptr;

// Not sure why the game doesn't call malloc directly.
using mallocWrapper_t = void* (*)(size_t a1);
constexpr const uintptr_t offset_mallocWrapper = 0x4579D20;
mallocWrapper_t original_mallocWrapper = nullptr;

// Initializes the handler wrapper object.
// Didn't investigate what it sets up exactly, but the function handler ptr is at offset 0x70.
// a1 is the 160-byte wrapper object allocated via mallocWrapper_t.
// a2 is a1 + 0x40. Not sure what it is though.
// a3 is a pointer to a stack-allocated struct that holds the function pointer and other data.
using initHandlerWrapper_t = void* (*)(void* a1, void* a2, void* a3);
constexpr const uintptr_t offset_initHandlerWrapper = 0x21108A0;
initHandlerWrapper_t original_initHandlerWrapper = nullptr;

// Not sure what it does. Name might be misleading.
using vtableCleanup_t = void (*)(void* a1, int a2);
constexpr const uintptr_t offset_vtable = 0x554E420;
void* vtablePtr = nullptr;


// I am not entirely sure that the function definition is correct.
// The assembly code suggests that it only uses a1, but while debugging, it seems that 3 parameters are passed.
static void undoMultiTargetCommandHandler(void* a1)
{
	// simulate a right click
	// but we need to make sure the cursor is not over a UI element
	// top-left corner (0,0) is a good candidate for now
	std::cout << "[MOD] UndoMultiTargetCommand handler called. Simulating right click at (0,0)." << std::endl;
	sdl_bindings::moveCursor(0, 0);
	auto task = []() {sdl_bindings::clickMouseButton(SDL_BUTTON_RIGHT);};
	scheduleTask(task, std::chrono::milliseconds(10));
}


// We are replicating the exact same logic used in the original registerHotBarUIBindingHandlers
// function to register a handler for a binding (based on the binding offset/id).
// 
// Side note: If we want to change the function pointer for an existing binding handler,
// we can do so by modifying the function pointer in the wrapper object for that handler directly:
//  uint64_t wrapper_ptr = *(uint64_t*)(a1 + 3104);
//  if (wrapper_ptr) {
//      uint64_t* function_ptr_location = (uint64_t*)(wrapper_ptr + 0x70);
//      *function_ptr_location = (uint64_t)&customHandler;
//  }
static void* detour_registerHotBarUIBindingHandlers(void* a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6, uint64_t a7)
{
	auto original_return = original_registerHotBarUIBindingHandlers(a1, a2, a3, a4, a5, a6, a7);

	void* handler_data[4]{};
	handler_data[0] = vtablePtr;
	handler_data[1] = a1;
	handler_data[2] = &undoMultiTargetCommandHandler;
	handler_data[3] = 0;

	auto handlerWrapper = original_mallocWrapper(160);

	if (handlerWrapper) {
		auto result = original_initHandlerWrapper(handlerWrapper, (void*)((uintptr_t)a1 + 0x40), handler_data);
		*(void**)((uintptr_t)a1 + 0xD00) = result;
		// Replicating (**handler_data)(&handler_data, 0) from the assembly code.
		auto cleanup = (vtableCleanup_t) * (void**)handler_data[0];
		cleanup(handler_data, 0);
	}
	else {
		*(void**)((uintptr_t)a1 + 0xD00) = 0;
	}

	return original_return;
}


bool installRegisterHotBarUIBindingHandlersHook(uintptr_t start, uintptr_t end)
{
	original_mallocWrapper = (mallocWrapper_t)(start + offset_mallocWrapper);
	original_initHandlerWrapper = (initHandlerWrapper_t)(start + offset_initHandlerWrapper);
	vtablePtr = (void*)(start + offset_vtable);

	uintptr_t targetAddr = start + offset_registerHotBarUIBindingHandlers;
	return installHook((LPVOID)targetAddr, (LPVOID)detour_registerHotBarUIBindingHandlers, (LPVOID*)&original_registerHotBarUIBindingHandlers, "registerHotBarUIBindingHandlers");
}