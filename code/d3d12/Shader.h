#pragma once

#include "D3D12Core.h"

namespace QD3D12
{
	class Shader
	{
	private:
		D3D12_SHADER_BYTECODE m_bc;

	public:
		Shader();
		~Shader();

		void Init(const char* name);
		void Destroy();

		const D3D12_SHADER_BYTECODE& ByteCode() const { return m_bc; }
		const BYTE* Data() const { return reinterpret_cast<const BYTE*>(m_bc.pShaderBytecode); }
		SIZE_T Size() const { return m_bc.BytecodeLength; }
	};
}
