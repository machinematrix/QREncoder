#include <fstream>
#include <iostream>
#include <unordered_map>
#include <regex>
#include <optional>
#include <Windows.h>
#include "Image.h"
#include "QREncoder.h"

int wmain(int argc, wchar_t **argv)
{
	using std::cout;
	using std::wcout;
	using std::cerr;
	using std::endl;
	using namespace std::string_view_literals;
	std::wregex versionFormat(L"-(M)?([[:digit:]]{1,2})-([LMQH])"), colorFormat(LR"(\{([[:digit:]]{1,3}),([[:digit:]]{1,3}),([[:digit:]]{1,3})\})");
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
	int result = 0;

	if (argc == 1)
	{
		wcout << "Usage: " << argv[0] << " -[M]V-E -numeric|alpha|byte message -light|dark {R,G,B} -output filename\n"
			<< "M: Indicates that the output will be a Micro QR symbol\n"
			<< "V: Indicates version number. Max is 40 for QR symbols and 4 for Micro QR symbols\n"
			<< "E: Error correction level. Valid values are L, M, Q, H\n"
			<< "light|dark: optional, set the color for light and/or dark modules\n"
			<< "Symbol version must be the first argument, the rest of the arguments may appear in any order\n"
			<< "Example: " << argv[0] << " -6-H -alpha \"Hello World\" -light {255,0,0} -output hello_world.bmp" << endl;
	}
	else if (std::regex_match(argv[1], results, versionFormat) || argv[1] == std::wstring_view(L"-M1"))
	{
		std::ofstream output;
		std::wstring filename;
		QR::SymbolType type;
		std::uint8_t version;
		QR::ErrorCorrectionLevel level;

		if (!results.empty())
		{
			type = results[1].matched ? QR::SymbolType::MICRO_QR : QR::SymbolType::QR;
			version = static_cast<std::uint8_t>(std::stoul(results[2]));
			level = levels.at(results[3].str().front());
		}
		else
		{
			type = QR::SymbolType::MICRO_QR;
			version = 1;
			level = QR::ErrorCorrectionLevel::ERROR_DETECTION_ONLY;
		}

		try
		{
			QR::Encoder encoder(type, version, level);
			QR::Color dark = {}, light = { 255, 255, 255 };
			
			for (int i = 2; i < argc - 1; ++i)
			{
				decltype(modes)::iterator modeIt;

				if ((modeIt = modes.find(argv[i])) != modes.end())
				{
					std::wstring_view wideString = argv[i + 1];
					std::string utf8Message;
					
					utf8Message.resize(WideCharToMultiByte(CP_UTF8, 0, wideString.data(), wideString.size(), nullptr, 0, nullptr, nullptr));
					WideCharToMultiByte(CP_UTF8, 0, wideString.data(), wideString.size(), utf8Message.data(), utf8Message.size(), nullptr, nullptr);
					encoder.addCharacters(utf8Message, modeIt->second);
					++i;
				}
				else if (argv[i] == L"-output"sv)
				{
					filename = argv[i + 1];
					++i;
				}
				else if (bool isLight; (isLight = argv[i] == L"-light"sv) || argv[i] == L"-dark"sv)
				{
					if (std::regex_match(argv[i + 1], results, colorFormat))
					{
						int red = std::stoi(results[1]), green = std::stoi(results[2]), blue = std::stoi(results[3]);

						if (red > 255 || green > 255 || blue > 255)
							throw std::invalid_argument("Invalid color intensity. Valid values are [0,255]");

						if (isLight)
							light.mRed = red, light.mGreen = green, light.mBlue = blue;
						else
							dark.mRed = red, dark.mGreen = green, dark.mBlue = blue;
					}
					else
						throw std::invalid_argument("Invalid color");
				}
			}

			auto qr = encoder.generateMatrix();

			output.open(filename, std::ios_base::binary);

			if (output.is_open())
				output << QR::QRToBMP(qr, 4, light, dark);
			else
			{
				cerr << "Could not open output file" << endl;
				result = -1;
			}
		}
		catch (const std::length_error &e)
		{
			cerr << e.what() << endl;
			result = -1;
		}
		catch (const std::invalid_argument &e)
		{
			cerr << e.what() << endl;
			result = -1;
		}
	}
	else
	{
		cerr << "Invalid version" << endl;
		result = -1;
	}

	return result;
}