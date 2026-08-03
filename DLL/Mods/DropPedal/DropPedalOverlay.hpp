#pragma once

#include <string>

struct ID3DXFont;
struct Resolution;

namespace DropPedal
{
	class Overlay final
	{
	public:
		void Render(ID3DXFont* font, const Resolution& windowSize);

	private:
		bool hasCachedTuningState = false;
		bool cachedEnabled = false;
		int cachedTargetSemitones = 0;
		int cachedBaseTuningSemitones = 0;
		ID3DXFont* cachedTuningFont = nullptr;
		std::string tuningLine;
		unsigned int tuningTextColor = 0;

		bool hasCachedEngineState = false;
		bool cachedInputShifterActive = false;
		ID3DXFont* cachedEngineFont = nullptr;
		std::string engineLine;
		unsigned long long lastSeenNoticeTick = 0;
		unsigned long long engineDisplayStartTick = 0;

		void RenderTuning(ID3DXFont* font, const Resolution& windowSize);
		void RenderEngine(ID3DXFont* font, const Resolution& windowSize);
		void UpdateTuningCache(ID3DXFont* font);
		void UpdateEngineCache(ID3DXFont* font);
		void DrawShadowedText(
			ID3DXFont* font,
			const Resolution& windowSize,
			const std::string& text,
			unsigned int textColor,
			int topLeftX,
			int topLeftY,
			int bottomRightX,
			int bottomRightY) const;
	};
}
