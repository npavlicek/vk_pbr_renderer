#include "app.h"

#include <stdexcept>
#include <iostream>

int main() {
	lvk::app app;
	try {
		app.init();
		app.loop();
		app.cleanup();
	} catch (const std::exception &e) {
		std::cerr << "error encountered:\n" << e.what() << std::endl;
	}
	return 0;
}
