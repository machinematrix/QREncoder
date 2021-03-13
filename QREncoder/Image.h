#ifndef IMAGE_H
#define IMAGE_H
#include <fstream>
#include <memory>
#include <string_view>

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
	friend std::ofstream& operator<<(std::ofstream&, const BMPImage&);
public:
	//BMPImage(std::string_view fileName);
	BMPImage(std::uint16_t width, std::uint16_t height, std::uint8_t bitsPerPixel = 32);
	BMPImage(const BMPImage&);
	BMPImage(BMPImage&&);
	~BMPImage();
	void setPixelColor(Point point, Color color);
	Color getPixelColor(Point point);
	Dimensions getDimensions();
};

std::ofstream& operator<<(std::ofstream&, const BMPImage&);

#endif