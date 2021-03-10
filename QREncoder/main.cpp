#include <windows.h>
#include <fstream>
#include <array>
#include <vector>
#include "Image.h"

int main()
{
	std::ofstream output("image.bmp", std::ios_base::binary);

	if (output.is_open())
	{
		short width = 128, height = 128;
		BMPImage image(width, height, 32);

		for (std::uint16_t y = 0; y < height; ++y)
		{
			for (std::uint16_t x = 0; x < width; ++x)
			{
				Color color;
				std::uint16_t xModulo = x % 2, yModulo = y % 2;

				if ((xModulo || yModulo) && !(xModulo && yModulo))
					color = { 255, 255, 255};
				else
					color = { 0, 0, 0 };

				image.setPixelColor({ x, y }, color);
			}

			image.setPixelColor({ y, y }, { 0, 0, 128 });
		}

		auto color = image.getPixelColor({ 0, 0 });
		auto color2 = image.getPixelColor({ 1, 0 });
		auto color3 = image.getPixelColor({ 2, 0 });

		output << image;
	}

	return 0;
}