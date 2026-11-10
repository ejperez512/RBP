#pragma once

#include "ofMain.h"

class ofApp : public ofBaseApp{

	public:
		ofApp(int argc, char *argv[]);
		void setup();
		void exit();
		void AudioSetup(void);
		void audioOut(float *outputBuffer, int bufferSize, int nChannels);
};
