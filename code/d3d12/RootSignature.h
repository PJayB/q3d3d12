#pragma once

namespace QD3D12
{
	class RootSignature
	{
	private:
		static ID3D12RootSignature* s_pEmpty;

	public:
		static ID3D12RootSignature* Empty();

		static ID3D12RootSignature* Create(
			_In_ const D3D12_ROOT_SIGNATURE_DESC* rsig);

		static void Init();
		static void Destroy();
	};
}
