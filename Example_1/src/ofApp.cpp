#include "ofApp.h"

#include <cctype>   // std::toupper
#include <cstdio>   // std::fopen, std::fread, std::fclose
#include <cstring>  // std::memset
#include <iostream> // std::cout, std::cin
#include <random>   // std::random_device, std::mt19937

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>

// ==================== ofApp::ofApp ====================

ofApp::ofApp(int argc, char *argv[]) {
	std::cout << std::endl;
	std::cout << "Usage: " << argv[0] << " [buffer_size] [num_src_steps] [device] [benchmark_sec]" << std::endl;
	std::cout << std::endl;

	if (argc >= 2) {
		buffer_size = std::atoi(argv[1]);

		if (buffer_size <= 0) {
			buffer_size = 2048;
		} // if (buffer_size <= 0)

	} // if (argc >= 2)

	if (argc >= 3) {
		num_src_steps = std::atoi(argv[2]);

		if (num_src_steps < 0) {
			num_src_steps = 0;
		} // if (num_src_steps < 0)
		
	} // if (argc >= 3)

	if (argc >= 4) {
		device = std::toupper(argv[3][0]) - 'A';

		if (device < 0) {
			device = 0;
		} // if (device < 0)

	} // if (argc >= 4)

	if (argc >= 5) {
		benchmark_sec = std::atoi(argv[4]);

		if (benchmark_sec <= 0) {
			benchmark_sec = -1;
		} // if (benchmark_sec <= 0)

	} // if (argc >= 5)

} // ofApp::ofApp(int argc, char *argv[])

// ==================== ofApp::setup ====================

