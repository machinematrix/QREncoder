#include "Image.h"
#include <windows.h>
#include <fstream>
#include <vector>
#include <map>
#include <bitset>

bool operator<(RGBQUAD lhs, RGBQUAD rhs)
{
	return lhs.rgbBlue < rhs.rgbBlue
		|| lhs.rgbBlue == rhs.rgbBlue && lhs.rgbGreen < rhs.rgbGreen
		|| lhs.rgbBlue == rhs.rgbBlue && lhs.rgbGreen == rhs.rgbGreen && lhs.rgbRed < rhs.rgbRed;
}

struct BMPImage::Impl //https://docs.microsoft.com/en-us/windows/win32/gdi/bitmap-storage
{
	BITMAPINFOHEADER mInfoHeader = {};
	std::vector<RGBQUAD> mColorTable;
	std::map<RGBQUAD, decltype(mColorTable)::size_type> mColorMap;
	std::vector<std::uint8_t> mBitmap;
};

//BMPImage::BMPImage(std::string_view fileName)
//	:mImpl(new Impl)
//{
//}

BMPImage::BMPImage(std::uint16_t width, std::uint16_t height, std::uint8_t bitsPerPixel)
	:mImpl(new Impl)
{
	if (width > 30000)
		throw std::invalid_argument("Invalid width");

	if (height > 30000)
		throw std::invalid_argument("Invalid height");

	switch (bitsPerPixel)
	{
		case 1:
		case 4:
		case 8:
			mImpl->mBitmap.resize(width * height / (8 / bitsPerPixel) + width * height % (8 / bitsPerPixel));
			break;

		case 16:
		case 24:
		case 32:
			mImpl->mBitmap.resize(width * height * (bitsPerPixel / 8));
			break;

		default:
			throw std::invalid_argument("Invalid bit count. Valid values are 1, 4, 8, 16, 24 and 32");
	}

	mImpl->mInfoHeader.biSize = sizeof(decltype(mImpl->mInfoHeader));
	mImpl->mInfoHeader.biWidth = width;
	mImpl->mInfoHeader.biHeight -= height; //top-down bitmap
	mImpl->mInfoHeader.biPlanes = 1;
	mImpl->mInfoHeader.biBitCount = bitsPerPixel;
	mImpl->mInfoHeader.biCompression = BI_RGB;
}

BMPImage::~BMPImage() = default;

void BMPImage::setPixelColor(Point point, Color color)
{
	decltype(mImpl->mBitmap)::size_type pos = (point.mY * mImpl->mInfoHeader.biWidth + point.mX) * mImpl->mInfoHeader.biBitCount;

	try
	{
		switch (mImpl->mInfoHeader.biBitCount)
		{
			case 1:
			case 4:
			case 8:
			{
				auto index = mImpl->mColorMap.find({ color.mBlue, color.mGreen, color.mRed });

				if (index == mImpl->mColorMap.end())
				{
					if (mImpl->mColorTable.size() < 1ull << mImpl->mInfoHeader.biBitCount)
					{
						mImpl->mColorTable.push_back({ color.mBlue, color.mGreen, color.mRed });
						index = mImpl->mColorMap.emplace(mImpl->mColorTable.back(), mImpl->mColorTable.size() - 1).first;
					}
					else
						throw std::runtime_error("Color table is full");
				}
				std::bitset<8> newByte = mImpl->mBitmap.at(pos / 8);

				for (size_t i = 0; i < mImpl->mInfoHeader.biBitCount; ++i)
					newByte.set(7 - pos % 8 - (mImpl->mInfoHeader.biBitCount - 1) + i, index->second & 1ull << i);

				mImpl->mBitmap.at(pos / 8) = static_cast<decltype(mImpl->mBitmap)::value_type>(newByte.to_ulong());
				break;
			}

			case 16:
				mImpl->mBitmap.at(pos / 8) = color.mBlue >> 3 | color.mGreen >> 3 << 5;
				mImpl->mBitmap.at(pos / 8 + 1) = color.mGreen >> 6 | color.mRed >> 3 << 2;
				break;

			case 24:
			case 32:
				mImpl->mBitmap.at(pos / 8) = color.mBlue;
				mImpl->mBitmap.at(pos / 8 + 1) = color.mGreen;
				mImpl->mBitmap.at(pos / 8 + 2) = color.mRed;
				break;
		}
	}
	catch (std::out_of_range &e)
	{
		e = std::out_of_range("Pixel coordinates out of range");
		throw;
	}
}

