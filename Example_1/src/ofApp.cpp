#include "ofApp.h"

#include <cstdio>
#include <cstring>
#include <iostream>
#include <random>

#include <BRTLibrary.h>

#define SRC_SPEECH_FILE 		"../resources/speech.wav"
#define SRC_SPEECH_AZIMUTH		90
#define SRC_SPEECH_ELEVATION	0
#define SRC_SPEECH_DISTANCE		2

#define SRC_STEPS_FILE 			"../resources/steps.wav"

#define HRTF_FILE				"../resources/hrtf.sofa"
#define ILD_FILE				"../resources/NearFieldCompensation_ILD_44100.sofa"

#define LISTENER_AZIMUTH		0
#define LISTENER_ELEVATION		0
#define LISTENER_DISTANCE		0

// ==================== ofApp::setup ====================

void ofApp::setup() {
	int azi, ele, dst;
	int cont_src_steps;

	std::cout << "PARAMETERS:" << std::endl;
	std::cout << ". buffer size: " << buffer_size << std::endl;
	std::cout << ". sources    : " << 1+num_src_steps << std::endl;
	std::cout << std::endl;

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

	sourcesBRT.clear();
	samplesVectorSources.clear();

	// Create Source 'speech'

	brtManager.BeginSetup();
	auto speechSource = brtManager.CreateSoundSource<BRTSourceModel::CSourceOmnidirectionalModel>("speech");
	brtManager.EndSetup();

	// --- SOURCE -> LISTENER MODEL

	brtManager.BeginSetup();
	listenerMODEL->ConnectSoundSource(speechSource);
	brtManager.EndSetup();

	// --- SOURCE POSITION

	// Set Source position
	Common::CTransform sourceBRT_pos = Common::CTransform();
	sourceBRT_pos.SetPosition(Spherical2Cartesians(SRC_SPEECH_AZIMUTH, SRC_SPEECH_ELEVATION, SRC_SPEECH_DISTANCE));
	speechSource->SetSourceTransform(sourceBRT_pos);
	std::cout << "Source 'speech'      position: [AZI: " << SRC_SPEECH_AZIMUTH << " ; ELE: " << SRC_SPEECH_ELEVATION << " ; DST: " << SRC_SPEECH_DISTANCE << "]" << std::endl;

	// --- ADD SOURCE

	sourcesBRT.push_back(speechSource);

	// --- SOURCE WAV

	// Load Source wave
	std::vector<float> speechSamples;
	LoadWav(speechSamples, SRC_SPEECH_FILE);
	samplesVectorSources.push_back(speechSamples);

	// --- SOURCES STEPS [OMNIDIRECTIONAL]

	for (cont_src_steps = 0; cont_src_steps < num_src_steps; cont_src_steps++) {

		// Create Source 'steps'

		brtManager.BeginSetup();
		auto stepSource = brtManager.CreateSoundSource<BRTSourceModel::CSourceOmnidirectionalModel>("steps" + std::to_string(cont_src_steps + 1));
		brtManager.EndSetup();

		// --- SOURCE -> LISTENER MODEL

		brtManager.BeginSetup();
		listenerMODEL->ConnectSoundSource(stepSource);
		brtManager.EndSetup();

		// --- SOURCE POSITION

		// Generate random position
		azi = RandomNumber(-180, 180);
		ele = RandomNumber(-90, 90);
		dst = RandomNumber(1, 5);

		// Set Source position
		Common::CTransform sourceNBRT_pos = Common::CTransform();
		sourceNBRT_pos.SetPosition(Spherical2Cartesians(azi, ele, dst));
		stepSource->SetSourceTransform(sourceNBRT_pos);
		std::cout << "Source 'steps' [#" << cont_src_steps+1 << "] position: [AZI: " << azi << " ; ELE: " << ele << " ; DST: " << dst << "]" << std::endl;

		// --- ADD SOURCE

		sourcesBRT.push_back(stepSource);
	
		// --- SOURCE WAV

		// Load Source wave
		std::vector<float> stepSamples;
		LoadWav(stepSamples, SRC_STEPS_FILE);
		samplesVectorSources.push_back(stepSamples);
	} // for (cont_src_steps = 0; cont_src_steps < num_src_steps; cont_src_steps++)

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

	} // if (deviceId >= 0)

	else {
		std::cout << "Could not find any usable sound Device" << std::endl;
	} // else: if (deviceId >= 0)

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
	} // for (auto it = iOutput.begin(); it != iOutput.end(); it++) {

} // void ofApp::audioOut

