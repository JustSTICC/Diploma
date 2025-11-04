#include "Application.h"


int main() {
    try {
		Config config;
        Application app = Application(config);
        app.run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}