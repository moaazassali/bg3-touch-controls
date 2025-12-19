#include <functional>
#include <chrono>


void scheduleTask(std::function<void()> task, std::chrono::milliseconds delay);

SIZE getWindowSizePx(HWND hwnd);