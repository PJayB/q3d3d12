#pragma once

#ifndef __cplusplus
#	error Arr, don't be includin' this from C, matey
#endif

extern "C" {
#   include "../renderer/tr_local.h"
#   include "../renderer/tr_layer.h"
#   include "../qcommon/qcommon.h"
}

#define D3D_PUBLIC  extern "C"

#include <dxgi1_4.h>

#include <wrl.h>
#include <atomic>

#define QDXGIDevice							IDXGIDevice2		
#define QDXGIAdapter						IDXGIAdapter		

#define QDXGIFactory						IDXGIFactory4	
#define QDXGISwapChain						IDXGISwapChain2	

#ifndef SAFE_RELEASE
#	undef SAFE_RELEASE // uses templated function below
#endif

#ifndef SAFE_DELETE
#	define SAFE_DELETE(x) if(x) { delete x; x = nullptr; }
#endif

#ifndef SAFE_DELETE_ARRAY
#	define SAFE_DELETE_ARRAY(x) if(x) { delete [] x; x = nullptr; }
#endif

#ifndef ASSERT
#	include <assert.h>
#	define ASSERT(x)	assert(x)
#endif

#ifndef UNUSED
#   define UNUSED(x)    (void)(x)
#endif

#ifndef PROFILE_FUNC
#	define PROFILE_FUNC()
#endif

#define DEBUG_BREAK() __debugbreak()

#define COM_ERROR_TRAP(x)	{ HRESULT _hr = (x); if(FAILED(_hr)) { DEBUG_BREAK(); QD3D::D3DError(_hr, #x, __FILE__, __LINE__); }}

template<class T> __forceinline T* ADDREF(T* object)
{
	object->AddRef();
	return object;
}

template<class T> inline T* SAFE_ADDREF(T* object)
{
	if (object) { object->AddRef(); }
	return object;
}

template<class T> inline unsigned int SAFE_RELEASE(T*& object)
{
	unsigned int rc = 0;
	if (object)
	{
		rc = object->Release();
		object = nullptr;
	}
	return rc;
}

// Makes sure we don't release other if ptr == other
template<class T> inline void SAFE_SWAP(T*& ptr, T* other)
{
	if (ptr != other)
	{
		if (other) { other->AddRef(); }
		if (ptr) { ptr->Release(); }
		ptr = other;
	}
}

template<class T> inline void CHECK_RELEASE(T*& object)
{
	if (object)
	{
		unsigned int rc = object->Release();
		ASSERT(rc == 0);

		object = nullptr;
	}
}

namespace QD3D
{
	//----------------------------------------------------------------------------
	// Utility function for aligning a single UINT
	//----------------------------------------------------------------------------
	template<class T> T Align(T x, T alignment)
	{
		--alignment;
		return (x + alignment) & ~alignment;
	}

	//----------------------------------------------------------------------------
	// Returns a string representing an HRESULT (static: do not cache, not thread 
	// safe)
	//----------------------------------------------------------------------------
	const char* HResultToString(
        _In_ HRESULT hr);

	//----------------------------------------------------------------------------
	// Throw a descriptive fatal error if HR is a failure code
	//----------------------------------------------------------------------------
	void D3DError(
		_In_ HRESULT hr,
		_In_ const char* msg,
		_In_ const char* srcFile,
		_In_ UINT line);
}
