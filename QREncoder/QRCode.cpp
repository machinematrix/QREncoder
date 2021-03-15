#include "QRCode.h"
#include <stdexcept>
#include <array>
#include <optional>
#include <tuple>

namespace QR
{
	using Symbol = std::vector<std::vector<bool>>;
	
	namespace
	{
		unsigned GetSymbolSize(SymbolType type, std::uint8_t version)
		{
			unsigned result = 0;

			switch (type)
			{
				case SymbolType::QR:
					if (version > 40)
						throw std::invalid_argument("Invalid version");
					result = 21u + (version - 1) * 4;
					break;

				case SymbolType::MICRO_QR:
					if (version > 4)
						throw std::invalid_argument("Invalid version");
					result = 11u + (version - 1) * 2;
					break;
			}

			return result;
		}

		//Divide by 8 to get data capacity in codewords, do % 8 to get remainder bits. Exceptions are M1 and M3, where the last codeword is 4 bits long
		unsigned GetDataModuleCount(SymbolType type, std::uint8_t version)
		{
			static const std::array<unsigned, 4> microModuleCount = { 36, 80, 132, 192 };
			static const std::array<unsigned, 40> moduleCount = {
				208, 359, 567, 807, 1079, 1383, 1568, 1936,
				2336, 2768, 3232, 3728, 4256, 4651, 5243, 5867,
				6523, 7211, 7931, 8683, 9252, 10068, 10916, 11796,
				12708, 13652, 14628, 15371, 16411, 17483, 18587, 19723,
				20891, 22091, 23008, 24272, 25568, 26896, 28256, 29648
			};
			unsigned result;

			if (type == SymbolType::MICRO_QR)
				result = microModuleCount[version - 1];
			else
				result = moduleCount[version - 1];

			return result;
		}

		//From table 9, page 38
		unsigned GetErrorCorrectionCodewordCount(SymbolType type, Version version)
		{
			static const std::array<std::array<unsigned, 4>, 4> microCodewordCount = { {
				{ 2 }, //This will be used for M1 symbols, ErrorCorrectionLevel will be NONE
				{ 5, 6 },
				{ 6, 8 },
				{ 8, 10, 14 },
			} };
			static const std::array<std::array<unsigned, 4>, 40> codewordCount = { {
				{ 7, 10, 13, 17 },
				{ 10, 16, 22, 28 },
				{ 15, 26, 36, 44 },
				{ 20, 36, 52, 64 },
				{ 26, 48, 72, 88 },
				{ 36, 64, 96, 112 },
				{ 40, 72, 108, 130 },
				{ 48, 88, 132, 156 },
				{ 60, 110, 160, 192 },
				{ 72, 130, 192, 224 },
				{ 80, 150, 224, 264 },
				{ 96, 176, 260, 308 },
				{ 104, 198, 288, 352 },
				{ 120, 216, 320, 384 },
				{ 132, 240, 360, 432 },
				{ 144, 280, 408, 480 },
				{ 168, 308, 448, 532 },
				{ 180, 338, 504, 588 },
				{ 196, 364, 546, 650 },
				{ 224, 416, 600, 700 },
				{ 224, 442, 644, 750 },
				{ 252, 476, 690, 816 },
				{ 270, 504, 750, 900 },
				{ 300, 560, 810, 960 },
				{ 312, 588, 870, 1050 },
				{ 336, 644, 952, 1110 },
				{ 360, 700, 1020, 1200 },
				{ 390, 728, 1050, 1260 },
				{ 420, 784, 1140, 1350 },
				{ 450, 812, 1200, 1440 },
				{ 480, 868, 1290, 1530 },
				{ 510, 924, 1350, 1620 },
				{ 540, 980, 1440, 1710 },
				{ 570, 1036, 1530, 1800 },
				{ 570, 1064, 1590, 1890 },
				{ 600, 1120, 1680, 1980 },
				{ 630, 1204, 1770, 2100 },
				{ 660, 1260, 1860, 2220 },
				{ 720, 1316, 1950, 2310 },
				{ 750, 1372, 2040, 2430 }
			} };
			unsigned result;

			if (type == SymbolType::MICRO_QR)
			{
				if (version.mVersion == 1)
					result = microCodewordCount[0][0];
				else
					result = microCodewordCount[version.mVersion - 1][static_cast<decltype(microCodewordCount)::size_type>(version.mLevel)];
			}
			else
				result = codewordCount[version.mVersion - 1][static_cast<decltype(codewordCount)::size_type>(version.mLevel)];

			return result;
		}

