#include "pch.h"

namespace QD3D12
{
	HRESULT SetResourceName(
		_In_ ID3D12Resource* resource,
		_In_ const char* fmt,
		_In_ ...)
	{
#if QD3D12_PROFILE_NAMES
		va_list args;
		CHAR tmp[2048];

		va_start(args, fmt);
		vsprintf_s(tmp, _countof(tmp), fmt, args);
		va_end(args);

		WCHAR tmp2[2048];
		swprintf_s(tmp2, _countof(tmp2), L"%S", tmp);

		return resource->SetName(tmp2);
#else
		return E_NOTIMPL;
#endif
	}

	HRESULT SetResourceName(
		_In_ ID3D12Resource* resource,
		_In_ const wchar_t* fmt,
		_In_ ...)
	{
#if QD3D12_PROFILE_NAMES
		va_list args;
		WCHAR tmp[2048];

		va_start(args, fmt);
		vswprintf_s(tmp, _countof(tmp), fmt, args);
		va_end(args);

		return resource->SetName(tmp);
#else
		return E_NOTIMPL;
#endif
	}

	void TransitionResource(
		_In_ ID3D12GraphicsCommandList* commandList,
		_In_ ID3D12Resource* resource,
		_In_ D3D12_RESOURCE_STATES stateBefore,
		_In_ D3D12_RESOURCE_STATES stateAfter)
	{
        D3D12_RESOURCE_BARRIER desc;
        ZeroMemory( &desc, sizeof(desc) );
		desc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		desc.Transition.pResource = resource;
		desc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		desc.Transition.StateBefore = stateBefore;
		desc.Transition.StateAfter = stateAfter;

		commandList->ResourceBarrier(1, &desc);
	}
}
