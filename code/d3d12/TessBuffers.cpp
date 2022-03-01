#include "pch.h"
#include "TessBuffers.h"
#include "Frame.h"

namespace QD3D12
{
	static void CreateIBV(
		D3D12_INDEX_BUFFER_VIEW* pIBV,
		UploadScratch* pScratch,
		const void* pData,
		UINT count)
	{
		pIBV->BufferLocation = pScratch->Write(
			pData,
			QD3D12_INDEX_BUFFER_ALIGNMENT,
			TessBuffers::IndexSize(count));
		pIBV->Format = DXGI_FORMAT_R16_UINT;
		pIBV->SizeInBytes = TessBuffers::IndexSize(count);
	}

	static void CreateVBV(
		D3D12_VERTEX_BUFFER_VIEW* pIBV,
		UploadScratch* pScratch,
		const void* pData,
		UINT stride,
		UINT count)
	{
		pIBV->BufferLocation = pScratch->Write(
			pData,
			QD3D12_VERTEX_BUFFER_ALIGNMENT,
			count * stride);
		pIBV->StrideInBytes = stride;
		pIBV->SizeInBytes = stride * count;
	}

	// --- PRIVATE CLASSES ---
	void TessBuffers::ShaderTess::Init(UINT64 size)
	{
		for (UINT i = 0; i < _countof(m_texCoords); ++i)
		{
			m_texCoords[i].Init(TexCoordSize(size));
		}

		m_colors.Init(ColorSize(size));
	}

	void TessBuffers::ShaderTess::Destroy()
	{
		for (UINT i = 0; i < _countof(m_texCoords); ++i)
		{
			m_texCoords[i].Destroy();
		}

		m_colors.Destroy();
	}

	void TessBuffers::ShaderTess::SetResourceName(const wchar_t* name, int stage)
	{
		for (UINT i = 0; i < _countof(m_texCoords); ++i)
		{
			QD3D12::SetResourceName(m_texCoords[i].Resource(), L"%s (stage %d, texcoord %d)", name, stage, i);
		}

		QD3D12::SetResourceName(m_colors.Resource(), L"%s (color)", name);
	}

	void TessBuffers::ShaderTess::Reset()
	{
		for (UINT i = 0; i < _countof(m_texCoords); ++i)
		{
			m_texCoords[i].Reset();
		}

		m_colors.Reset();
	}

	void TessBuffers::FogTess::Init(UINT64 size)
	{
		m_texCoords.Init(TexCoordSize(size));
		m_colors.Init(ColorSize(size));
	}

	void TessBuffers::FogTess::Destroy()
	{
		m_texCoords.Destroy();
		m_colors.Destroy();
	}

	void TessBuffers::FogTess::Reset()
	{
		m_texCoords.Reset();
		m_colors.Reset();
	}

	void TessBuffers::FogTess::SetResourceName(const wchar_t* name)
	{
		QD3D12::SetResourceName(m_texCoords.Resource(), L"%s (texcoord)", name);
		QD3D12::SetResourceName(m_colors.Resource(), L"%s (color)", name);
	}

	// --- INSTANCE ---
	void TessBuffers::Init()
	{
		// Capped size of each of the buffers
		UINT64 size = BufferCapacity();

		for (UINT i = 0; i < _countof(m_stages); ++i)
		{
			m_stages[i].Init(size);
		}

		m_fog.Init(size);

		m_indices.Init(IndexSize(size));
		m_xyz.Init(XYZSize(size));
		m_normal.Init(NormalSize(size));
	}

	void TessBuffers::Destroy()
	{
		for (UINT i = 0; i < _countof(m_stages); ++i)
		{
			m_stages[i].Destroy();
		}

		m_fog.Destroy();
		m_indices.Destroy();
		m_xyz.Destroy();
		m_normal.Destroy();		
	}

	void TessBuffers::Reset()
	{
		for (UINT i = 0; i < _countof(m_stages); ++i)
		{
			m_stages[i].Reset();
		}

		m_fog.Reset();
		m_indices.Reset();
		m_xyz.Reset();
		m_normal.Reset();
	}

	void TessBuffers::SetResourceName(
		_In_ const char* fmt,
		_In_ ...)
	{
		va_list args;
		CHAR tmp[2048];

		va_start(args, fmt);
		vsprintf_s(tmp, _countof(tmp), fmt, args);
		va_end(args);

		WCHAR tmp2[2048];
		swprintf_s(tmp2, _countof(tmp2), L"%S", tmp);

		SetResourceNamesInternal(tmp2);
	}

	void TessBuffers::SetResourceName(
		_In_ const wchar_t* fmt,
		_In_ ...)
	{
		va_list args;
		WCHAR tmp[2048];

		va_start(args, fmt);
		vswprintf_s(tmp, _countof(tmp), fmt, args);
		va_end(args);

		SetResourceNamesInternal(tmp);
	}