void ofApp::setup() {
	int azi, ele, dst;
	int cont_src_steps;

	ofSetLogLevel(OF_LOG_SILENT);

	std::cout << "--- BUFFER SIZE ---" << std::endl;
	std::cout << std::endl;

	if (buffer_size == -1) {
		std::cout << "    Enter buffer size (default is 2048): ";
		std::cin >> buffer_size;
		std::cout << std::endl;
		
		if (buffer_size <= 0) {
			buffer_size = 2048;
		} // if (buffer_size <= 0)

	} // if (buffer_size == -1)

	std::cout << "    Buffer size: " << buffer_size << std::endl;
	std::cout << std::endl;

	std::cout << "--- NUMBER OF SOURCES ---" << std::endl;
	std::cout << std::endl;
	std::cout << "    This application uses a 'speech' source and a configurable number of 'steps' sources at random positions." << std::endl;
	std::cout << std::endl;

	if (num_src_steps == -1) {
		std::cout << "    Enter number of 'steps' sources: ";
		std::cin >> num_src_steps;
		std::cout << std::endl;

		if (num_src_steps < 0) {
			num_src_steps = 0;
		} // if (num_src_steps < 0)
		
	} // if (num_src_steps == -1)

	std::cout << "    Number of 'steps' sources: " << num_src_steps << std::endl;
	std::cout << "    TOTAL number of sources: " << num_src_steps + 1 << std::endl;
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

	std::cout << "--- LOADING FILES ---" << std::endl;
	std::cout << std::endl;

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

	std::cout << std::endl;
	std::cout << "--- CREATING SOURCES ---" << std::endl;
	std::cout << std::endl;

	// --- SOURCE SPEECH [OMNIDIRECTIONAL]

	// Create Source 'speech'

	brtManager.BeginSetup();
	sourcesBRT.push_back(brtManager.CreateSoundSource<BRTSourceModel::CSourceOmnidirectionalModel>("speech"));
	brtManager.EndSetup();

	// --- SOURCE -> LISTENER MODEL

	brtManager.BeginSetup();
	listenerMODEL->ConnectSoundSource(sourcesBRT.back());
	brtManager.EndSetup();

	// --- SOURCE POSITION

	// Set Source position
	Common::CTransform sourceBRT_pos = Common::CTransform();
	sourceBRT_pos.SetPosition(Spherical2Cartesians(SRC_SPEECH_AZIMUTH, SRC_SPEECH_ELEVATION, SRC_SPEECH_DISTANCE));
	sourcesBRT.back()->SetSourceTransform(sourceBRT_pos);
	std::cout << "    Source 'speech' position: [AZI: " << SRC_SPEECH_AZIMUTH << " ; ELE: " << SRC_SPEECH_ELEVATION << " ; DST: " << SRC_SPEECH_DISTANCE << "]" << std::endl;

	// --- SOURCE WAV

	// Load Source wave
	samplesVectorSources.push_back(std::vector<float>());
	LoadWav(samplesVectorSources.back(), SRC_SPEECH_FILE);

	// --- SOURCES STEPS [OMNIDIRECTIONAL]

	for (cont_src_steps = 0; cont_src_steps < num_src_steps; cont_src_steps++) {

		// Create Source 'steps'

		brtManager.BeginSetup();
		sourcesBRT.push_back(brtManager.CreateSoundSource<BRTSourceModel::CSourceOmnidirectionalModel>("steps" + std::to_string(cont_src_steps+1)));
		brtManager.EndSetup();

		// --- SOURCE -> LISTENER MODEL

		brtManager.BeginSetup();
		listenerMODEL->ConnectSoundSource(sourcesBRT.back());
		brtManager.EndSetup();

		// --- SOURCE POSITION

		// Set Source position
		Common::CTransform sourceNBRT_pos = Common::CTransform();
		azi = RandomNumber(-180, 180);
		ele = RandomNumber(-90, 90);
		dst = RandomNumber(1, 5);
		sourceNBRT_pos.SetPosition(Spherical2Cartesians(azi, ele, dst));
		sourcesBRT.back()->SetSourceTransform(sourceNBRT_pos);
		std::cout << "    Source 'steps' [#" << cont_src_steps+1 << "] position: [AZI: " << azi << " ; ELE: " << ele << " ; DST: " << dst << "]" << std::endl;

		// --- SOURCE WAV

		// Load Source wave
		samplesVectorSources.push_back(std::vector<float>());
		LoadWav(samplesVectorSources.back(), SRC_STEPS_FILE);
	} // for (cont_src_steps = 0; cont_src_steps < num_src_steps; cont_src_steps++)

	sourceInputBuffers.resize(sourcesBRT.size(), CMonoBuffer<float>(buffer_size));
	wavSamplePositions.resize(sourcesBRT.size(), 0);

	// =====================
	// === AUDIO PROCESS ===
	// =====================

  	outputBufferStereo.left.resize(buffer_size);
  	outputBufferStereo.right.resize(buffer_size);

	bufferProcessed.left.resize(buffer_size);
	bufferProcessed.right.resize(buffer_size);

	// #################### BRT SOURCE CODE END ####################

if (benchmark_sec > 0) {
		double blocksPerSec = static_cast<double>(SAMPLE_RATE) / static_cast<double>(buffer_size);
		targetBlocks = static_cast<std::size_t>(benchmark_sec * blocksPerSec);
		procTimesMs.reserve(targetBlocks);
		arrivalTimesMs.reserve(targetBlocks);
		currentBlockCount = 0;
		hasPreviousCallback = false;
		benchmarkFinished.store(false);
	}

	AudioSetup();
} // void ofApp::setup()

// ==================== ofApp::update ====================

void ofApp::update() {

	if (benchmark_sec > 0 && benchmarkFinished.load()) {
		ofExit(0);
	} // if (benchmark_sec > 0 && benchmarkFinished.load())

} // void ofApp::update()

// ==================== ofApp::exit ====================

void ofApp::exit() {
    systemSoundStream.close();
    systemSoundStream.stop();

	if (benchmark_sec > 0) {
		SaveAndPrintMetrics();
	} // if (benchmark_sec > 0)

} // void ofApp::exit()

// ==================== ofApp::audioOut ====================

