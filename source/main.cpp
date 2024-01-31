#include <iostream>
#include "../dshowcapture.hpp"
#include <Windows.h>
#include <sstream>
#include "main.hpp"
#include <regex>
#include <iomanip>


#ifdef USE_FOR_DLLS
#define USE_FOR_DLL __declspec(dllexport)
#else
#define USE_FOR_DLL
#endif
using namespace std;
using namespace DShow;

struct CurrVideoInfo {
	Device cam;
	VideoConfig config;
	int cX = 0;
	int cY = 0;
	long long interval;
	unsigned char* data = nullptr;
	size_t size = 0;
	size_t realSize = 0;
};


CurrVideoInfo* ptr = new CurrVideoInfo;

int jsonLength = 0;

stringstream jsonstream;

CRITICAL_SECTION isUsing;

bool updated = false;

size_t expectedSize = 0;


vector<VideoDevice> devices;

int main() {
	int cinmsg = 1;

	MakeJson();

	const int width = 640;
	const int height = 480;
	const int preparedSize = width * height * 4;
	unsigned char* data = new unsigned char[preparedSize];

	StartCamConfig();
	int temp = OpenCamera(2, width, height, 333667, VideoFormat::XRGB, false);
	if (temp != 1) {

		cout << "Camera not valid -" << temp << endl;
		return 1;
	}
	else {
		cout << "IsFlipped : " << ptr->config.cy_flip << endl;
		cout << "InternalFormat : " << static_cast<int>(ptr->config.internalFormat) << endl;
		cout << "Format : " << static_cast<int>(ptr->config.format) << endl;
	}
	for (size_t i = 0; i < 3000000000; i++)
	{
		size_t tem = GetFrame(data, preparedSize);

		cout << "size  : " << tem << endl;
	}
	TerminateCamConfig();
	return 1;
}

void Capture(const VideoConfig& config, unsigned char* data, size_t size, long long startTime, long long stopTime, long rotation) {
	__try {
		EnterCriticalSection(&isUsing);
	}

	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return;
	}
	__try {
		if (size > 0 && size <= ptr->size && ptr->data != nullptr)
		{
			memcpy(ptr->data, data, size);
			ptr->realSize = size;
			updated = true;
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		LeaveCriticalSection(&isUsing);
		return;
	}
	LeaveCriticalSection(&isUsing);

}

