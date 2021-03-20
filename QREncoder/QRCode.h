#ifndef QRCODE_H
#define QRCODE_H
#include <vector>
#include <string_view>

namespace QR
{
	enum class SymbolType : std::uint8_t { QR, MICRO_QR };
	enum class ErrorCorrectionLevel : std::uint8_t { L, M, Q, H, ERROR_DETECTION_ONLY };
	enum class Mode : std::uint8_t { NUMERIC, ALPHANUMERIC, BYTE, KANJI };
	using ModeRange = std::tuple<Mode, size_t, size_t>; //<type, [from, to)>

	std::vector<std::vector<bool>> Encode(std::string_view message, SymbolType type, std::uint8_t version, ErrorCorrectionLevel level, const std::vector<ModeRange> &modes);

	//Single mode for the whole message
	std::vector<std::vector<bool>> Encode(std::string_view message, SymbolType type, std::uint8_t version, ErrorCorrectionLevel level, Mode mode);
}

#endif