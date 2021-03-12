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
		BMPImage image(width, height, 1);

		for (std::uint16_t i = 0; i < height; ++i)
		{
			for (std::uint16_t j = 0; j < width; ++j)
			{
				Color color;
				std::uint16_t xModulo = j % 2, yModulo = i % 2;

				if (/*(xModulo || yModulo) && !(xModulo && yModulo)*/!((i * j) % 2 + (i * j) % 3))
					color = { 0, 0, 0 };
				else
					color = { 255, 255, 255 };

				image.setPixelColor({ j, i }, color);
			}

			//image.setPixelColor({ i, i }, { 0, 0, 128 });
		}

		auto color = image.getPixelColor({ 0, 0 });
		auto color2 = image.getPixelColor({ 1, 0 });
		auto color3 = image.getPixelColor({ 2, 0 });

		output << image;
	}

	return 0;
}