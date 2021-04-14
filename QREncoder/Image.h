#ifndef IMAGE_H
#define IMAGE_H
#include <fstream>
#include <memory>
#include <vector>

namespace QR
{
	struct Dimensions
	{
		std::uint16_t mWidth;
		std::uint16_t mHeight;
	};

	struct Point
	{
		std::uint16_t mX;
		std::uint16_t mY;
	};

	struct Color
	{
		std::uint8_t mRed;
		std::uint8_t mGreen;
		std::uint8_t mBlue;
	};

	class BMPImage
	{
		struct Impl;
		std::unique_ptr<Impl> mImpl;
		friend std::ofstream &operator<<(std::ofstream &, const BMPImage &);
	public:
		BMPImage(std::uint16_t width, std::uint16_t height, std::uint8_t bitsPerPixel = 32);
		BMPImage(const BMPImage &);
		BMPImage(BMPImage &&) noexcept;
		~BMPImage();
		void setPixelColor(Point point, Color color);
		Color getPixelColor(Point point);
		Dimensions getDimensions();
	};

	std::ofstream &operator<<(std::ofstream &, const BMPImage &);
	BMPImage QRToBMP(const std::vector<std::vector<bool>> &code, unsigned multiplier, Color lightModuleColor, Color darkModuleColor);
	bool operator==(Color, Color);
}

#endif