Color BMPImage::getPixelColor(Point point)
{
	decltype(mImpl->mBitmap)::size_type pos = (point.mY * mImpl->mInfoHeader.biWidth + point.mX) * mImpl->mInfoHeader.biBitCount;
	Color result = {};

	try
	{
		switch (mImpl->mInfoHeader.biBitCount)
		{
			case 1:
			case 4:
			case 8:
			{
				std::bitset<8> byte = mImpl->mBitmap.at(pos / 8);
				decltype(mImpl->mColorTable)::size_type index = 0;

				for (int i = 0; i < mImpl->mInfoHeader.biBitCount; ++i)
					index |= byte.test(7 - pos % 8 - (mImpl->mInfoHeader.biBitCount - 1) + i) << i;

				result.mRed = mImpl->mColorTable[index].rgbRed;
				result.mGreen = mImpl->mColorTable[index].rgbGreen;
				result.mBlue = mImpl->mColorTable[index].rgbBlue;

				break;
			}

			case 16:
				result.mBlue = (mImpl->mBitmap.at(pos / 8) & 0b11111) * 8; //lower 5 bits of the lower byte
				result.mGreen = (mImpl->mBitmap.at(pos / 8) >> 5 | mImpl->mBitmap.at(pos / 8 + 1) & 0b11 << 3) * 8; //higher 3 bits of the lower byte OR lower 2 bits of the higher byte
				result.mRed = (mImpl->mBitmap.at(pos / 8 + 1) << 1 >> 3) * 8; //5 bits starting from the second most significant bit of the higher byte

				if (result.mRed == 248)
					result.mRed = 255;

				if (result.mGreen == 248)
					result.mGreen = 255;

				if (result.mBlue == 248)
					result.mBlue = 255;

				break;

			case 24:
			case 32:
				result.mBlue = mImpl->mBitmap.at(pos / 8);
				result.mGreen = mImpl->mBitmap.at(pos / 8 + 1);
				result.mRed = mImpl->mBitmap.at(pos / 8 + 2);
				break;
		}
	}
	catch (std::out_of_range &e)
	{
		e = std::out_of_range("Pixel coordinates out of range");
		throw;
	}

	return result;
}

Dimensions BMPImage::getDimensions()
{
	return { static_cast<decltype(Dimensions::mWidth)>(mImpl->mInfoHeader.biWidth), static_cast<decltype(Dimensions::mWidth)>(mImpl->mInfoHeader.biHeight) };
}

std::ofstream& operator<<(std::ofstream &stream, const BMPImage &image)
{
	BITMAPFILEHEADER imageHeader = {};
	decltype(image.mImpl->mColorTable) colorTable;

	if (stream.tellp())
		throw std::runtime_error("Stream output position indicator must be at 0");

	if (image.mImpl->mInfoHeader.biBitCount == 1 || image.mImpl->mInfoHeader.biBitCount == 4 || image.mImpl->mInfoHeader.biBitCount == 8)
	{
		colorTable = image.mImpl->mColorTable;
		colorTable.resize(1ull << image.mImpl->mInfoHeader.biBitCount, { 0, 0, 0 });
	}

	imageHeader.bfType = MAKEWORD('B', 'M');
	imageHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + colorTable.size() * sizeof(decltype(colorTable)::value_type);
	imageHeader.bfSize = imageHeader.bfOffBits + image.mImpl->mBitmap.size() * sizeof(decltype(image.mImpl->mBitmap)::value_type);

	stream.write(reinterpret_cast<const char*>(&imageHeader), sizeof(imageHeader));
	stream.write(reinterpret_cast<const char*>(&image.mImpl->mInfoHeader), sizeof(image.mImpl->mInfoHeader));
	stream.write(reinterpret_cast<const char*>(colorTable.data()), static_cast<std::uint64_t>(colorTable.size()) * sizeof(decltype(colorTable)::value_type));
	stream.write(reinterpret_cast<const char*>(image.mImpl->mBitmap.data()), static_cast<std::uint64_t>(image.mImpl->mBitmap.size()) * sizeof(decltype(image.mImpl->mBitmap)::value_type));
	
	return stream;
}