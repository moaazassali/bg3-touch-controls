#pragma once
#include <cstdint>
#include <imgui.h>


bool installAbilityUsageModule(uintptr_t start, uintptr_t end);

bool isAnAbilityPrimed();

void renderConfirmAbilityButton(ImDrawList* drawList);
bool isPointInsideConfirmButton(float x, float y);
void pressInConfirmButton();
void pressOutConfirmButton();