void ofApp::audioOut(float *outputBuffer, int bufferSize, int nChannels) {

	if (benchmark_sec > 0) {
		if (benchmarkFinished.load()) {
			std::memset(outputBuffer, 0, sizeof(float) * bufferSize * nChannels);
			return;
		}

		auto tStart = std::chrono::steady_clock::now();
		bool isWarmupDone = (currentBlockCount >= WARMUP_BLOCKS);

		if (isWarmupDone && hasPreviousCallback && arrivalTimesMs.size() < targetBlocks) {
			double arrivalMs = std::chrono::duration<double, std::milli>(tStart - lastCallbackStart).count();
			arrivalTimesMs.push_back(arrivalMs);
		}
		lastCallbackStart = tStart;
		hasPreviousCallback = true;

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

		auto tEnd = std::chrono::steady_clock::now();
		if (isWarmupDone && procTimesMs.size() < targetBlocks) {
			double procMs = std::chrono::duration<double, std::milli>(tEnd - tStart).count();
			procTimesMs.push_back(procMs);

			if (procTimesMs.size() >= targetBlocks) {
				benchmarkFinished.store(true);
			}
		}

		currentBlockCount++;
		return;
	} // if (benchmark_sec > 0 && benchmarkFinished.load())

	float *floatOutputBuffer = (float *) outputBuffer;

	outputBufferStereo.left.Fill(bufferSize, 0.0f);
	outputBufferStereo.right.Fill(bufferSize, 0.0f);

	audioProcess(outputBufferStereo, bufferSize);

	CStereoBuffer<float> iOutput;
	iOutput.Interlace(outputBufferStereo.left, outputBufferStereo.right);

	for (auto it = iOutput.begin(); it != iOutput.end(); it++) {
		floatOutputBuffer[0] = *it;
		floatOutputBuffer = &floatOutputBuffer[1];
	} // for (auto it = iOutput.begin(); it != iOutput.end(); it++)

} // void ofApp::audioOut

// ==================== ofApp::AudioSetup ====================

void ofApp::AudioSetup(void) {
    int bak = dup(STDERR_FILENO);
    int nullFd = open("/dev/null", O_WRONLY);
    dup2(nullFd, STDERR_FILENO);
	
    std::vector<ofSoundDevice> deviceList;
    systemSoundStream.close();
    deviceList = systemSoundStream.getDeviceList();

	dup2(bak, STDERR_FILENO);

	int deviceId = GetAudioDeviceIndex(deviceList);

	dup2(nullFd, STDERR_FILENO);

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

	} // if (deviceId >= 0)

	else {
		std::cout << "Could not find any usable sound Device" << std::endl;
	} // else: if (deviceId >= 0)

	dup2(bak, STDERR_FILENO);
    close(bak);
    close(nullFd);

	if (benchmark_sec > 0) {
		std::cout << "--- BENCHMARK MODE: " << benchmark_sec << " s (" << targetBlocks << " blocks + " << WARMUP_BLOCKS << " warmup) ---" << std::endl;
		std::cout << std::endl;
	} // if (benchmark_sec > 0)

	else {
		std::cout << "--- REALTIME MODE: PRESS <CTRL+C> TO QUIT" << std::endl;
		std::cout << std::endl;
	} // else: if (benchmark_sec > 0)

} // void ofApp::AudioSetup(void)

// ==================== ofApp::audioProcess ====================

void ofApp::audioProcess(Common::CEarPair<CMonoBuffer<float>> &bufferOutput, int bufferSize) {
	int cont_src;

	for (cont_src = 0; cont_src < sourcesBRT.size(); cont_src++) {
		FillBuffer(sourceInputBuffers[cont_src], wavSamplePositions[cont_src], samplesVectorSources[cont_src]);
		sourcesBRT[cont_src]->SetBuffer(sourceInputBuffers[cont_src]);
	}

	brtManager.ProcessAll();
	listenerBRT->GetBuffers(bufferProcessed.left, bufferProcessed.right);

	bufferOutput.left += bufferProcessed.left;
	bufferOutput.right += bufferProcessed.right;
} // void ofApp::audioProcess(Common::CEarPair<CMonoBuffer<float>> &bufferOutput, int bufferSize)

// ==================== ofApp::GetAudioDeviceIndex ====================

