#include <iostream>
#include "../dshowcapture.hpp"
#include <Windows.h>
#include <sstream>

#ifdef USE_FOR_DLLS
#define USE_FOR_DLL __declspec(dllexport)
#else
#define USE_FOR_DLL
#endif

using namespace std;
using namespace DShow;



extern "C" {
	USE_FOR_DLL void StartCamConfig();
	USE_FOR_DLL void TerminateCamConfig();
	USE_FOR_DLL int OpenCamera(int devID, int cX, int cY, long long interval, VideoFormat format, bool cyFlip);
	USE_FOR_DLL int StopCamera();
	USE_FOR_DLL size_t VideoCapture(unsigned char* img, int preparedSize);
	string HandlingControl(string str);
	USE_FOR_DLL void MakeJson();
	USE_FOR_DLL int GetMadeJsonSize();
	USE_FOR_DLL char* GetJson(int jsonSize);

	string Wstring2String(wstring widestring);

	string Bool2String(bool boolean);

	void NstringVAddStream(stringstream& jsonstream, string name, string value, bool addcomma);

	void NintVAddStream(stringstream& jsonstream, string name, int value, bool addcomma);

	void NlonglongVAddStream(stringstream& jsonstream, string name, long long value, bool addcomma);

	USE_FOR_DLL int MakeConfig(int id, int width, int height, long long interval, int format);

	USE_FOR_DLL size_t GetFrame(unsigned char* data, int preparedSize);
	USE_FOR_DLL bool IsUpdated();

	USE_FOR_DLL void TerminateConfig();

	USE_FOR_DLL int GetFormat();
	USE_FOR_DLL int GetCX();
	USE_FOR_DLL int GetCX();


}