		//From table 2, page 23. version parameter is only used for Micro QR
		std::vector<bool> GetModeIndicator(SymbolType type, std::uint8_t version, Mode mode)
		{
			std::vector<bool> result;

			if (type == SymbolType::MICRO_QR)
			{
				result.resize(version - 1);

				if (version > 1)
				{
					if (mode == Mode::ALPHANUMERIC || mode == Mode::KANJI)
						result.back() = 1;

					if (version > 2 && (mode == Mode::BYTE || mode == Mode::KANJI))
						result.at(result.size() - 2) = 1;
				}
			}
			else
			{
				switch (mode)
				{
					case Mode::NUMERIC:
						result = { 0, 0, 0, 1 };
						break;

					case Mode::ALPHANUMERIC:
						result = { 0, 0, 1, 0 };
						break;

					case Mode::BYTE:
						result = { 0, 1, 0, 0 };
						break;

					case Mode::KANJI:
						result = { 1, 0, 0, 0 };
						break;
				}
			}

			return result;
		}

		//version parameter is only used for Micro QR
		std::vector<bool> GetTerminator(SymbolType type, std::uint8_t version)
		{
			return std::vector<bool>(type == SymbolType::MICRO_QR ? 3 + (version - 1) * 2 : 4, false);
		}

		unsigned GetCharacterCountIndicatorLength(Mode mode, SymbolType type, std::uint8_t version)
		{
			using TableType = std::array<std::array<unsigned, 4>, 4>;
			static const TableType microModeLengths = { { { 3 }, { 4, 3 }, { 5, 4, 4, 3 }, { 6, 5, 5, 4 } } };
			static const TableType modeLengths = { { { 10, 9, 8, 8 }, { 12, 11, 16, 10 }, { 14, 13, 16, 12 } } };
			unsigned result = 0;
			
			if (type == SymbolType::MICRO_QR)
				result = microModeLengths[version][static_cast<size_t>(mode)];
			else
			{
				if (version <= 9)
					result = modeLengths[0][static_cast<size_t>(mode)];
				else
					if (version <= 26)
						result = modeLengths[1][static_cast<size_t>(mode)];
					else
						result = modeLengths[2][static_cast<size_t>(mode)];
			}

			return result;
		}

		//From table 3, page 23
		std::vector<bool> GetCharacterCountIndicator(SymbolType type, std::uint8_t version, Mode mode, std::uint16_t characterCount)
		{
			using TableType = std::array<std::array<unsigned, 4>, 4>;
			static const TableType microModeLengths = { { { 3 }, { 4, 3 }, { 5, 4, 4, 3 }, { 6, 5, 5, 4 } } };
			static const TableType modeLengths = { { { 10, 9, 8, 8 }, { 12, 11, 16, 10 }, { 14, 13, 16, 12 } } };
			unsigned length = 0;
			std::vector<bool> result;

			if (type == SymbolType::MICRO_QR)
				length = microModeLengths[version - 1][static_cast<size_t>(mode)];
			else
			{
				if (version <= 9)
					length = modeLengths[0][static_cast<size_t>(mode)];
				else
					if (version <= 26)
						length = modeLengths[1][static_cast<size_t>(mode)];
					else
						length = modeLengths[2][static_cast<size_t>(mode)];
			}

			result.resize(length);

			for (decltype(result)::size_type i = result.size(); i--;)
				result.at(i) = characterCount & 1 << (length - 1) >> i ? true : false;

			return result;
		}

