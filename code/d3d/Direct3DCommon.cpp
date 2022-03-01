#include "Direct3DCommon.h"

namespace QD3D
{
    const char* HResultToString(HRESULT hr)
    {
        static char sysMsg[1024];

        int len = (int) FormatMessageA(
			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			nullptr,
			hr,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPSTR)sysMsg,
			_countof(sysMsg),
			nullptr);
        if (len == 0)
        {
            sprintf_s(sysMsg, _countof(sysMsg), "Unknown error.");
            return sysMsg;
        }

        // Strip newlines and periods from the end of the string.
        for (int i = len-1; i > 0 && (sysMsg[i] == '\n' || sysMsg[i] == '\r' || sysMsg[i] == '.'); --i)
        {
            sysMsg[i] = 0;
        }

        return sysMsg;
    }

	void D3DError(
		_In_ HRESULT hr,
		_In_ const char* msg,
		_In_ const char* srcFile,
		_In_ UINT line)
	{
		if (FAILED(hr))
		{
			Com_Error(ERR_FATAL, "A Direct3D API returned error 0x%08X: %s\n%s(%d): %s",
				hr, HResultToString(hr),
				srcFile, line, msg);
		}
	}
}
