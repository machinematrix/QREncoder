#ifndef QRCODE_H
#define QRCODE_H
#include <vector>
#include <string_view>

namespace QR
{
	enum class SymbolType : std::uint8_t { QR, MICRO_QR };
	enum class ErrorCorrectionLevel : std::uint8_t { L, M, Q, H, ERROR_DETECTION_ONLY };
	enum class Mode : std::uint8_t { NUMERIC, ALPHANUMERIC, BYTE, KANJI };

	std::vector<std::vector<bool>> Encode(std::string_view message, SymbolType type, std::uint8_t version, ErrorCorrectionLevel mLevel, Mode mode);
}

#endif