		const std::vector<Symbol::size_type>& GetAlignmentPatternCenters(std::uint8_t version)
		{
			static const std::array<std::vector<Symbol::size_type>, 40> centers = { {
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
			} };

			return centers.at(version - 1);
		}

		std::vector<std::vector<bool>> GetDataRegionMask(SymbolType type, std::uint8_t version)
		{
			using std::vector;
			using std::pair;
			using SizeType = Symbol::size_type;
			struct Rectangle
			{
				struct Point
				{
					Symbol::size_type mX;
					Symbol::size_type mY;
				};
				Point mFrom;
				Point mTo;
			};
			auto symbolSize = GetSymbolSize(type, version);
			vector<vector<bool>> result(symbolSize, vector<bool>(symbolSize));
			auto &centers = GetAlignmentPatternCenters(version);
			vector<Rectangle> functionPatterns;
			SizeType timingRowColumn = type == SymbolType::MICRO_QR ? 0 : 6;

			//Top left finder pattern
			functionPatterns.push_back({ { 0, 0 }, { 7, 7 } });

			//Timing patterns
			functionPatterns.push_back({ { timingRowColumn, 0 }, { timingRowColumn, symbolSize - 1 } });
			functionPatterns.push_back({ { 0, timingRowColumn }, { symbolSize - 1, timingRowColumn } });

			//Top left format information
			functionPatterns.push_back({ { 8, 0 }, { 8, 8 } });
			functionPatterns.push_back({ { 0, 8 }, { 8, 8 } });

			if (type != SymbolType::MICRO_QR)
			{
				//Top right and bottom left finder patterns
				functionPatterns.push_back({ { 0, symbolSize - 8 }, { 7, symbolSize - 1 } });
				functionPatterns.push_back({ { symbolSize - 8, 0 }, { symbolSize - 1, 7 } });

				//Bottom left and top right format infomation
				functionPatterns.push_back({ { 8, symbolSize - 8 }, { 8, symbolSize - 1 } });
				functionPatterns.push_back({ { symbolSize - 8, 8 }, { symbolSize - 1, 8 } });

				//Alignment patterns
				for (unsigned i = 0; i < centers.size(); ++i)
					for (unsigned j = i; j < centers.size(); ++j)
					{
						auto centerX = centers[j], centerY = centers[i];

						if (!(centerY == 6 && centerX == symbolSize - 7 || centerY == symbolSize - 7 && centerX == 6 || centerY == 6 && centerX == 6))
						{
							functionPatterns.push_back({ { centerX - 2, centerY - 2 }, { centerX + 2, centerY + 2 } });
							functionPatterns.push_back({ { centerY - 2, centerX - 2 }, { centerY + 2, centerX + 2 } });
						}
					}

				if (version >= 7)
				{
					functionPatterns.push_back({ { 0, symbolSize - 11 }, { 5, symbolSize - 9 } });
					functionPatterns.push_back({ { symbolSize - 11, 0 }, { symbolSize - 9, 5 } });
				}
			}

			for (const auto &rectangle : functionPatterns)
				for (unsigned x = rectangle.mFrom.mX; x <= rectangle.mTo.mX; ++x)
					for (unsigned y = rectangle.mFrom.mY; y <= rectangle.mTo.mY; ++y)
						result[y][x] = true;

			return result;
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
			unsigned rowColumn = type == SymbolType::MICRO_QR ? 0 : 6;

			for (unsigned i = 8; i < (type == SymbolType::MICRO_QR ? size : size - 8); ++i)
				symbol[i][rowColumn] = symbol[rowColumn][i] = !(i % 2);
		}

