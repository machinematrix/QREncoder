#include <windows.h>
#include <fstream>
#include <array>
#include <vector>
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

int main()
{
	std::ofstream output("image.bmp", std::ios_base::binary);

	if (output.is_open())
	{
		BMPImage image = QRToBMP(QR::Encode("HELLO WORLD", QR::SymbolType::QR, 2_Q, QR::Mode::BYTE), 1);
		output << image;
	}

	return 0;
}