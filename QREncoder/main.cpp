#include <fstream>
#include <iostream>
#include <unordered_map>
#include <regex>
#include <optional>
#include "Image.h"
#include "QRCode.h"

BMPImage QRToBMP(const std::vector<std::vector<bool>> &code, unsigned multiplier)
{
	BMPImage result(static_cast<std::uint16_t>(code.size() * multiplier), static_cast<std::uint16_t>(code.size() * multiplier), 1);

	for (std::vector<std::vector<bool>>::size_type i = 0; i < code.size(); ++i)
		for (std::vector<std::vector<bool>>::size_type j = 0; j < code[i].size(); ++j)
				for (decltype(i) y = 0; y < multiplier; ++y)
					for (decltype(j) x = 0; x < multiplier; ++x)
					{
						Point point;

						point.mX = static_cast<std::uint16_t>(j * multiplier + x);
						point.mY = static_cast<std::uint16_t>(i * multiplier + y);

						if (code[i][j])
							result.setPixelColor(point, { 0, 0, 0 });
						else
							result.setPixelColor(point, { 255, 255, 255 });
					}

	return result;
}

int main(int argc, char **argv)
{
	using std::cout;
	using std::wcout;
	using std::cerr;
	using std::endl;
	std::ofstream output;
	std::regex versionFormat("-(M)?([[:digit:]]{1,2})-([LMQH])");
	std::cmatch results;
	std::unordered_map<char, QR::ErrorCorrectionLevel> levels = {
		{ 'L', QR::ErrorCorrectionLevel::L },
		{ 'M', QR::ErrorCorrectionLevel::M },
		{ 'Q', QR::ErrorCorrectionLevel::Q },
		{ 'H', QR::ErrorCorrectionLevel::H }
	};
	std::unordered_map<std::string_view, QR::Mode> modes = {
		{ "-numeric", QR::Mode::NUMERIC },
		{ "-alpha", QR::Mode::ALPHANUMERIC },
		{ "-byte", QR::Mode::BYTE }
	};
	
	std::string message;
	std::optional<QR::SymbolType> type;
	std::optional<std::uint8_t> version;
	std::optional<QR::ErrorCorrectionLevel> level;
	std::vector<QR::ModeRange> modeRanges;

	for (int i = 1; i < argc - 1; ++i)
	{
		decltype(modes)::iterator modeIt;
		std::string_view argument(argv[i]);

		if ((modeIt = modes.find(argument)) != modes.end())
		{
			auto oldSize = message.size();

			message += argv[i + 1];
			modeRanges.emplace_back(modeIt->second, oldSize, message.size());
			++i;
		}
		else if (argument == "-output")
		{
			output.open(argv[i + 1], std::ios_base::binary);
			++i;
		}
		else if (argument == "-M1")
		{
			type = QR::SymbolType::MICRO_QR;
			version = 1;
			level = QR::ErrorCorrectionLevel::ERROR_DETECTION_ONLY;
		}
		else if (std::regex_match(argument.data(), results, versionFormat))
		{
			type = results[1].matched ? QR::SymbolType::MICRO_QR : QR::SymbolType::QR;
			version = static_cast<std::uint8_t>(std::stoul(results[2]));
			level = levels.at(results[3].str().front());
		}
	}

	if (argc == 1)
	{
		cout << "Usage: " << argv[0] << " -numeric|alpha|byte message -[M]V-E -output filename\n"
			<< "M: Indicates that the output will be a Micro QR symbol\n"
			<< "V: Indicates version number. Max is 40 for QR symbols and 4 for Micro QR symbols\n"
			<< "E: Error correction levels. Valid values are L, M, Q, H" << endl;

		return 0;
	}

	if (!output.is_open())
	{
		cerr << "Could not open output file" << endl;
		return -1;
	}

	if (message.empty())
	{
		cerr << "You must supply a message" << endl;
		return -1;
	}

	if (!type.has_value())
	{
		cerr << "You must specify symbol type, version and error correction level" << endl;
		return -1;
	}

	try
	{
		output << QRToBMP(QR::Encode<char>(message, type.value(), version.value(), level.value(), modeRanges), 4);
		/*QR::QREncoder code(QR::SymbolType::QR, 1, QR::ErrorCorrectionLevel::L);
		code.addCharacters<char>("1234", QR::Mode::NUMERIC);
		code.addCharacters<char>("45", QR::Mode::NUMERIC);
		code.addCharacters<char>("\xAF\x88", QR::Mode::KANJI);
		output << QRToBMP(code.generateMatrix(), 4);*/
	}
	catch (const std::length_error &e)
	{
		cerr << e.what() << endl;
		return -1;
	}
	catch (const std::invalid_argument &e)
	{
		cerr << e.what() << endl;
		return -1;
	}

	return 0;
}