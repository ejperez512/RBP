#include "ofMain.h"
#include "ofApp.h"
#include "ofAppNoWindow.h"

int main(int argc, char *argv[]) {
	auto window = make_shared<ofAppNoWindow>();
	auto app = make_shared<ofApp>(argc, argv);

	ofRunApp(window, app);
	ofRunMainLoop();
} // int main(int argc, char *argv[])
