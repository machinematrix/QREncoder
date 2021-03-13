#include "QRCode.h"
#include <stdexcept>
#include <array>

namespace QR
{
	using Symbol = std::vector<std::vector<bool>>;

	namespace
	{
		constexpr unsigned GetSymbolSize(SymbolType type, std::uint8_t version)
		{
			unsigned result = 0;

			switch (type)
			{
				case SymbolType::NORMAL:
					if (version > 40)
						throw std::invalid_argument("Invalid version");
					result = 21u + (version - 1) * 4;
					break;

				case SymbolType::MICRO:
					if (version > 4)
						throw std::invalid_argument("Invalid version");
					result = 11u + (version - 1) * 2;
					break;
			}

			return result;
		}

		const std::vector<Symbol::size_type>& GetAlignmentPatternCenters(std::uint8_t version)
		{
			static const std::vector<std::vector<Symbol::size_type>> centers = {
				{},
				{ 6, 18 },
				{ 6, 22 },
				{ 6, 26 },
				{ 6, 30 },
				{ 6, 34 },
				{ 6, 22, 38 },
				{ 6, 24, 42 },
				{ 6, 26, 46 },
				{ 6, 28, 50 },
				{ 6, 30, 54 },
				{ 6, 32, 58 },
				{ 6, 34, 62 },
				{ 6, 26, 46, 66 },
				{ 6, 26, 48, 70 },
				{ 6, 26, 50, 74 },
				{ 6, 30, 54, 78 },
				{ 6, 30, 56, 82 },
				{ 6, 30, 58, 86 },
				{ 6, 34, 62, 90 },
				{ 6, 28, 50, 72, 94 },
				{ 6, 26, 50, 74, 98 },
				{ 6, 30, 54, 78, 102 },
				{ 6, 28, 54, 80, 106 },
				{ 6, 32, 58, 84, 110 },
				{ 6, 30, 58, 86, 114 },
				{ 6, 34, 62, 90, 118 },
				{ 6, 26, 50, 74, 98, 122 },
				{ 6, 30, 54, 78, 102, 126 },
				{ 6, 26, 52, 78, 104, 130 },
				{ 6, 30, 56, 82, 108, 134 },
				{ 6, 34, 60, 86, 112, 138 },
				{ 6, 30, 58, 86, 114, 142 },
				{ 6, 34, 62, 90, 118, 146 },
				{ 6, 30, 54, 78, 102, 126, 150 },
				{ 6, 24, 50, 76, 102, 128, 154 },
				{ 6, 28, 54, 80, 106, 132, 158 },
				{ 6, 32, 58, 84, 110, 136, 162 },
				{ 6, 26, 54, 82, 110, 138, 166 },
				{ 6, 30, 58, 86, 114, 142, 170 }
			};

			return centers.at(version - 1);
		}

		void DrawFinderPatterns(Symbol &symbol, Symbol::size_type startingRow, Symbol::size_type startingColumn)
		{
			for (decltype(startingRow) i = startingRow; i < startingRow + 7; ++i)
				for (decltype(startingColumn) j = startingColumn; j < startingColumn + 7; ++j)
				{
					switch (i - startingRow)
					{
						case 0:
						case 6:
							symbol[i][j] = true;
							break;

						case 1:
						case 5:
							if (j - startingColumn == 0 || j - startingColumn == 6)
								symbol[i][j] = true;
							break;

						case 2:
						case 3:
						case 4:
							if (j - startingColumn == 0 || j - startingColumn == 2 || j - startingColumn == 3 || j - startingColumn == 4 || j - startingColumn == 6)
								symbol[i][j] = true;
							break;
					}
				}
		}

		void DrawTimingPatterns(Symbol &symbol, SymbolType type, std::uint8_t version)
		{
			auto size = GetSymbolSize(type, version);
			unsigned rowColumn = type == SymbolType::MICRO ? 0 : 6;

			for (unsigned i = 8; i < (type == SymbolType::MICRO ? size : size - 8); ++i)
				symbol[i][rowColumn] = symbol[rowColumn][i] = !(i % 2);
		}

		void DrawAlignmentPatterns(Symbol &symbol, std::uint8_t version)
		{
			const std::vector<Symbol::size_type> &centers = GetAlignmentPatternCenters(version);
			auto symbolSize = GetSymbolSize(QR::SymbolType::NORMAL, version);

			for (unsigned i = 0; i < centers.size(); ++i)
				for (unsigned j = i; j < centers.size(); ++j)
				{
					auto centerX = centers[j], centerY = centers[i];

					if (!(centerY == 6 && centerX == symbolSize - 7 || centerY == symbolSize - 7 && centerX == 6 || centerY == 6 && centerX == 6))
						for (unsigned startX = centerX - 2, x = startX; x < centerX + 3; ++x)
							for (unsigned startY = centerY - 2, y = startY; y < centerY + 3; ++y)
							{
								switch (y - startY)
								{
									case 0:
									case 4:
										symbol[y][x] = symbol[x][y] = true;
										break;
									
									case 2:
										if (x - startX == 0 || x - startX == 2 || x - startX == 4)
										{
											symbol[y][x] = symbol[x][y] = true;
											break;
										}
										[[fallthrough]];

									case 1:
									case 3:
										if (x - startX == 0 || x - startX == 4)
											symbol[y][x] = symbol[x][y] = true;
										break;
								}
							}
				}
		}
	}

	std::vector<std::vector<bool>> QR::Encode(std::string_view message, Mode mode, SymbolType type, std::uint8_t version)
	{
		std::vector<std::vector<bool>> result(GetSymbolSize(type, version), std::vector<bool>(GetSymbolSize(type, version)));
		unsigned quietZoneWidth = type == SymbolType::MICRO ? 2 : 4;

		DrawFinderPatterns(result, 0, 0);
		if (type != SymbolType::MICRO)
		{
			DrawFinderPatterns(result, 0, GetSymbolSize(type, version) - 7);
			DrawFinderPatterns(result, GetSymbolSize(type, version) - 7, 0);
		}
		DrawTimingPatterns(result, type, version);
		DrawAlignmentPatterns(result, version);

		//Add quiet zone
		for (auto &row : result)
		{
			row.insert(row.begin(), quietZoneWidth, false);
			row.insert(row.end(), quietZoneWidth, false);
		}

		for (auto i = 0u; i < quietZoneWidth; ++i)
		{
			result.insert(result.begin(), std::vector<bool>(GetSymbolSize(type, version) + quietZoneWidth * 2));
			result.insert(result.end(), std::vector<bool>(GetSymbolSize(type, version) + quietZoneWidth * 2));
		}

		return result;
	}
}