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

int wmain(int argc, wchar_t **argv)
{
	using std::cout;
	using std::wcout;
	using std::cerr;
	using std::endl;
	std::ofstream output;
	std::wregex versionFormat(L"-(M)?([[:digit:]]{1,2})-([LMQH])");
	std::wcmatch results;
	std::unordered_map<wchar_t, QR::ErrorCorrectionLevel> levels = {
		{ L'L', QR::ErrorCorrectionLevel::L },
		{ L'M', QR::ErrorCorrectionLevel::M },
		{ L'Q', QR::ErrorCorrectionLevel::Q },
		{ L'H', QR::ErrorCorrectionLevel::H }
	};
	std::unordered_map<std::wstring_view, QR::Mode> modes = {
		{ L"-numeric", QR::Mode::NUMERIC },
		{ L"-alpha", QR::Mode::ALPHANUMERIC },
		{ L"-byte", QR::Mode::BYTE }
	};
	
	std::wstring message;
	std::optional<QR::SymbolType> type;
	std::optional<std::uint8_t> version;
	std::optional<QR::ErrorCorrectionLevel> level;
	std::vector<QR::ModeRange> modeRanges;

	for (int i = 1; i < argc - 1; ++i)
	{
		decltype(modes)::iterator modeIt;
		std::wstring_view argument(argv[i]);

		if ((modeIt = modes.find(argument)) != modes.end())
		{
			auto oldSize = message.size();

			message += argv[i + 1];
			modeRanges.emplace_back(modeIt->second, oldSize, message.size());
			++i;
		}
		else if (argument == L"-output")
		{
			output.open(argv[i + 1], std::ios_base::binary);
			++i;
		}
		else if (argument == L"-M1")
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
		wcout << "Usage: " << argv[0] << " -numeric|alpha|byte message -[M]V-E -output filename\n"
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
		output << QRToBMP(QR::Encode<wchar_t>(L"\x629", type.value(), version.value(), level.value(), modeRanges), 4);
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