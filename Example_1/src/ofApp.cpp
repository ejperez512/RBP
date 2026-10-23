#include "ofApp.h"

#include <cstdio>
#include <cstring>
#include <iostream>
#include <random>

#include <BRTLibrary.h>

#define MAX_SRC					100

#define SAMPLE_RATE				44100

#define SRC_SPEECH_FILE 		"resources/speech.wav"
#define SRC_SPEECH_AZIMUTH		90
#define SRC_SPEECH_ELEVATION	0
#define SRC_SPEECH_DISTANCE		2

#define SRC_STEPS_FILE 			"resources/steps.wav"

#define HRTF_FILE				"resources/hrtf.sofa"
#define ILD_FILE				"resources/NearFieldCompensation_ILD_44100.sofa"

#define LISTENER_AZIMUTH		0
#define LISTENER_ELEVATION		0
#define LISTENER_DISTANCE		0

#define HRTF_RESAMPLING_STEP	15

int buffer_size;
int num_src_steps;

// OpenFrameworks
ofSoundStream systemSoundStream;

// BRT
BRTBase::CBRTManager brtManager;
Common::CGlobalParameters globalParameters;
std::shared_ptr<BRTSourceModel::CSourceModelBase> sourcesBRT[MAX_SRC];
std::vector<float> samplesVectorSources[MAX_SRC];
std::shared_ptr<BRTListenerModel::CListenerModelBase> listenerMODEL;
std::shared_ptr<BRTBase::CListener> listenerBRT;
std::vector<std::shared_ptr<BRTServices::CSphericalInterpolatedFIRTable>> HRTF_list;
std::vector<std::shared_ptr<BRTServices::CSphericalSOSTable>> ILD_list;
Common::CEarPair<CMonoBuffer<float>> outputBufferStereo;

// BRT
int GetAudioDeviceIndex(std::vector<ofSoundDevice> list);
void LoadWav(std::vector<float> &samplesVector, const char *stringIn);
bool LoadSofaFile(const std::string &_filePath, std::shared_ptr<BRTServices::CSphericalInterpolatedFIRTable> hrtf);
bool LoadILD(std::string _ildFilePath, std::shared_ptr<BRTServices::CSphericalSOSTable> _sosFilter);
void audioProcess(Common::CEarPair<CMonoBuffer<float>> &bufferOutput, int bufferSize);
void FillBuffer(CMonoBuffer<float> &output, unsigned int &position, unsigned int &endFrame, std::vector<float> &samplesVector);
Common::CVector3 Spherical2Cartesians(float azimuth, float elevation, float radius);
int RandomNumber(int min, int max);

// ==================== main ====================