// ==================== GetAudioDeviceIndex ====================

int ofApp::GetAudioDeviceIndex(std::vector<ofSoundDevice> list) {
	int numberOfAudioDevices = list.size();
	std::cout << endl;
	std::cout << "LIST OF AVAILABLE DEVICES:" << std::endl;
	std::cout << "--------------------------" << std::endl;

	for (int i = 0; i < numberOfAudioDevices; i++) {
		std::cout << "DEVICE '" << char('A'+i) << "': " << list[i].name << " (ID: " << list[i].deviceID << ")";

		if (list[i].outputChannels == 0) {
			std::cout << " [NO OUTPUT DEVICE!]";
		} // if (list[i].outputChannels == 0)

		std::cout << std::endl;
	} // for (int i = 0; i < numberOfAudioDevices; i++) {
	
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
		} // if ((dev.outputChannels >= 0) && c == selectedAudioDevice)

	} // for (int c = 0; c < numberOfAudioDevices; c++) {

	for (int c = 0; c < numberOfAudioDevices; c++) {
		ofSoundDevice &dev = list[c];

		if ((dev.outputChannels >= 0) && dev.isDefaultOutput) {
			return c;
		} // if ((dev.outputChannels >= 0) && dev.isDefaultOutput)

	} // for (int c = 0; c < numberOfAudioDevices; c++) {

	return -1;
} // int ofApp::GetAudioDeviceIndex(std::vector<ofSoundDevice> list) {

// ==================== LoadWav ====================

void ofApp::LoadWav(std::vector<float> &samplesVector, const char *stringIn) {
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

	for (int i = 0; i < samplesCount; i++) {
		sample[i] = int16_t(byteSample[2 * i] | byteSample[2 * i + 1] << 8);
	} // for (int i = 0; i < samplesCount; i++) {
	
	samplesVector.reserve(samplesCount);

	for (int i = 0; i < samplesCount; i++) {
		samplesVector.push_back((float)sample[i] / (float)INT16_MAX);
	} // for (int i = 0; i < samplesCount; i++) {

	delete[] sample;
	delete[] byteSample;
} // void ofApp::LoadWav(std::vector<float> &samplesVector, const char *stringIn) {

// ==================== LoadSofaFile ====================

bool ofApp::LoadSofaFile(const std::string &_filePath, std::shared_ptr<BRTServices::CSphericalInterpolatedFIRTable> hrtf) {
	BRTReaders::CSOFAReader sofaReader;
	Common::CGlobalParameters globalParameters; 

	int sampleRateInSOFAFile = sofaReader.GetSampleRateFromSofa(_filePath);

	if (sampleRateInSOFAFile == -1) {
		std::cout << "ERROR: LoadSofaFile -> GetSampleRateFromSofa [" << _filePath << "]" << std::endl;
		return (false);
	} // if (sampleRateInSOFAFile == -1)

	if (globalParameters.GetSampleRate() != sampleRateInSOFAFile) {
		std::cout << "ERROR: LoadSofaFile -> GetSampleRate" << std::endl;
		return (false);
	} // if (globalParameters.GetSampleRate() != sampleRateInSOFAFile)

	bool result = sofaReader.ReadHRTFFromSofa(_filePath, "", hrtf, HRTF_RESAMPLING_STEP, BRTServices::TEXTRAPOLATION_METHOD::nearest_point);

	if (!result) {
		std::cout << "ERROR: LoadSofaFile -> ReadHRTFFromSofa [" << _filePath << "]" << std::endl;
		return (false);
	} // if (!result)

	return (result);
} // bool ofApp::LoadSofaFile(const std::string &_filePath, std::shared_ptr<BRTServices::CSphericalInterpolatedFIRTable> hrtf) {

// ==================== LoadILD ====================

