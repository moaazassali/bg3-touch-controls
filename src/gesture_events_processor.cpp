#include "touch_handler.h"


static void processTap(const GestureEvent& event) {

}


static void processHold(const GestureEvent& event) {

}


static void processDrag(const GestureEvent& event) {

}


static void processSingleGestureEvent(const GestureEvent& event) {
	switch (event.type)
	{
	case Tap:
		processTap(event);
		break;
	case Hold:
		processHold(event);
		break;
	case Drag:
		processDrag(event);
		break;
	default:
		std::cout << "Other gesture detected: " << event.type << std::endl;
	}
}


void processTouchInput() {
	auto events = TouchHandler::getSingleton().getGestureEvents();

	for (int i = 0; i < events.count; ++i) {
		const auto& event = events.events[i];
		std::cout << "Detected gesture: " << event.type << std::endl;
		processSingleGestureEvent(event);
	}
}