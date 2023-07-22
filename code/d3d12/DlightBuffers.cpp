#include "DlightBuffers.h"

namespace QD3D12
{
	void DlightBuffers::Init()
	{
		m_arrayScratch.Init(sizeof(DLIGHT) * MAX_DLIGHTS * BufferCapacity());
	}

	void DlightBuffers::Destroy()
	{
		m_arrayScratch.Destroy();
	}

	void DlightBuffers::Reset()
	{
		m_arrayScratch.Reset();
	}

	DlightBufferBindings::DlightBufferBindings()
		: ModLights(0)
		, AddLights(0)
	{
	}

	void DlightBufferBindings::Upload(
		_In_ const shaderCommands_t* input,
		_In_ DlightBuffers* dlights)
	{
		// Count the dlights
		PROFILE_FUNC();
		UINT modDlightCount = 0;
		UINT addDlightCount = 0;
		for (int l = 0; l < input->dlightCount; ++l)
		{
			const dlightProjectionInfo_t* light = &input->dlightInfo[l];
			if (input->dlightBits & (1 << l))
			{
				if (light->additive)
					addDlightCount++;
				else
					modDlightCount++;
			}
		}

		// Update the constant buffer with the counts
		SetDlightCounts(addDlightCount, modDlightCount);

		// Map the light buffers and copy in the lighting data
		DLIGHT* modLights = nullptr;
		DLIGHT* addLights = nullptr;
		if (modDlightCount)
		{
			ModLights = dlights->DlightScratch()->Reserve(
				QD3D12_CONSTANT_BUFFER_ALIGNMENT,
				sizeof(DLIGHT) * modDlightCount,
				(void**) &modLights);
		}
		if (addDlightCount) 
		{
			AddLights = dlights->DlightScratch()->Reserve(
				QD3D12_CONSTANT_BUFFER_ALIGNMENT,
				sizeof(DLIGHT) * addDlightCount,
				(void**) &addDlightCount);
		}

		for (int l = 0; l < input->dlightCount; ++l)
		{
			if (input->dlightBits & (1 << l))
			{
				const dlightProjectionInfo_t* cpuLight = &input->dlightInfo[l];

				DLIGHT* gpuLight = NULL;
				if (cpuLight->additive) {
					// Add to the additive light buffer
					gpuLight = &addLights[--addDlightCount];
				}
				else {
					// Add to the modulated light buffer
					gpuLight = &modLights[--modDlightCount];
				}

				VectorCopy(cpuLight->origin, gpuLight->Origin);
				VectorCopy(cpuLight->color, gpuLight->Color);
				gpuLight->Radius = cpuLight->radius;
			}
		}
	}

	void DlightBufferBindings::Zero(
		_In_ DlightBuffers* dlights)
	{
		SetDlightCounts(0, 0);
	}

	void DlightBufferBindings::SetDlightCounts(
		_In_ UINT add,
		_In_ UINT mod)
	{
		Counts.AddLightCount = add;
		Counts.ModLightCount = mod;
	}
}
