#include <windows.h>
#include <fstream>
#include <array>
#include <vector>
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
		//short width = 159, height = 159;
		//BMPImage image(width, height, 1);

		//image.setPixelColor({ 1, 1 }, { 0, 0, 0 });

		//for (std::uint16_t i = 0; i < height; ++i)
		//{
		//	for (std::uint16_t j = 0; j < width; ++j)
		//	{
		//		Color color/* = { 255, 255, 255 }*/;
		//		std::uint16_t xModulo = j % 2, yModulo = i % 2;

		//		if (/*(xModulo || yModulo) && !(xModulo && yModulo)*/!((i * j) % 2 + (i * j) % 3))
		//			color = { 0, 0, 0 };
		//		else
		//			color = { 255, 255, 255 };

		//		image.setPixelColor({ j, i }, color);
		//	}

		//	//image.setPixelColor({ i, i }, { 0, 0, 128 });
		//}

		//auto color = image.getPixelColor({ 0, 0 });
		//auto color2 = image.getPixelColor({ 1, 0 });
		//auto color3 = image.getPixelColor({ 2, 0 });
		BMPImage image = QRToBMP(QR::Encode("asd", QR::Mode::ALPHANUMERIC, QR::SymbolType::NORMAL, 1), 1);

		output << image;
	}

	return 0;
}