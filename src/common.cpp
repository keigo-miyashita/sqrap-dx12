#include <common.hpp>

//extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = D3D12_SDK_VERSION_MACRO; }
//extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = D3D12_SDK_PATH_MACRO; }

void DebugOutputFormatString(const char* format, ...)
{
#ifdef _DEBUG
	va_list valist;
	va_start(valist, format);
	printf(format, valist);
	va_end(valist);
#endif
}