void ofApp::setup() {
	int azi, ele, dst;
	int cont_src_steps;

	std::cout << std::endl;
	std::cout << "Enter buffer size (default is 2048): ";
	std::cin >> buffer_size;

	if (buffer_size <= 0) {
		buffer_size = 2048;
	} // if (buffer_size <= 0)
	
	std::cout << std::endl;
	std::cout << "This application uses a 'speech' source and a configurable number of 'steps' sources at random positions." << std::endl;
	std::cout << std::endl;
	std::cout << "Number of 'steps' sources (0.." << MAX_SRC-1 << "): ";
	std::cin >> num_src_steps;

	if (num_src_steps < 0) {
		num_src_steps = 0;
	} // if (num_src_steps < 0)
	
	if (num_src_steps > MAX_SRC-1) {
		num_src_steps = MAX_SRC-1;
	} // if (num_src_steps > MAX_SRC-1)

	std::cout << endl;

	// #################### BRT SOURCE CODE BEGIN ####################
	
	globalParameters.SetSampleRate(SAMPLE_RATE);
	globalParameters.SetBufferSize(buffer_size);

	// --- LISTENER ---

	// Create Listener
	brtManager.BeginSetup();
	listenerBRT = brtManager.CreateListener<BRTBase::CListener>("listener");
	brtManager.EndSetup();
	// Set Listener position
	Common::CTransform listenerBRT_pos = Common::CTransform();
	listenerBRT_pos.SetPosition(Spherical2Cartesians(LISTENER_AZIMUTH, LISTENER_ELEVATION, LISTENER_DISTANCE));
	listenerBRT->SetListenerTransform(listenerBRT_pos);

	// --- LISTENERMODEL ---

	// Create Listener Model
	brtManager.BeginSetup();
	listenerMODEL = brtManager.CreateListenerModel<BRTListenerModel::CListenerDirectHRTFConvolutionModel>("listenerMODEL");
	brtManager.EndSetup();

	// --- LISTENERMODEL -> LISTENERS

	brtManager.BeginSetup();
	listenerBRT->ConnectListenerModel(listenerMODEL);
	brtManager.EndSetup();

	// Load HRTF
	std::shared_ptr<BRTServices::CSphericalInterpolatedFIRTable> hrtf = std::make_shared<BRTServices::CSphericalInterpolatedFIRTable>();
	LoadSofaFile(HRTF_FILE, hrtf);
	// Set HRTF
	listenerBRT->SetHRTF(hrtf);
	// Load ILD
	std::shared_ptr<BRTServices::CSphericalSOSTable> sosFilter = std::make_shared<BRTServices::CSphericalSOSTable>();
	LoadILD(ILD_FILE, sosFilter);
	// Set ILD
	listenerBRT->SetNearFieldCompensationFilters(sosFilter);

	// === MULTISOURCES ===

	// --- SOURCE SPEECH [OMNIDIRECTIONAL]

	// Create Source 'speech'

	brtManager.BeginSetup();
	sourcesBRT[0] = brtManager.CreateSoundSource<BRTSourceModel::CSourceOmnidirectionalModel>("speech");
	brtManager.EndSetup();

	// --- SOURCE -> LISTENER MODEL

	brtManager.BeginSetup();
	listenerMODEL->ConnectSoundSource(sourcesBRT[0]);
	brtManager.EndSetup();

	// --- SOURCE POSITION

	// Set Source position
	Common::CTransform sourceBRT_pos = Common::CTransform();
	sourceBRT_pos.SetPosition(Spherical2Cartesians(SRC_SPEECH_AZIMUTH, SRC_SPEECH_ELEVATION, SRC_SPEECH_DISTANCE));
	sourcesBRT[0]->SetSourceTransform(sourceBRT_pos);
	printf("Source 'speech'      position: [AZI: % 04d ; ELE: % 03d ; DST: %d]\n", SRC_SPEECH_AZIMUTH, SRC_SPEECH_ELEVATION, SRC_SPEECH_DISTANCE);

	// --- SOURCE WAV

	// Load Source wave
	LoadWav(samplesVectorSources[0], SRC_SPEECH_FILE);

	// --- SOURCES STEPS [OMNIDIRECTIONAL]

	for (cont_src_steps = 0; cont_src_steps < num_src_steps; cont_src_steps++) {

		// Create Source 'steps'

		brtManager.BeginSetup();
		sourcesBRT[cont_src_steps+1] = brtManager.CreateSoundSource<BRTSourceModel::CSourceOmnidirectionalModel>("steps" + std::to_string(cont_src_steps+1));
		brtManager.EndSetup();

		// --- SOURCE -> LISTENER MODEL

		brtManager.BeginSetup();
		listenerMODEL->ConnectSoundSource(sourcesBRT[cont_src_steps+1]);
		brtManager.EndSetup();

		// --- SOURCE POSITION

		// Set Source position
		Common::CTransform sourceNBRT_pos = Common::CTransform();
		azi = RandomNumber(-180, 180);
		ele = RandomNumber(-90, 90);
		dst = RandomNumber(1, 5);
		sourceNBRT_pos.SetPosition(Spherical2Cartesians(azi, ele, dst));
		sourcesBRT[cont_src_steps+1]->SetSourceTransform(sourceNBRT_pos);
		printf("Source 'steps' [#%02d] position: [AZI: % 04d ; ELE: % 03d ; DST: %d]\n", cont_src_steps+1, azi, ele, dst);

		// --- SOURCE WAV

		// Load Source wave
		LoadWav(samplesVectorSources[cont_src_steps+1], SRC_STEPS_FILE);
	}

	// =====================
	// === AUDIO PROCESS ===
	// =====================

  	outputBufferStereo.left.resize(buffer_size);
  	outputBufferStereo.right.resize(buffer_size);

	// #################### BRT SOURCE CODE END ####################

	ofApp::AudioSetup();

} // void ofApp::setup()

// ==================== ofApp::AudioSetup ====================

void ofApp::AudioSetup(void) {
	std::vector<ofSoundDevice> deviceList;

	systemSoundStream.close();
	deviceList = systemSoundStream.getDeviceList();

	int deviceId = GetAudioDeviceIndex(deviceList);

	if (deviceId >= 0) {
		systemSoundStream.setDevice(deviceList[deviceId]);

		ofSoundDevice &dev = deviceList[deviceId];

		systemSoundStream.setup(this,
			dev.outputChannels,
			dev.inputChannels,
			SAMPLE_RATE,
			buffer_size,
			4
		);
		std::cout << endl;
		std::cout << "Device selected: " << dev.name << " (ID: " << dev.deviceID << ")" << std::endl;

	}
	else {
		std::cout << "Could not find any usable sound Device" << std::endl;
	}

} // void ofApp::AudioSetup(void)

