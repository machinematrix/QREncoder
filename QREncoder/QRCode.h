#ifndef QRCODE_H
#define QRCODE_H
#include <vector>
#include <string_view>
#include <memory>

namespace QR
{
	enum class SymbolType { NORMAL, MICRO };
	enum class Mode { NUMERIC, ALPHANUMERIC, BYTE, KANJI };
	enum class ErrorCorrectionLevel { L, M, Q, H };
	struct Version
	{
		std::uint8_t mVersion;
		ErrorCorrectionLevel mLevel;
	};

	std::vector<std::vector<bool>> Encode(std::string_view message, SymbolType type, Version version);
}

QR::Version operator""_L(std::uint64_t);
QR::Version operator""_M(std::uint64_t);
QR::Version operator""_Q(std::uint64_t);
QR::Version operator""_H(std::uint64_t);

#endif