#include <fstream>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <regex>
#include "Image.h"
#include "QRCode.h"

BMPImage QRToBMP(const std::vector<std::vector<bool>> &code, unsigned multiplier)
{
	BMPImage result(code.size() * multiplier, code.size() * multiplier, 1);

	for (std::vector<std::vector<bool>>::size_type i = 0; i < code.size(); ++i)
		for (std::vector<std::vector<bool>>::size_type j = 0; j < code[i].size(); ++j)
				for (decltype(i) y = 0; y < multiplier; ++y)
					for (decltype(j) x = 0; x < multiplier; ++x)
					{
						Point point;

						point.mX = j * multiplier + x;
						point.mY = i * multiplier + y;

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
	using std::cerr;
	using std::endl;
	int result = 0;

	if (argc == 4)
	{
		std::ofstream output;
		std::regex versionFormat("(M)?([[:digit:]]{1,2})-([LMQH])");
		std::cmatch results;

		if (std::regex_match(argv[2], results, versionFormat))
		{
			std::unordered_map<char, QR::ErrorCorrectionLevel> levels = {
				{ 'L', QR::ErrorCorrectionLevel::L },
				{ 'M', QR::ErrorCorrectionLevel::M },
				{ 'Q', QR::ErrorCorrectionLevel::Q },
				{ 'H', QR::ErrorCorrectionLevel::H }
			};
			QR::SymbolType type = results[1].matched ? QR::SymbolType::MICRO_QR : QR::SymbolType::QR;
			std::uint8_t version = std::stoul(results[2]);

			output.open(argv[3], std::ios_base::binary);

			if (output.is_open())
				output << QRToBMP(QR::Encode(argv[1], type, version, levels.at(results[3].str().front()), QR::Mode::BYTE), 8);
			else
			{
				cerr << "Could not open file" << endl;
				result = -1;
			}
		}
		else
			if (std::string_view(argv[2]) == "M1")
			{
				std::ofstream output(argv[3], std::ios_base::binary);

				if (output.is_open())
					output << QRToBMP(QR::Encode(argv[1], QR::SymbolType::MICRO_QR, 1, QR::ErrorCorrectionLevel::ERROR_DETECTION_ONLY, QR::Mode::BYTE), 8);
				else
				{
					cerr << "Could not open file" << endl;
					result = -1;
				}
			}
			else
			{
				cerr << "Invalid version" << endl;
				result = -1;
			}
	}
	else
	{
		cout	<< "Usage: " << argv[0] << " message [M]V-E output_file\n"
				<< "M: Indicates that the output will be a Micro QR symbol\n"
				<< "V: Indicates version number. Max is 40 for QR symbols and 4 for Micro QR symbols\n"
				<< "E: Error correction levels. Valid values are L, M, Q, H" << endl;
	}

	return result;
}