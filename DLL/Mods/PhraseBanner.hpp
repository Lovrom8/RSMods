#pragma once

namespace PhraseBanner {
	void Install();

	/// <summary>
	/// Called from D3DHooks::Hook_SetVertexShader. Returns the shader that should actually be bound:
	/// our vertex texture version of a banner shader when the banner has too many phrases, otherwise the input.
	/// </summary>
	IDirect3DVertexShader9* OnSetVertexShader(IDirect3DDevice9* pDevice, IDirect3DVertexShader9* shader);

	/// <summary>
	/// Called after a successful device reset. Shader pointers can be reused after a reset, so forget what we classified.
	/// </summary>
	void OnDeviceReset();
}
