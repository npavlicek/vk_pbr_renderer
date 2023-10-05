#include "App.h"

#include <stdexcept>
#include <iostream>

int main() {
	lvk::App app;
	try {
		app.init();
		app.loop();
		app.cleanup();
	} catch (const std::exception &e) {
		std::cerr << "error encountered:\n" << e.what() << std::endl;
	}
	return 0;
}