	void TessBuffers::SetResourceNamesInternal(const wchar_t* name)
	{
#if QD3D12_PROFILE_NAMES
		for (UINT i = 0; i < _countof(m_stages); ++i)
		{
			m_stages[i].SetResourceName(name, i);
		}

		m_fog.SetResourceName(name);
		QD3D12::SetResourceName(m_indices.Resource(), name);
		QD3D12::SetResourceName(m_xyz.Resource(), name);
		QD3D12::SetResourceName(m_normal.Resource(), name);
#endif
	}
	
	// --- BINDINGS ---
	TessBufferBindings::TessBufferBindings()
	{
#ifdef _DEBUG
		ZeroMemory(this, sizeof(*this));
#endif
	}

	void TessBufferBindings::Upload(
		_In_ const shaderCommands_t* input,
		_In_ TessBuffers* tessBufs,
		_In_ BOOL needFog)
	{
		CreateIBV(
			&Indices,
			tessBufs->Indices(),
			input->indexes,
			input->numIndexes);

		CreateVBV(
			&XYZ,
			tessBufs->XYZ(),
			input->xyz,
			TessBuffers::XYZ_STRIDE,
			input->numVertexes);

		CreateVBV(
			&Normal,
			tessBufs->Normals(),
			input->normal,
			TessBuffers::NORMAL_STRIDE,
			input->numVertexes);

		for (UINT stage = 0; stage < TessBuffers::NUM_STAGES; ++stage)
		{
			const shaderStage_t* xstage = input->xstages[stage];
			const stageVars_t *cpuStage = &input->svars[stage];

			if (!xstage || !cpuStage) {
				break;
			}

			CreateVBV(
				&Stages[stage].Colors,
				tessBufs->StageColors(stage),
				cpuStage->colors,
				TessBuffers::COLOR_STRIDE,
				input->numVertexes);

			for (UINT bundle = 0; bundle < TessBuffers::NUM_TEXBUNDLES; ++bundle)
			{
				if (xstage->bundle[bundle].image[0] != 0)
				{
					CreateVBV(
						&Stages[stage].TexCoords[bundle],
						tessBufs->StageTexCoords(stage, bundle),
						cpuStage->texcoords[bundle],
						TessBuffers::TEXCOORD_STRIDE,
						input->numVertexes);
				}
			}
		}

		if (needFog)
		{
			CreateVBV(
				&Fog.Colors,
				tessBufs->FogColors(),
				input->fogVars.colors,
				TessBuffers::COLOR_STRIDE,
				input->numVertexes);

			CreateVBV(
				&Fog.TexCoords,
				tessBufs->FogTexCoords(),
				input->fogVars.texcoords,
				TessBuffers::TEXCOORD_STRIDE,
				input->numVertexes);
		}
	}

	// --- PRIVATE CLASSES ---

	void DlightTessBuffers::DlightTess::Init(UINT64 size)
	{
		m_indices.Init(TessBuffers::IndexSize(size));
		m_texCoords.Init(TessBuffers::TexCoordSize(size));
		m_colors.Init(TessBuffers::ColorSize(size));
	}

	void DlightTessBuffers::DlightTess::Destroy()
	{
		m_indices.Destroy();
		m_texCoords.Destroy();
		m_colors.Destroy();
	}

	void DlightTessBuffers::DlightTess::Reset()
	{
		m_indices.Reset();
		m_texCoords.Reset();
		m_colors.Reset();
	}

	// --- INSTANCE ---
	void DlightTessBuffers::Init()
	{
		// Capped size of each of the buffers
		UINT64 size = TessBuffers::BufferCapacity();

		for (UINT i = 0; i < _countof(m_dlights); ++i)
		{
			m_dlights[i].Init(size);
		}
	}

	void DlightTessBuffers::Destroy()
	{
		for (UINT i = 0; i < _countof(m_dlights); ++i)
		{
			m_dlights[i].Destroy();
		}
	}

	void DlightTessBuffers::Reset()
	{
		for (UINT i = 0; i < _countof(m_dlights); ++i)
		{
			m_dlights[i].Reset();
		}
	}

	// --- BINDINGS ---
	DlightTessBufferBindings::DlightTessBufferBindings()
	{
#ifdef _DEBUG
		ZeroMemory(this, sizeof(*this));
#endif
	}

	void DlightTessBufferBindings::Upload(
		_In_ const shaderCommands_t* input,
		_In_ DlightTessBuffers* tessBufs)
	{
		for (int l = 0; l < input->dlightCount; ++l)
		{
			const dlightProjectionInfo_t* cpuLight = &input->dlightInfo[l];

			if (!cpuLight->numIndexes) {
				continue;
			}

			TESS_DLIGHT_VIEWS* pView = &Dlights[l];

			CreateIBV(
				&pView->Indices,
				tessBufs->Indices(l),
				cpuLight->hitIndexes,
				cpuLight->numIndexes);

			CreateVBV(
				&pView->Colors,
				tessBufs->Colors(l),
				cpuLight->colorArray,
				TessBuffers::COLOR_STRIDE,
				input->numVertexes);

			CreateVBV(
				&pView->TexCoords,
				tessBufs->TexCoords(l),
				cpuLight->texCoordsArray,
				TessBuffers::TEXCOORD_STRIDE,
				input->numVertexes);
		}
	}
}