bool ofApp::LoadILD(std::string _ildFilePath, std::shared_ptr<BRTServices::CSphericalSOSTable> _sosFilter) {
	BRTReaders::CSOFAReader sofaReader;
	Common::CGlobalParameters globalParameters;
	
	int sampleRateInSOFAFile = sofaReader.GetSampleRateFromSofa(_ildFilePath);

	if (sampleRateInSOFAFile == -1) {
		std::cout << "ERROR: LoadILD -> GetSampleRateFromSofa [" << _ildFilePath << "]" << std::endl;
		return (false);
	} // if (sampleRateInSOFAFile == -1)

	if (globalParameters.GetSampleRate() != sampleRateInSOFAFile) {
		std::cout << "ERROR: LoadILD -> GetSampleRate" << std::endl;
		return (false);
	} // if (globalParameters.GetSampleRate() != sampleRateInSOFAFile)
	
	bool result = sofaReader.ReadSOSFiltersFromSofa(_ildFilePath, _sosFilter);

	if (!result) {
		std::cout << "ERROR: LoadILD -> ReadSOSFiltersFromSofa [" << _ildFilePath << "]" << std::endl;
		return (false);
	} // if (!result)

	return (result);
} // bool ofApp::LoadILD(std::string _ildFilePath, std::shared_ptr<BRTServices::CSphericalSOSTable> _sosFilter) {

// ==================== audioProcess ====================

void ofApp::audioProcess(Common::CEarPair<CMonoBuffer<float>> &bufferOutput, int bufferSize) {
	int cont_src;

	if (sourcesBRT.empty()) {
		return;
	} // if (sourcesBRT.empty())

	static std::vector<CMonoBuffer<float>> sourceInput;
	static std::vector<unsigned int> wavPositions;
	static std::vector<unsigned int> endFrames;

	if (sourceInput.size() != sourcesBRT.size()) {
		sourceInput.assign(sourcesBRT.size(), CMonoBuffer<float>(bufferSize));
		wavPositions.assign(sourcesBRT.size(), 0);
		endFrames.assign(sourcesBRT.size(), 0);
	} // if (sourceInput.size() != sourcesBRT.size())

	for (cont_src = 0; cont_src < sourcesBRT.size(); cont_src++) {
		FillBuffer(sourceInput[cont_src], wavPositions[cont_src], endFrames[cont_src], samplesVectorSources[cont_src]);
		sourcesBRT[cont_src]->SetBuffer(sourceInput[cont_src]);
	} // for (cont_src = 0; cont_src < sourcesBRT.size(); cont_src++) {

	brtManager.ProcessAll();

	Common::CEarPair<CMonoBuffer<float>> bufferProcessed;
	bufferProcessed.left = CMonoBuffer<float>(bufferSize);
	bufferProcessed.right = CMonoBuffer<float>(bufferSize);
	
	listenerBRT->GetBuffers(bufferProcessed.left, bufferProcessed.right);

	bufferOutput.left += bufferProcessed.left;
	bufferOutput.right += bufferProcessed.right;
} // void ofApp::audioProcess(Common::CEarPair<CMonoBuffer<float>> &bufferOutput, int bufferSize) {

// ==================== FillBuffer ====================

void ofApp::FillBuffer(CMonoBuffer<float> &output, unsigned int &position, unsigned int &endFrame, std::vector<float> &samplesVector) {
	position = endFrame + 1;

	if (position >= samplesVector.size()) {
		position = 0;
	} // if (position >= samplesVector.size())

	endFrame = position + output.size() - 1;

	for (int i = 0; i < output.size(); i++) {

		if ((position + i) < samplesVector.size()) {
			output[i] = (samplesVector[position + i]);
		} // if ((position + i) < samplesVector.size())

		else {
			output[i] = 0.0f;
		} // else: if ((position + i) < samplesVector.size())

	} // for (int i = 0; i < output.size(); i++) {

} // void ofApp::FillBuffer(CMonoBuffer<float> &output, unsigned int &position, unsigned int &endFrame, std::vector<float> &samplesVector) {

// ==================== Spherical2Cartesians ====================

Common::CVector3 ofApp::Spherical2Cartesians(float azimuth, float elevation, float radius) {
	Common::CVector3 pos;

	pos.x = (float) radius * cos(azimuth) * cos(elevation);
	pos.y = (float) radius * sin(azimuth) * cos(elevation);
	pos.z = (float) radius * sin(elevation);

	return (pos);
} // Common::CVector3 ofApp::Spherical2Cartesians(float azimuth, float elevation, float radius) {

// ==================== RandomNumber ====================

int ofApp::RandomNumber(int min, int max) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(min, max);

    return dist(gen);
} // int ofApp::RandomNumber(int min, int max) {