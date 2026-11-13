#pragma once

#include "ofMain.h"
#include <vector>
#include <memory>
#include <string>
#include <BRTLibrary.h>

class ofApp : public ofBaseApp {

	public:
		// === ATTRIBUTES ===

		// === METHODS ===
		// OpenFrameworks
		ofApp(int argc, char *argv[]);
		void setup();
		void exit();
		void audioOut(float *outputBuffer, int bufferSize, int nChannels);
	
	private:
		// === ATTRIBUTES ===
		// Constants
		static constexpr int SAMPLE_RATE = 44100;
		static constexpr int HRTF_RESAMPLING_STEP = 15;

		static constexpr const char* SRC_SPEECH_FILE = "../resources/speech.wav";
		static constexpr float SRC_SPEECH_AZIMUTH = 90.0f;
		static constexpr float SRC_SPEECH_ELEVATION = 0.0f;
		static constexpr float SRC_SPEECH_DISTANCE = 2.0f;

		static constexpr const char* SRC_STEPS_FILE = "../resources/steps.wav";
		static constexpr const char* HRTF_FILE = "../resources/hrtf.sofa";
		static constexpr const char* ILD_FILE = "../resources/NearFieldCompensation_ILD_44100.sofa";

		static constexpr float LISTENER_AZIMUTH = 0.0f;
		static constexpr float LISTENER_ELEVATION = 0.0f;
		static constexpr float LISTENER_DISTANCE = 0.0f;

		// Parameters
		int buffer_size = -1;
		int num_src_steps = -1;
		int device = -1;

		// BRT
		BRTBase::CBRTManager brtManager;
		Common::CGlobalParameters globalParameters;
		std::shared_ptr<BRTListenerModel::CListenerModelBase> listenerMODEL;
		std::shared_ptr<BRTBase::CListener> listenerBRT;
		std::vector<std::shared_ptr<BRTSourceModel::CSourceModelBase>> sourcesBRT;
		std::vector<std::vector<float>> samplesVectorSources;
		std::vector<CMonoBuffer<float>> sourceInputBuffers;
		std::vector<unsigned int> wavSamplePositions;
		Common::CEarPair<CMonoBuffer<float>> outputBufferStereo;
		Common::CEarPair<CMonoBuffer<float>> bufferProcessed;

		// OpenFrameworks
		ofSoundStream systemSoundStream;

		// === METHODS ===
		// BRT
		void AudioSetup(void);
		void audioProcess(Common::CEarPair<CMonoBuffer<float>> &bufferOutput, int bufferSize);
		int GetAudioDeviceIndex(std::vector<ofSoundDevice> list);

		// Tools
		void LoadWav(std::vector<float> &samplesVector, const char *stringIn);
		bool LoadSofaFile(const std::string &_filePath, std::shared_ptr<BRTServices::CSphericalInterpolatedFIRTable> hrtf);
		bool LoadILD(std::string _ildFilePath, std::shared_ptr<BRTServices::CSphericalSOSTable> _sosFilter);
		void FillBuffer(CMonoBuffer<float> &output, unsigned int &position, std::vector<float> &samplesVector);
		Common::CVector3 Spherical2Cartesians(float azimuth, float elevation, float radius);
		int RandomNumber(int min, int max);
}; // class ofApp : public ofBaseApp
