#include "pch.h"
#include "Shader.h"

#define HLSL_PATH "hlsl"
#define HLSL_EXTENSION "cso"

namespace QD3D12
{
	Shader::Shader()
	{
		ZeroMemory(&m_bc, sizeof(m_bc));
	}

	Shader::~Shader()
	{
		Destroy();
	}

	void Shader::Init(const char* name)
	{
		char filename[MAX_PATH];
		sprintf_s(filename, "%s/%s.%s", HLSL_PATH, name, HLSL_EXTENSION);

		void* blob = nullptr;
		m_bc.BytecodeLength = FS_ReadFile(filename, &blob);
		if (m_bc.BytecodeLength <= 0 || !blob) {
			RI_Error(ERR_FATAL, "Failed to load shader '%s'\n", name);
		}

		// TODO: go through Q3's memory allocator?
		m_bc.pShaderBytecode = new BYTE[m_bc.BytecodeLength];
		Com_Memcpy((void*) m_bc.pShaderBytecode, blob, m_bc.BytecodeLength);

		FS_FreeFile(blob);
	}

	void Shader::Destroy()
	{
		SAFE_DELETE_ARRAY(m_bc.pShaderBytecode);
		ZeroMemory(&m_bc, sizeof(m_bc));
	}
}