// ==================== ofApp::audioOut ====================

void ofApp::audioOut(float *outputBuffer, int bufferSize, int nChannels) {
	float *floatOutputBuffer = (float *) outputBuffer;

	outputBufferStereo.left.Fill(bufferSize, 0.0f);
	outputBufferStereo.right.Fill(bufferSize, 0.0f);

	audioProcess(outputBufferStereo, bufferSize);

	CStereoBuffer<float> iOutput;
	iOutput.Interlace(outputBufferStereo.left, outputBufferStereo.right);

	for (auto it = iOutput.begin(); it != iOutput.end(); it++) {
		floatOutputBuffer[0] = *it;
		floatOutputBuffer = &floatOutputBuffer[1];
	}

} // void ofApp::audioOut

// ==================== GetAudioDeviceIndex ====================

int GetAudioDeviceIndex(std::vector<ofSoundDevice> list) {
	int numberOfAudioDevices = list.size();
	std::cout << endl;
	std::cout << "LIST OF AVAILABLE DEVICES:" << std::endl;
	std::cout << "--------------------------" << std::endl;

	for (int i = 0; i < numberOfAudioDevices; i++) {
		std::cout << "DEVICE '" << char('A'+i) << "': " << list[i].name << " (ID: " << list[i].deviceID << ")";

		if (list[i].outputChannels == 0) {
			std::cout << " [NO OUTPUT DEVICE!]";
		}

		std::cout << std::endl;
	}
	
	char selectedAudioDevice_key;
	int selectedAudioDevice;

	do {
		std::cout << std::endl;
		std::cout << "Please choose which audio output you wish to use: ";
		std::cin >> selectedAudioDevice_key;
		selectedAudioDevice_key = toupper(selectedAudioDevice_key);
		std::cin.clear();
		std::cin.ignore(INT_MAX, '\n');
		selectedAudioDevice = selectedAudioDevice_key - 'A';
	} while ((selectedAudioDevice < 0) || (selectedAudioDevice >= numberOfAudioDevices));

	for (int c = 0; c < numberOfAudioDevices; c++) {
		ofSoundDevice &dev = list[c];

		if ((dev.outputChannels >= 0) && c == selectedAudioDevice) {
			return c;
		}

	}

	for (int c = 0; c < numberOfAudioDevices; c++) {
		ofSoundDevice &dev = list[c];

		if ((dev.outputChannels >= 0) && dev.isDefaultOutput) {
			return c;
		}

	}

	return -1;
}

// ==================== LoadWav ====================

void LoadWav(std::vector<float> &samplesVector, const char *stringIn) {
	struct WavHeader {
		char		fill[40];
		uint32_t	bytesCount;
	} wavHeader;

	FILE* wavFile = fopen(stringIn, "rb");
	if (!wavFile) return;

	fread(&wavHeader, sizeof(wavHeader), 1, wavFile);
	fseek(wavFile, sizeof(wavHeader), SEEK_SET);

	unsigned int samplesCount = wavHeader.bytesCount / 2;
	int16_t *sample = new int16_t[samplesCount];
	memset(sample, 0, sizeof(int16_t) * samplesCount);

	uint8_t *byteSample = new uint8_t[2 * samplesCount];
	memset(byteSample, 0, sizeof(uint8_t) * 2 * samplesCount);

	fread(byteSample, 1, 2 * samplesCount, wavFile);
	fclose(wavFile);

	for (int i = 0; i < samplesCount; i++)
		sample[i] = int16_t(byteSample[2 * i] | byteSample[2 * i + 1] << 8);
	
	samplesVector.reserve(samplesCount);

	for (int i = 0; i < samplesCount; i++)
		samplesVector.push_back((float)sample[i] / (float)INT16_MAX);

	delete[] sample;
	delete[] byteSample;
}

// ==================== LoadSofaFile ====================