int ofApp::GetAudioDeviceIndex(std::vector<ofSoundDevice> list) {
	int numberOfAudioDevices = list.size();
	std::cout << std::endl;
	std::cout << "--- DEVICES ---" << std::endl;
	std::cout << std::endl;

	for (int i = 0; i < numberOfAudioDevices; i++) {
		std::cout << "    DEVICE '" << char('A'+i) << "': " << list[i].name << " (ID: " << list[i].deviceID << ")";

		if (list[i].outputChannels == 0) {
			std::cout << "[NO OUTPUT DEVICE!]";
		} // if (list[i].outputChannels == 0)

		std::cout << std::endl;
	} // for (int i = 0; i < numberOfAudioDevices; i++)

	std::cout << std::endl;
	
	char selectedAudioDevice_key;
	int selectedAudioDevice;

	if (device == -1) {

		do {
			std::cout << "    Select audio device: ";
			std::cin >> selectedAudioDevice_key;
			selectedAudioDevice_key = std::toupper(selectedAudioDevice_key);
			std::cin.clear();
			std::cin.ignore(INT_MAX, '\n');
			selectedAudioDevice = selectedAudioDevice_key - 'A';
		} while ((selectedAudioDevice < 0) || (selectedAudioDevice >= numberOfAudioDevices));

		std::cout << std::endl;
	} // if (device == -1)

	else {
		
		if (device > (numberOfAudioDevices-1)) {
			selectedAudioDevice = 0;
		} // if (device > (numberOfAudioDevices-1))
		
		else {
			selectedAudioDevice = device;
		} // else: if (device > (numberOfAudioDevices-1))

	} // else: if (device == -1)

	std::cout << "    Audio device: " << list[selectedAudioDevice].name << " (ID: " << list[selectedAudioDevice].deviceID << ")" << std::endl;
	std::cout << std::endl;

	for (int c = 0; c < numberOfAudioDevices; c++) {
		ofSoundDevice &dev = list[c];

		if ((dev.outputChannels >= 0) && c == selectedAudioDevice) {
			return c;
		} // if ((dev.outputChannels >= 0) && c == selectedAudioDevice)

	} // for (int c = 0; c < numberOfAudioDevices; c++)

	for (int c = 0; c < numberOfAudioDevices; c++) {
		ofSoundDevice &dev = list[c];

		if ((dev.outputChannels >= 0) && dev.isDefaultOutput) {
			return c;
		} // if ((dev.outputChannels >= 0) && dev.isDefaultOutput)

	} // for (int c = 0; c < numberOfAudioDevices; c++)

	return -1;
} // int ofApp::GetAudioDeviceIndex(std::vector<ofSoundDevice> list)

// ==================== ofApp::LoadWav ====================

void ofApp::LoadWav(std::vector<float> &samplesVector, const char *stringIn) {
	struct WavHeader {
		char		fill[40];
		uint32_t	bytesCount;
	} wavHeader;

	FILE* wavFile = std::fopen(stringIn, "rb");

	if (!wavFile) {
		std::cout << "    Error loading WAV file [" << stringIn << "]" << std::endl;
		return;
	} // if (!wavFile)

	std::cout << "    Loading WAV File [" << stringIn << "]: ";
	std::fread(&wavHeader, sizeof(wavHeader), 1, wavFile);
	std::fseek(wavFile, sizeof(wavHeader), SEEK_SET);

	unsigned int samplesCount = wavHeader.bytesCount / 2;
	int16_t *sample = new int16_t[samplesCount];
	std::memset(sample, 0, sizeof(int16_t) * samplesCount);

	uint8_t *byteSample = new uint8_t[2 * samplesCount];
	std::memset(byteSample, 0, sizeof(uint8_t) * 2 * samplesCount);

	std::fread(byteSample, 1, 2 * samplesCount, wavFile);
	std::fclose(wavFile);

	for (int i = 0; i < samplesCount; i++) {
		sample[i] = int16_t(byteSample[2 * i] | byteSample[2 * i + 1] << 8);
	} // for (int i = 0; i < samplesCount; i++)
	
	samplesVector.reserve(samplesCount);

	for (int i = 0; i < samplesCount; i++)
		samplesVector.push_back((float)sample[i] / (float)INT16_MAX);

	delete[] sample;
	delete[] byteSample;

	std::cout << "successful!" << std::endl; 
} // void ofApp::LoadWav(std::vector<float> &samplesVector, const char *stringIn)