extern "C" {

	USE_FOR_DLL void StartCamConfig() {
		InitializeCriticalSection(&isUsing);
		ptr = new CurrVideoInfo();
		ptr->cX = -1;
		ptr->cY = -1;
		ptr->interval = -1;
		ptr->data = nullptr;
		ptr->size = 0;
		expectedSize = 0;
		return;
	}

	USE_FOR_DLL void TerminateCamConfig() {
		ptr->cam.Stop();
		DeleteCriticalSection(&isUsing);
		delete ptr->data;
		ptr->data = nullptr;
		ptr->size = 0;
		updated = false;
		return;
	}

	USE_FOR_DLL int OpenCamera(int devID, int cX, int cY, long long interval, VideoFormat format, bool cyFlip) {

		ptr->config.name = devices.at(devID).name;
		ptr->config.path = devices.at(devID).path;
		ptr->config.useDefaultConfig = false;
		ptr->config.callback = Capture;
		ptr->config.cx = cX;
		ptr->config.cy_abs = cY;
		ptr->config.cy_flip = cyFlip;
		ptr->config.frameInterval = interval;
		ptr->config.internalFormat = format;
		ptr->config.format = VideoFormat::Any;

		if (!ptr->cam.ResetGraph()) return 5;
		if (!ptr->cam.Valid()) return 6;



		bool configSuccess = ptr->cam.SetVideoConfig(&ptr->config);
		if (!configSuccess) return 7;
		bool filterSuccess = ptr->cam.ConnectFilters();
		if (!filterSuccess) return 8;
		bool startSuccess = ptr->cam.Start() == Result::Success;
		if (!startSuccess) return 9;

		ptr->cX = cX;
		ptr->cY = cY;
		ptr->interval = interval;

		return ptr->cam.Valid();
	}

	USE_FOR_DLL int StopCamera() {
		TerminateCamConfig();
		return 1;
	}

	USE_FOR_DLL size_t VideoCapture(unsigned char* img, int preparedSize) {
		if (ptr->realSize <= preparedSize) {
			memcpy(img, ptr->data, ptr->realSize);
			return ptr->realSize;
		}
		else {
			return 0;
		}

	}

	USE_FOR_DLL void MakeJson() {
		CoInitialize(0);
		Device::EnumVideoDevices(devices);
		jsonstream.str("");
		//cache
		int devicesSize = devices.size();
		VideoDevice nowDevice;
		VideoInfo nowVideoInfo;
		vector<VideoInfo> caps;

		jsonstream << "{\"webcamDevices\" : [";
		for (auto nowDevice = devices.begin(); nowDevice != devices.end(); nowDevice++) {
			jsonstream << "{";

			string name = HandlingControl(Wstring2String(nowDevice->name));
			cout << name << endl;

			NstringVAddStream(jsonstream, "Name", (name), true);
			jsonstream << "\"VideoInfo\" : [";
			caps = nowDevice->caps;
			for (auto nowVideoInfo = caps.begin(); nowVideoInfo != caps.end(); nowVideoInfo++) {
				jsonstream << "{";
				NintVAddStream(jsonstream, "MinCX", nowVideoInfo->minCX, true);
				NintVAddStream(jsonstream, "MinCY", nowVideoInfo->minCY, true);
				NintVAddStream(jsonstream, "MaxCX", nowVideoInfo->maxCX, true);
				NintVAddStream(jsonstream, "MaxCY", nowVideoInfo->maxCY, true);
				NintVAddStream(jsonstream, "GranularityCX", nowVideoInfo->granularityCX, true);
				NintVAddStream(jsonstream, "GranularityCY", nowVideoInfo->granularityCY, true);
				NlonglongVAddStream(jsonstream, "MinInterval", nowVideoInfo->minInterval, true);
				NlonglongVAddStream(jsonstream, "MaxInterval", nowVideoInfo->maxInterval, true);
				NintVAddStream(jsonstream, "Format", static_cast<int>(nowVideoInfo->format), false);
				jsonstream << "}";
				if (nowVideoInfo != caps.end() - 1)
					jsonstream << ",";
			}
			jsonstream << "]}";
			if (nowDevice != devices.end() - 1)
				jsonstream << ",";
		}
		jsonstream << "]}";
		jsonLength = jsonstream.str().size();

		return;
	}

	string HandlingControl(string str) {
		std::ostringstream o;
		for (auto c = str.cbegin(); c != str.cend(); c++) {
			if (*c == '"' || *c == '\\' || ('\x00' <= *c && *c <= '\x1f')) {
				o << "\\u"
					<< std::hex << std::setw(4) << std::setfill('0') << (int)*c;
			}
			else {
				o << *c;
			}
		}
		return o.str();
	}

	USE_FOR_DLL int GetMadeJsonSize() {
		// Size for null char
		return jsonLength + 1;
	}

	USE_FOR_DLL char* GetJson(int jsonSize)
	{
		char* data = new char[jsonSize];
		strcpy(data, jsonstream.str().c_str());
		return data;
	}

	string Wstring2String(wstring widestring) {
		int stringSize = WideCharToMultiByte(CP_UTF8, WC_COMPOSITECHECK, widestring.c_str(), int(widestring.length() + 1), NULL, 0, NULL, NULL);
		char* newString = new char[stringSize];
		WideCharToMultiByte(CP_UTF8, WC_COMPOSITECHECK, widestring.c_str(), int(widestring.length() + 1), &newString[0], stringSize, NULL, NULL);

		return newString;
	}


	void NstringVAddStream(stringstream& jsonstream, string name, string value, bool addcomma) {
		if (addcomma) {
			jsonstream << "\"" << name << "\" : \"" << value << "\" ," << endl;
		}
		else {
			jsonstream << "\"" << name << "\" : \"" << value;
		}
		return;
	}
	void NintVAddStream(stringstream& jsonstream, string name, int value, bool addcomma) {
		if (addcomma) {
			jsonstream << "\"" << name << "\" : " << value << "," << endl;
		}
		else {
			jsonstream << "\"" << name << "\" : " << value;
		}
		return;
	}
	void NlonglongVAddStream(stringstream& jsonstream, string name, long long value, bool addcomma) {
		if (addcomma) {
			jsonstream << "\"" << name << "\" : " << value << "," << endl;
		}
		else {
			jsonstream << "\"" << name << "\" : " << value;
		}
		return;
	}


	USE_FOR_DLL int MakeConfig(int id, int width, int height, long long interval, int format) {

		MakeJson();

		StartCamConfig();

		int temp = OpenCamera(id, width, height, interval, static_cast<VideoFormat>(format), false);

		return temp;
	}

	USE_FOR_DLL size_t GetFrame(unsigned char* data, int preparedSize) {
		expectedSize = preparedSize;

		EnterCriticalSection(&isUsing);
		if (ptr->size < expectedSize) {
			ptr->data = new unsigned char[expectedSize];
			ptr->size = expectedSize;
		}

		if (updated) {
			size_t tem = VideoCapture(data, preparedSize);
			updated = false;
			LeaveCriticalSection(&isUsing);
			return tem;
		}

		else {
			LeaveCriticalSection(&isUsing);
			return 0;
		}

	}
	USE_FOR_DLL bool IsUpdated() {
		return updated;
	}
	USE_FOR_DLL int GetFormat() {
		return static_cast<int>(ptr->config.internalFormat);
	}
	USE_FOR_DLL int GetCX() {
		return ptr->config.cx;
	}
	USE_FOR_DLL int GetCY() {
		return ptr->config.cy_abs;
	}

}