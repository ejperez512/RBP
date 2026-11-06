#pragma once

#include "ofMain.h"
#include <BRTLibrary.h>
#include <random>
#include <vector>
#include <memory>
#include <string>

class ofApp : public ofBaseApp{

	public:
		// ===== ATTRIBUTES =====
		// Configuration Variables
		int buffer_size = 2048;
    	int num_src_steps = 0;

		// ===== METHODS =====
		// OpenFrameworks
		void setup();
		void AudioSetup(void);
		void audioOut(float *outputBuffer, int bufferSize, int nChannels);
	
	private:
		// ===== ATTRIBUTES =====
		// Audio Constants
		static constexpr int SAMPLE_RATE = 44100;
    	static constexpr int HRTF_RESAMPLING_STEP = 15;
		// OpenFrameworks
		ofSoundStream systemSoundStream;
		// BRT Library
		BRTBase::CBRTManager brtManager;
   		Common::CGlobalParameters globalParameters;
		std::vector<std::shared_ptr<BRTSourceModel::CSourceModelBase>> sourcesBRT;
		std::vector<std::vector<float>> samplesVectorSources;
		std::shared_ptr<BRTListenerModel::CListenerModelBase> listenerMODEL;
		std::shared_ptr<BRTBase::CListener> listenerBRT;
		std::vector<std::shared_ptr<BRTServices::CSphericalInterpolatedFIRTable>> HRTF_list;
		std::vector<std::shared_ptr<BRTServices::CSphericalSOSTable>> ILD_list;
		Common::CEarPair<CMonoBuffer<float>> outputBufferStereo;

		// ===== METHODS =====
		// BRT Library
		int GetAudioDeviceIndex(std::vector<ofSoundDevice> list);
		void LoadWav(std::vector<float> &samplesVector, const char *stringIn);
		bool LoadSofaFile(const std::string &_filePath, std::shared_ptr<BRTServices::CSphericalInterpolatedFIRTable> hrtf);
		bool LoadILD(std::string _ildFilePath, std::shared_ptr<BRTServices::CSphericalSOSTable> _sosFilter);
		void audioProcess(Common::CEarPair<CMonoBuffer<float>> &bufferOutput, int bufferSize);
		void FillBuffer(CMonoBuffer<float> &output, unsigned int &position, unsigned int &endFrame, std::vector<float> &samplesVector);
		Common::CVector3 Spherical2Cartesians(float azimuth, float elevation, float radius);
		int RandomNumber(int min, int max);
};