// ==================== ofApp::LoadSofaFile ====================

bool ofApp::LoadSofaFile(const std::string &_filePath, std::shared_ptr<BRTServices::CSphericalInterpolatedFIRTable> hrtf) {
	BRTReaders::CSOFAReader sofaReader;

	int sampleRateInSOFAFile = sofaReader.GetSampleRateFromSofa(_filePath);

	if (sampleRateInSOFAFile == -1) {
		std::cout << "    Error loading SOFA file [" << _filePath << "]" << std::endl;
		return false;
	} // if (sampleRateInSOFAFile == -1)

	if (globalParameters.GetSampleRate() != sampleRateInSOFAFile) {
		std::cout << "    The sample rate in SOFA file doesn't match the configuration." << std::endl;
		return false;
	} // if (globalParameters.GetSampleRate() != sampleRateInSOFAFile)

	std::cout << "    Loading SOFA File [" << _filePath << "]: ";
	bool result = sofaReader.ReadHRTFFromSofa(_filePath, "", hrtf, HRTF_RESAMPLING_STEP, BRTServices::TEXTRAPOLATION_METHOD::nearest_point);

	if (result) {
		std::cout << "successful!" << std::endl;            
		return true;
	} // if (result)

	else {
		std::cout << "ERROR!" << std::endl;
		return false;
	} // else: if (result)
	
} // bool ofApp::LoadSofaFile(const std::string &_filePath, std::shared_ptr<BRTServices::CSphericalInterpolatedFIRTable> hrtf)

// ==================== ofApp::LoadILD ====================

bool ofApp::LoadILD(std::string _ildFilePath, std::shared_ptr<BRTServices::CSphericalSOSTable> _sosFilter) {
	BRTReaders::CSOFAReader sofaReader;
	
	int sampleRateInSOFAFile = sofaReader.GetSampleRateFromSofa(_ildFilePath);

	if (sampleRateInSOFAFile == -1) {
		std::cout << "    Error loading ILD file [" << _ildFilePath << "]" << std::endl;
		return false;
	} // if (sampleRateInSOFAFile == -1)

	if (globalParameters.GetSampleRate() != sampleRateInSOFAFile) {
		std::cout << "    The sample rate in ILD file doesn't match the configuration." << std::endl;
		return false;
	} // if (globalParameters.GetSampleRate() != sampleRateInSOFAFile)
	
	std::cout << "    Loading ILD File [" << _ildFilePath << "]: ";
	bool result = sofaReader.ReadSOSFiltersFromSofa(_ildFilePath, _sosFilter);

	if (result) {
		std::cout << "successful!" << std::endl;            
		return true;
	} // if (result)

	else {
		std::cout << "ERROR!" << std::endl;
		return false;
	} // else: if (result)

} // bool ofApp::LoadILD(std::string _ildFilePath, std::shared_ptr<BRTServices::CSphericalSOSTable> _sosFilter)

// ==================== ofApp::FillBuffer ====================

void ofApp::FillBuffer(CMonoBuffer<float> &output, unsigned int &position, std::vector<float> &samplesVector) {

	if (position >= samplesVector.size()) {
		position = 0;
	} // if (position >= samplesVector.size())

	for (int i = 0; i < output.size(); i++) {

		if ((position + i) < samplesVector.size()) {
			output[i] = (samplesVector[position + i]);
		} // if ((position + i) < samplesVector.size())

		else {
			output[i] = 0.0f;
		} // else: if ((position + i) < samplesVector.size())

	}  // for (int i = 0; i < output.size(); i++)

	position += output.size();
} // void ofApp::FillBuffer(CMonoBuffer<float> &output, unsigned int &position, unsigned int &endFrame, std::vector<float> &samplesVector)

