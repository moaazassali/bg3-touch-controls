#include "render_hook.h"
#include "utils.h"


bool installDX11PresentHook();


bool installRenderHookModule() {
	printModuleInstallHeader("Render Hook");
	bool success = true;
	success &= installDX11PresentHook();
	printModuleInstallResult(success);
	return success;
}