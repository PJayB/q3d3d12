#pragma once

#include "D3D12Core.h"

namespace QD3D12 
{
	class ConstantBuffer
	{
	private:
		D3D12_GPU_VIRTUAL_ADDRESS m_addr;
		
	public:
		D3D12_GPU_VIRTUAL_ADDRESS Write(
			_In_reads_(size) LPCVOID data,
			_In_ UINT64 size);

		static D3D12_GPU_VIRTUAL_ADDRESS GlobalWrite(
			_In_reads_(size) LPCVOID data,
			_In_ UINT64 size);

		inline D3D12_GPU_VIRTUAL_ADDRESS Address() const { return m_addr; }
	};
}