// ==================== ofApp::Spherical2Cartesians ====================

Common::CVector3 ofApp::Spherical2Cartesians(float azimuth, float elevation, float radius) {
	Common::CVector3 pos;

	pos.x = (float) radius * std::cos(azimuth) * std::cos(elevation);
	pos.y = (float) radius * std::sin(azimuth) * std::cos(elevation);
	pos.z = (float) radius * std::sin(elevation);

	return pos;
} // Common::CVector3 ofApp::Spherical2Cartesians(float azimuth, float elevation, float radius)

// ==================== ofApp::RandomNumber ====================

int ofApp::RandomNumber(int min, int max) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(min, max);
    return dist(gen);
} // int ofApp::RandomNumber(int min, int max)

// ==================== ofApp::SaveAndPrintMetrics ====================

void ofApp::SaveAndPrintMetrics() {
	if (procTimesMs.empty()) {
		std::cout << "\nNot enough audio blocks recorded to compute statistics." << std::endl;
		return;
	}

	double deadlineMs = (static_cast<double>(buffer_size) / static_cast<double>(SAMPLE_RATE)) * 1000.0;

	// Helper lambda to compute stats over all recorded blocks
	auto computeStats = [](const std::vector<double>& data, double thresholdMs) {
		struct Stats {
			double mean = 0.0, stdDev = 0.0, minVal = 0.0, p99 = 0.0, maxVal = 0.0;
			std::size_t count = 0, overruns = 0;
		} s;

		if (data.empty()) return s;

		std::vector<double> valid = data;
		s.count = valid.size();

		double sum = 0.0;
		for (double v : valid) {
			sum += v;
			if (thresholdMs > 0.0 && v > thresholdMs) s.overruns++;
		}
		s.mean = sum / s.count;

		double sqSum = 0.0;
		for (double v : valid) {
			sqSum += (v - s.mean) * (v - s.mean);
		}
		s.stdDev = std::sqrt(sqSum / s.count);

		std::sort(valid.begin(), valid.end());
		s.minVal = valid.front();
		s.maxVal = valid.back();
		std::size_t p99Idx = static_cast<std::size_t>(0.99 * (s.count - 1));
		s.p99 = valid[p99Idx];

		return s;
	};

	auto procStats = computeStats(procTimesMs, deadlineMs);
	auto arrStats  = computeStats(arrivalTimesMs, 0.0);

	double meanRtLoadPct = (procStats.mean / deadlineMs) * 100.0;
	double p99RtLoadPct  = (procStats.p99 / deadlineMs) * 100.0;
	double maxRtLoadPct  = (procStats.maxVal / deadlineMs) * 100.0;

	// 1. Print report to console
	std::cout << std::fixed << std::setprecision(3);
	std::cout << "\n==================== PERFORMANCE REPORT ====================" << std::endl;
	std::cout << "    Buffer size: " << buffer_size << " samples | Sample rate: " << SAMPLE_RATE << " Hz" << std::endl;
	std::cout << "    Total sources: " << sourcesBRT.size() << " | Recorded blocks: " << procStats.count << std::endl;
	std::cout << "    Audio Deadline (T_deadline): " << deadlineMs << " ms" << std::endl;
	std::cout << "------------------------------------------------------------" << std::endl;
	std::cout << "    [t_proc] Processing Time:" << std::endl;
	std::cout << "      Mean: " << procStats.mean << " ms (" << meanRtLoadPct << "% RT Load)" << std::endl;
	std::cout << "      StdDev (Jitter): " << procStats.stdDev << " ms" << std::endl;
	std::cout << "      P99:  " << procStats.p99 << " ms (" << p99RtLoadPct << "% RT Load)" << std::endl;
	std::cout << "      Max:  " << procStats.maxVal << " ms (" << maxRtLoadPct << "% RT Load)" << std::endl;
	std::cout << "      Overruns (t_proc > T_deadline): " << procStats.overruns << " / " << procStats.count << std::endl;
	std::cout << "------------------------------------------------------------" << std::endl;
	std::cout << "    [dt_arrival] Callback Interval:" << std::endl;
	std::cout << "      Mean: " << arrStats.mean << " ms | StdDev (Jitter): " << arrStats.stdDev << " ms" << std::endl;
	std::cout << "      Min:  " << arrStats.minVal << " ms | Max: " << arrStats.maxVal << " ms" << std::endl;
	std::cout << "============================================================" << std::endl;

	// 2. Export self-contained block-by-block CSV (with commented summary header)
	std::string csvName = "metrics_buf" + std::to_string(buffer_size) + "_src" + std::to_string(sourcesBRT.size()) + ".csv";
	std::ofstream csvFile(csvName);
	
	if (csvFile.is_open()) {
		csvFile << std::fixed << std::setprecision(4);
		csvFile << "# buffer_size=" << buffer_size << ", sample_rate=" << SAMPLE_RATE
		        << ", total_sources=" << sourcesBRT.size() << ", recorded_blocks=" << procStats.count
		        << ", deadline_ms=" << deadlineMs << "\n";
		csvFile << "# t_proc_mean_ms=" << procStats.mean << ", t_proc_std_ms=" << procStats.stdDev
		        << ", t_proc_p99_ms=" << procStats.p99 << ", t_proc_max_ms=" << procStats.maxVal
		        << ", overruns=" << procStats.overruns << "\n";
		csvFile << "# rt_load_mean_pct=" << meanRtLoadPct << ", rt_load_p99_pct=" << p99RtLoadPct
		        << ", rt_load_max_pct=" << maxRtLoadPct << "\n";
		csvFile << "# dt_arrival_mean_ms=" << arrStats.mean << ", dt_arrival_std_ms=" << arrStats.stdDev
		        << ", dt_arrival_min_ms=" << arrStats.minVal << ", dt_arrival_max_ms=" << arrStats.maxVal << "\n";

		csvFile << "block,t_proc_ms,dt_arrival_ms,rt_load_pct\n";
		std::size_t n = std::min(procTimesMs.size(), arrivalTimesMs.size());
		for (std::size_t i = 0; i < n; ++i) {
			csvFile << i << ","
			        << procTimesMs[i] << ","
			        << arrivalTimesMs[i] << ","
			        << (procTimesMs[i] / deadlineMs) * 100.0 << "\n";
		}
		std::cout << "    Detailed CSV saved to: " << csvName << std::endl;
	}

	// 3. Append single summary row to global summary_results.csv
	const std::string summaryName = "summary_results.csv";
	bool fileExists = std::ifstream(summaryName).good();
	std::ofstream summaryFile(summaryName, std::ios::app);

	if (summaryFile.is_open()) {
		summaryFile << std::fixed << std::setprecision(4);
		if (!fileExists) {
			summaryFile << "buffer_size,sample_rate,total_sources,recorded_blocks,deadline_ms,"
			            << "t_proc_mean_ms,t_proc_std_ms,t_proc_p99_ms,t_proc_max_ms,"
			            << "rt_load_mean_pct,rt_load_p99_pct,rt_load_max_pct,overruns,"
			            << "dt_arrival_mean_ms,dt_arrival_std_ms,dt_arrival_min_ms,dt_arrival_max_ms\n";
		}
		summaryFile << buffer_size << ","
		            << SAMPLE_RATE << ","
		            << sourcesBRT.size() << ","
		            << procStats.count << ","
		            << deadlineMs << ","
		            << procStats.mean << ","
		            << procStats.stdDev << ","
		            << procStats.p99 << ","
		            << procStats.maxVal << ","
		            << meanRtLoadPct << ","
		            << p99RtLoadPct << ","
		            << maxRtLoadPct << ","
		            << procStats.overruns << ","
		            << arrStats.mean << ","
		            << arrStats.stdDev << ","
		            << arrStats.minVal << ","
		            << arrStats.maxVal << "\n";
		std::cout << "    Summary appended to: " << summaryName << std::endl;
	}

} // void ofApp::SaveAndPrintMetrics()