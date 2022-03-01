#include "pch.h"
#include "GlobalCommandList.h"


namespace QD3D12
{
	ID3D12CommandQueue* GlobalCommandList::s_pCmdQ = nullptr;
	ID3D12CommandAllocator* GlobalCommandList::s_pCmdAlloc = nullptr;
	ID3D12GraphicsCommandList* GlobalCommandList::s_pCmdList = nullptr;

	void GlobalCommandList::Init(
		_In_ ID3D12Device* pDevice,
		_In_ ID3D12CommandQueue* pCmdQ )
	{
		assert( s_pCmdList == nullptr );

		// Get default command queue
        s_pCmdQ = SAFE_ADDREF( pCmdQ );
                
        // Create a command allocator
        COM_ERROR_TRAP( pDevice->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            __uuidof(ID3D12CommandAllocator),
            (void**) &s_pCmdAlloc ) );

        // Create a command list
        COM_ERROR_TRAP( pDevice->CreateCommandList(
            QD3D12_NODE_MASK,
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            s_pCmdAlloc,
            nullptr,
            __uuidof(ID3D12GraphicsCommandList),
            (void**) &s_pCmdList ) );
	}

	ID3D12GraphicsCommandList* GlobalCommandList::Get()
	{
		assert(s_pCmdList != nullptr);
		return s_pCmdList;
	}

	void GlobalCommandList::KickOff()
	{
		assert(s_pCmdQ != nullptr);
        COM_ERROR_TRAP( s_pCmdList->Close() );
		s_pCmdQ->ExecuteCommandLists( 1, (ID3D12CommandList**) &s_pCmdList );
        COM_ERROR_TRAP( s_pCmdList->Reset( s_pCmdAlloc, nullptr ) );
	}

	void GlobalCommandList::Destroy()
	{
		Device::WaitForGpu();

        SAFE_RELEASE(s_pCmdList);
        SAFE_RELEASE(s_pCmdAlloc);
		SAFE_RELEASE(s_pCmdQ);
	}
}
