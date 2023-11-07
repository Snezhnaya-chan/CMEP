#pragma once

#include <unordered_map>
#include "PlatformSemantics.hpp"

namespace Engine
{
	class AssetManager;
	namespace Rendering
	{
		class Texture;
		struct FontChar
		{
			int x, y, width, height, xoffset, yoffset, xadvance, page, channel;
		};

		class CMEP_EXPORT Font final
		{
		private:
			AssetManager* asset_manager;
			std::string fntfile;
			unsigned int char_count = 0;

			// Data from fnt file
			std::unordered_map<std::string, std::string> info;
			std::unordered_map<int, std::shared_ptr<Texture>> pages;
			std::unordered_map<int, FontChar> chars;

			void EvalBmfont(FILE* file);
			void EvalBmfontLine(int type, char* data);
		public:
			Font(AssetManager* managed_by = nullptr);
			~Font();

			int Init(std::string path);

			FontChar* GetChar(char ch);
			Texture* GetPageTexture(int page);
			std::string* GetFontInfoParameter(std::string name);
		};
	}
}