		void DrawAlignmentPatterns(Symbol &symbol, std::uint8_t version)
		{
			const std::vector<Symbol::size_type> &centers = GetAlignmentPatternCenters(version);
			auto symbolSize = GetSymbolSize(QR::SymbolType::QR, version);

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

	std::vector<std::vector<bool>> QR::Encode(std::string_view message, SymbolType type, Version version, Mode mode)
	{
		std::vector<std::vector<bool>> result(GetSymbolSize(type, version.mVersion), std::vector<bool>(GetSymbolSize(type, version.mVersion)));
		std::vector<bool> dataBitStream, terminator = GetTerminator(type, version.mVersion);
		std::vector<std::tuple<Mode, decltype(message)::size_type, decltype(message)::size_type>> modeRanges = { { mode, 0, message.size() } }; //<type, [from, to)>
		unsigned quietZoneWidth = type == SymbolType::MICRO_QR ? 2 : 4, dataModuleCount = GetDataModuleCount(type, version.mVersion) - GetErrorCorrectionCodewordCount(type, version) * 8;
		std::array<std::vector<bool>, 2> padCodewords = { { { 1, 1, 1, 0, 1, 1, 0, 0 }, { 0, 0, 0, 1, 0, 0, 0, 1 } } }; //Pad codeword in the last 4 bit codeword in M1 and M3 shall be 0b0000

		DrawFinderPatterns(result, 0, 0);
		if (type != SymbolType::MICRO_QR)
		{
			DrawFinderPatterns(result, 0, GetSymbolSize(type, version.mVersion) - 7);
			DrawFinderPatterns(result, GetSymbolSize(type, version.mVersion) - 7, 0);
			DrawAlignmentPatterns(result, version.mVersion);
		}
		DrawTimingPatterns(result, type, version.mVersion);
		
		for (const auto &range : modeRanges)
		{
			switch (std::get<0>(range))
			{
				case Mode::BYTE:
				{
					auto modeIndicator = GetModeIndicator(type, version.mVersion, mode), terminator = GetTerminator(type, version.mVersion);
					auto countIndicator = GetCharacterCountIndicator(type, version.mVersion, mode, static_cast<std::uint16_t>(message.size()));
					dataBitStream.insert(dataBitStream.end(), modeIndicator.begin(), modeIndicator.end());
					dataBitStream.insert(dataBitStream.end(), countIndicator.begin(), countIndicator.end());
					auto codeword = dataBitStream.size();
					dataBitStream.resize(dataBitStream.size() + (std::get<2>(range) - std::get<1>(range)) * 8);

					for (auto i = std::get<1>(range); i < std::get<2>(range); ++i, codeword += 8)
						for (decltype(result)::size_type bit = 8; bit--;)
							dataBitStream.at(codeword + bit) = message[i] & 1 << 7 >> bit ? true : false;

					break;
				}
			}
		}

		if (dataBitStream.size() <= dataModuleCount)
		{
			if (dataModuleCount - dataBitStream.size() >= terminator.size())
				dataBitStream.insert(dataBitStream.end(), terminator.begin(), terminator.end());


		}

		//Add quiet zone
		for (auto &row : result)
		{
			row.insert(row.begin(), quietZoneWidth, false);
			row.insert(row.end(), quietZoneWidth, false);
		}

		for (unsigned i = 0; i < quietZoneWidth; ++i)
		{
			result.insert(result.begin(), std::vector<bool>(GetSymbolSize(type, version.mVersion) + quietZoneWidth * 2));
			result.insert(result.end(), std::vector<bool>(GetSymbolSize(type, version.mVersion) + quietZoneWidth * 2));
		}

		return result;
	}
}

QR::Version operator""_L(std::uint64_t version)
{
	return QR::Version{ static_cast<std::uint8_t>(version), QR::ErrorCorrectionLevel::L };
}

QR::Version operator""_M(std::uint64_t version)
{
	return QR::Version{ static_cast<std::uint8_t>(version), QR::ErrorCorrectionLevel::M };
}

QR::Version operator""_Q(std::uint64_t version)
{
	return QR::Version{ static_cast<std::uint8_t>(version), QR::ErrorCorrectionLevel::Q };
}

QR::Version operator""_H(std::uint64_t version)
{
	return QR::Version{ static_cast<std::uint8_t>(version), QR::ErrorCorrectionLevel::H };
}