bool LoadSofaFile(const std::string &_filePath, std::shared_ptr<BRTServices::CSphericalInterpolatedFIRTable> hrtf) {
	BRTReaders::CSOFAReader sofaReader;
	Common::CGlobalParameters globalParameters; 

	int sampleRateInSOFAFile = sofaReader.GetSampleRateFromSofa(_filePath);

	if (sampleRateInSOFAFile == -1) {
		std::cout << ("Error loading HRTF Sofa file") << std::endl;
		return false;
	}

	if (globalParameters.GetSampleRate() != sampleRateInSOFAFile) {
		std::cout << "The sample rate in HRTF SOFA file doesn't match the configuration." << std::endl;
		return false;
	}

	std::cout << std::endl << "Loadind HRTF SOFA File....." << std::endl << std::endl;
	bool result = sofaReader.ReadHRTFFromSofa(_filePath, "", hrtf, HRTF_RESAMPLING_STEP, BRTServices::TEXTRAPOLATION_METHOD::nearest_point);

	if (result) {
		std::cout << ("HRTF Sofa file loaded successfully.") << std::endl;            
		return true;
	}

	else {
		std::cout << ("Error loading HRTF") << std::endl;
		return false;
	}
	
}

// ==================== LoadILD ====================

bool LoadILD(std::string _ildFilePath, std::shared_ptr<BRTServices::CSphericalSOSTable> _sosFilter) {
	BRTReaders::CSOFAReader sofaReader;
	Common::CGlobalParameters globalParameters;
	
	int sampleRateInSOFAFile = sofaReader.GetSampleRateFromSofa(_ildFilePath);

	if (sampleRateInSOFAFile == -1) {
		std::cout << ("Error loading ILD Sofa file") << std::endl;
		return false;
	}

	if (globalParameters.GetSampleRate() != sampleRateInSOFAFile) {
		std::cout << "The sample rate in ILD SOFA file" << std::endl;
		return false;
	}
	
	std::cout << std::endl << "Loadind SOS File....." << std::endl;
	bool result = sofaReader.ReadSOSFiltersFromSofa(_ildFilePath, _sosFilter);

	if (result) {
		std::cout << "ILD Sofa file loaded successfully: " << std::endl;            
		return true;
	}

	else {
		std::cout << "Error loading HRTF" << std::endl;
		return false;
	}

}

// ==================== audioProcess ====================

void audioProcess(Common::CEarPair<CMonoBuffer<float>> &bufferOutput, int bufferSize) {
	int cont_src_steps;
	CMonoBuffer<float> sourceInput[MAX_SRC];
	
	static unsigned int wavSamplePositionSource1 = 0;
	static unsigned int positionEndFrameSource1 = 0;
	static unsigned int wavSamplePositionSourceN = 0;
	static unsigned int positionEndFrameSourceN = 0;

	sourceInput[0] = CMonoBuffer<float> (bufferSize);
	FillBuffer(sourceInput[0], wavSamplePositionSource1, positionEndFrameSource1, samplesVectorSources[0]);
	
	for (cont_src_steps = 0; cont_src_steps < num_src_steps; cont_src_steps++) {
		sourceInput[cont_src_steps+1] = CMonoBuffer<float> (bufferSize);
		FillBuffer(sourceInput[cont_src_steps+1], wavSamplePositionSourceN, positionEndFrameSourceN, samplesVectorSources[cont_src_steps+1]);
	}

	Common::CEarPair<CMonoBuffer<float>> bufferProcessed;

	sourcesBRT[0]->SetBuffer(sourceInput[0]);

	for (cont_src_steps = 0; cont_src_steps < num_src_steps; cont_src_steps++) {
		sourcesBRT[cont_src_steps+1]->SetBuffer(sourceInput[cont_src_steps+1]);
	}

	brtManager.ProcessAll();
	listenerBRT->GetBuffers(bufferProcessed.left, bufferProcessed.right);

	bufferOutput.left += bufferProcessed.left;
	bufferOutput.right += bufferProcessed.right;
}

// ==================== FillBuffer ====================

void FillBuffer(CMonoBuffer<float> &output, unsigned int &position, unsigned int &endFrame, std::vector<float> &samplesVector) {
	position = endFrame + 1;

	if (position >= samplesVector.size()) {
		position = 0;
	}

	endFrame = position + output.size() - 1;

	for (int i = 0; i < output.size(); i++) {

		if ((position + i) < samplesVector.size()) {
			output[i] = (samplesVector[position + i]);
		}

		else {
			output[i] = 0.0f;
		}

	}

}

// ==================== Spherical2Cartesians ====================

Common::CVector3 Spherical2Cartesians(float azimuth, float elevation, float radius) {
	Common::CVector3 pos;

	pos.x = (float) radius * cos(azimuth) * cos(elevation);
	pos.y = (float) radius * sin(azimuth) * cos(elevation);
	pos.z = (float) radius * sin(elevation);

	return pos;
}

// ==================== RandomNumber ====================

int RandomNumber(int min, int max) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(min, max);
    return dist(gen);
}