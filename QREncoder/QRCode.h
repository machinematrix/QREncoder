#ifndef QRCODE_H
#define QRCODE_H
#include <vector>
#include <string_view>

namespace QR
{
	enum class SymbolType { NORMAL, MICRO };
	enum class Mode { NUMERIC, ALPHANUMERIC, BYTE, KANJI };

	std::vector<std::vector<bool>> encode(std::string_view message, SymbolType type, std::uint8_t version);
}
#endif