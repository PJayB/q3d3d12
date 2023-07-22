#pragma once

#include "D3D12Core.h"

namespace QD3D12
{
	class CommandState
	{
	private:

        ID3D12GraphicsCommandList* m_list;
		ID3D12CommandAllocator* m_alloc;
        bool m_isOpen;

	public:

		CommandState();
		~CommandState();

		void Init();
		void Destroy();

        ID3D12GraphicsCommandList* List() const { ASSERT(m_isOpen); return m_list; }
		ID3D12CommandAllocator* Allocator() const { return m_alloc; }

		// Call this at the start of the frame
		void Reset(ID3D12PipelineState* pPSO = nullptr);

		// Call this at the end of the frame
		void CloseAndExecute();
	};

}
