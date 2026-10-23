#pragma once

#include "ofMain.h"

class ofApp : public ofBaseApp{

	public:
		// OpenFrameworks
		void setup();
		void AudioSetup(void);
		void audioOut(float *outputBuffer, int bufferSize, int nChannels);
		//void audioOut(ofSoundBuffer& buffer);
};
