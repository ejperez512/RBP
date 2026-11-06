#include <iostream>

#include "ofMain.h"
#include "ofApp.h"
#include "ofAppNoWindow.h"

int main(int argc, char *argv[]) {
	auto window = make_shared<ofAppNoWindow>();
	auto app = make_shared<ofApp>();

	std::cout << std::endl;
	std::cout << "=========================================================================================================" << std::endl;
    std::cout << "AUDIO 3D BRT LIBRARY (https://github.com/GrupoDiana/BRTLibrary)" << std::endl;
	std::cout << "---------------------------------------------------------------------------------------------------------" << std::endl;
	std::cout << "This application uses a 'speech' source and a configurable number of 'steps' sources at random positions." << std::endl;
	std::cout << std::endl;
    std::cout << "Use: " << argv[0] << " [buffer_size] [num_src_steps]" << std::endl;
	std::cout << "     buffer_size: size of the audio buffer (default: 2048)" << std::endl;
	std::cout << "     num_src_steps: number of 'steps' sources (default: 0)" << std::endl;
	std::cout << "=========================================================================================================" << std::endl;
	std::cout << std::endl;

	if (argc > 1) {
        app->buffer_size = std::stoi(argv[1]);

		if (app->buffer_size <= 0) {
			app->buffer_size = 2048;
		} // if (buffer_size <= 0)

    } // if (argc > 1)
	
	else {
        app->buffer_size = 2048;
    } // else: if (argc > 1)

    if (argc > 2) {
        app->num_src_steps = std::stoi(argv[2]);

		if (app->num_src_steps < 0) {
			app->num_src_steps = 0;
		} // if (num_src_steps < 0)

    } // if (argc > 2)

	else {
		app->num_src_steps = 0;
	} // else: if (argc > 2)

	ofRunApp(window, app);
	return (0);
} // int main(int argc, char *argv[]) {
