#ifndef QRCODE_H
#define QRCODE_H
#include <vector>
#include <string_view>

namespace QR
{
	enum class SymbolType : std::uint8_t { QR, MICRO_QR };
	enum class ErrorCorrectionLevel : std::uint8_t { L, M, Q, H, NONE };
	enum class Mode : std::uint8_t { NUMERIC, ALPHANUMERIC, BYTE, KANJI };
	struct Version
	{
		std::uint8_t mVersion;
		ErrorCorrectionLevel mLevel;
	};

	std::vector<std::vector<bool>> Encode(std::string_view message, SymbolType type, Version version, Mode mode);
}

QR::Version operator""_L(std::uint64_t);
QR::Version operator""_M(std::uint64_t);
QR::Version operator""_Q(std::uint64_t);
QR::Version operator""_H(std::uint64_t);

#endif