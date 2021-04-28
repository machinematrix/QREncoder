#include "gtest/gtest.h"
#include "Image.h"
#include <array>

TEST(Bitmap, ColorTableSize)
{
	for (int bitCount : { 1, 4, 8 })
	{
		QR::BMPImage bitmap(128, 128, bitCount);

		for (int i = 0; i < 1 << bitCount; ++i) //Fill the color table
		{
			std::uint8_t color = static_cast<std::uint8_t>(i);
			EXPECT_NO_THROW(bitmap.setPixelColor({ 0, 0 }, { color, color, color }));
		}

		EXPECT_THROW(bitmap.setPixelColor({ 0, 0 }, { 255, 0, 0 }), std::runtime_error); //Try to add a new color after the color table has been filled
	}
}

TEST(Bitmap, PixelColor)
{
	std::array<QR::Color, 5> colors = { { { 255, 0, 0 }, { 0, 255, 0 }, { 0, 0, 255 }, { 0, 0, 0 }, { 255, 255, 255 } } };

	for (int bitCount : { 1, 4, 8, 16, 24, 32 })
	{
		QR::BMPImage bitmap(128, 128, bitCount);

		for (size_t i = 0, sz = std::min(size_t{ 1 } << bitCount, colors.size()); i < sz; ++i)
		{
			QR::Color color;
			bitmap.setPixelColor({ 127, 127 }, colors[i]);
			color = bitmap.getPixelColor({ 127, 127 });
			EXPECT_EQ(color, colors[i]);
		}
	}
}