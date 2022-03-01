#pragma once
#include "UploadScratch.h"

namespace QD3D12 
{
	class TessBuffers
	{
	public: 
		static const UINT NUM_DLIGHTS = MAX_DLIGHTS;
		static const UINT NUM_STAGES = MAX_SHADER_STAGES;
		static const UINT NUM_TEXBUNDLES = NUM_TEXTURE_BUNDLES;

	private:
		class ShaderTess
		{
		private:
			UploadScratch m_texCoords[NUM_TEXBUNDLES];
			UploadScratch m_colors;

		public:
			void Init(UINT64 size);
			void Destroy();
			void Reset();
			void SetResourceName(const wchar_t* name, int stage);

			inline UploadScratch* TexCoords(UINT i) { return &m_texCoords[i]; }
			inline UploadScratch* Colors() { return &m_colors; }
		};

		struct FogTess
		{
		private:
			UploadScratch m_texCoords;
			UploadScratch m_colors;

		public:
			void Init(UINT64 size);
			void Destroy();
			void Reset();
			void SetResourceName(const wchar_t* name);

			inline UploadScratch* TexCoords() { return &m_texCoords; }
			inline UploadScratch* Colors() { return &m_colors; }
		};

		ShaderTess m_stages[NUM_STAGES];
		FogTess m_fog;
		UploadScratch m_indices;
		UploadScratch m_xyz;
		UploadScratch m_normal;

		void SetResourceNamesInternal(const wchar_t* name);

	public:		
		void Init();
		void Destroy();
		void Reset();

		void SetResourceName(const char* fmt, ...);
		void SetResourceName(const wchar_t* fmt, ...);

		inline UploadScratch* StageTexCoords(UINT stage, UINT bundle) { return m_stages[stage].TexCoords(bundle); }
		inline UploadScratch* StageColors(UINT stage) { return m_stages[stage].Colors(); }
		inline UploadScratch* FogTexCoords() { return m_fog.TexCoords(); }
		inline UploadScratch* FogColors() { return m_fog.Colors(); }
		inline UploadScratch* Indices() { return &m_indices; }
		inline UploadScratch* XYZ() { return &m_xyz; }
		inline UploadScratch* Normals() { return &m_normal; }

	public:
		static inline UINT64 BufferCapacity() { return SHADER_MAX_INDEXES * QD3D12_ESTIMATED_DRAW_CALLS; }

		static const UINT64 INDEX_STRIDE = sizeof(glIndex_t);
		static const UINT64 XYZ_STRIDE = sizeof(vec4_t);
		static const UINT64 NORMAL_STRIDE = sizeof(vec4_t);
		static const UINT64 COLOR_STRIDE = sizeof(color4ub_t);
		static const UINT64 TEXCOORD_STRIDE = sizeof(vec2_t);

		static inline UINT64 IndexSize(UINT64 count) { return INDEX_STRIDE * count; }
		static inline UINT64 XYZSize(UINT64 count) { return XYZ_STRIDE * count; }
		static inline UINT64 NormalSize(UINT64 count) { return NORMAL_STRIDE * count; }
		static inline UINT64 ColorSize(UINT64 count) { return COLOR_STRIDE * count; }
		static inline UINT64 TexCoordSize(UINT64 count) { return TEXCOORD_STRIDE * count; }
	};

	class TessBufferBindings
	{
	public:
		struct TESS_DLIGHT_VIEWS
		{
			D3D12_INDEX_BUFFER_VIEW Indices;
			D3D12_VERTEX_BUFFER_VIEW TexCoords;
			D3D12_VERTEX_BUFFER_VIEW Colors;
		};

		struct TESS_SHADER_STAGE_VIEWS
		{
			D3D12_VERTEX_BUFFER_VIEW TexCoords[TessBuffers::NUM_TEXBUNDLES];
			D3D12_VERTEX_BUFFER_VIEW Colors;
		};

		struct TESS_FOG_VIEWS
		{
			D3D12_VERTEX_BUFFER_VIEW TexCoords;
			D3D12_VERTEX_BUFFER_VIEW Colors;
		};

		TessBufferBindings();

		void Upload(
			_In_ const shaderCommands_t* input, 
			_In_ TessBuffers* tessBufs,
			_In_ BOOL needFog);

		TESS_SHADER_STAGE_VIEWS Stages[TessBuffers::NUM_STAGES];
		TESS_FOG_VIEWS Fog;
		D3D12_INDEX_BUFFER_VIEW Indices;

		union {
			D3D12_VERTEX_BUFFER_VIEW BaseVertexBuffers[2];
			struct {
				D3D12_VERTEX_BUFFER_VIEW XYZ;
				D3D12_VERTEX_BUFFER_VIEW Normal;
			};
		};

		static const SIZE_T VERTEX_BUFFER_COUNT = 2;
	};

	class DlightTessBuffers
	{
	public:
		static const UINT NUM_DLIGHTS = MAX_DLIGHTS;

	private:
		struct DlightTess
		{
		private:
			UploadScratch m_indices;
			UploadScratch m_texCoords;
			UploadScratch m_colors;

		public:
			void Init(UINT64 size);
			void Destroy();
			void Reset();

			inline UploadScratch* Indices() { return &m_indices; }
			inline UploadScratch* TexCoords() { return &m_texCoords; }
			inline UploadScratch* Colors() { return &m_colors; }
		};

		DlightTess m_dlights[NUM_DLIGHTS];

	public:
		void Init();
		void Destroy();
		void Reset();

		inline UploadScratch* Indices(UINT dlight) { return m_dlights[dlight].Indices(); }
		inline UploadScratch* TexCoords(UINT dlight) { return m_dlights[dlight].TexCoords(); }
		inline UploadScratch* Colors(UINT dlight) { return m_dlights[dlight].Colors(); }
	};

	class DlightTessBufferBindings
	{
	public:
		struct TESS_DLIGHT_VIEWS
		{
			D3D12_INDEX_BUFFER_VIEW Indices;
			D3D12_VERTEX_BUFFER_VIEW TexCoords;
			D3D12_VERTEX_BUFFER_VIEW Colors;
		};

		DlightTessBufferBindings();

		void Upload(
			_In_ const shaderCommands_t* input,
			_In_ DlightTessBuffers* tessBufs);

		TESS_DLIGHT_VIEWS Dlights[DlightTessBuffers::NUM_DLIGHTS];
	};
}
