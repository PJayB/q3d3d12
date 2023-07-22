#include "CommandState.h"
#include "Device.h"
#include "RootSignature.h"

namespace QD3D12
{
	CommandState::CommandState()
		: m_list(nullptr)
		, m_alloc(nullptr)
        , m_isOpen(false)
	{
	}

	CommandState::~CommandState()
	{
		Destroy();
	}

	void CommandState::Init()
	{
		ID3D12Device* pDevice = Device::Get();

		// Create default command allocator
		COM_ERROR_TRAP(pDevice->CreateCommandAllocator(
			D3D12_COMMAND_LIST_TYPE_DIRECT,
            __uuidof(ID3D12CommandAllocator),
			(void**) &m_alloc));

		// Create default command list
		COM_ERROR_TRAP(pDevice->CreateCommandList(
            QD3D12_NODE_MASK,
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			m_alloc,
			nullptr,
            __uuidof(ID3D12GraphicsCommandList),
			(void**) &m_list));

		m_list->Close();

        m_isOpen = false;
	}

	void CommandState::Destroy()
	{		
		SAFE_RELEASE(m_list);
		SAFE_RELEASE(m_alloc);
        m_isOpen = false;
	}

	void CommandState::Reset(ID3D12PipelineState* pPSO)
	{
		m_alloc->Reset();
		m_list->Reset(m_alloc, pPSO);
        m_isOpen = true;
	}

	void CommandState::CloseAndExecute()
	{
        ASSERT(m_isOpen);
        m_isOpen = false;

		COM_ERROR_TRAP(m_list->Close());
        ID3D12CommandList* pCmdLists[] = 
        { 
            m_list 
        };
        Device::GetQ()->ExecuteCommandLists(_countof(pCmdLists), pCmdLists);
	}
}
