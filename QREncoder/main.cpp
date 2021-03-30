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
	int result = 0;

	if (argc == 1)
	{
		cout << "Usage: " << argv[0] << " -[M]V-E -numeric|alpha|byte message -output filename\n"
			<< "M: Indicates that the output will be a Micro QR symbol\n"
			<< "V: Indicates version number. Max is 40 for QR symbols and 4 for Micro QR symbols\n"
			<< "E: Error correction levels. Valid values are L, M, Q, H"
			<< "Symbol version must be the first argument, the rest of the arguments may appear in any order" << endl;
	}
	else if (std::regex_match(argv[1], results, versionFormat) || argv[1] == std::string_view("-M1"))
	{
		std::ofstream output;
		std::string filename;
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
			
			for (int i = 2; i < argc - 1; ++i)
			{
				decltype(modes)::iterator modeIt;
				std::string_view argument(argv[i]);

				if ((modeIt = modes.find(argument)) != modes.end())
				{
					encoder.addCharacters(argv[i + 1], modeIt->second);
					++i;
				}
				else if (argument == "-output")
				{
					filename = argv[i + 1];
					++i;
				}
			}

			auto qr = encoder.generateMatrix();

			output.open(filename, std::ios_base::binary);

			if (output.is_open())
				output << QRToBMP(qr, 4);
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