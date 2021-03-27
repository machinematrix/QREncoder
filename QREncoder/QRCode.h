#ifndef QRCODE_H
#define QRCODE_H
#include <vector>
#include <string_view>
#include <memory>

namespace QR
{
	enum class SymbolType : std::uint8_t { QR, MICRO_QR };
	enum class ErrorCorrectionLevel : std::uint8_t { L, M, Q, H, ERROR_DETECTION_ONLY };
	enum class Mode : std::uint8_t { NUMERIC, ALPHANUMERIC, BYTE, KANJI };
	using ModeRange = std::tuple<Mode, size_t, size_t>; //<type, [from, to)>

	template <typename CharacterType>
	std::vector<std::vector<bool>> Encode(std::basic_string_view<CharacterType> message, SymbolType type, std::uint8_t version, ErrorCorrectionLevel level, const std::vector<ModeRange> &modes);

	//Single mode for the whole message
	template <typename CharacterType>
	std::vector<std::vector<bool>> Encode(std::basic_string_view<CharacterType> message, SymbolType type, std::uint8_t version, ErrorCorrectionLevel level, Mode mode);

	class QREncoder
	{
		struct Impl;
		std::unique_ptr<Impl> mImpl;
	public:
		QREncoder(SymbolType type, unsigned version, ErrorCorrectionLevel level);
		QREncoder(const QREncoder&);
		QREncoder(QREncoder&&) = default;
		QREncoder& operator=(const QREncoder&);
		QREncoder& operator=(QREncoder&&) = default;
		~QREncoder();

		template <typename CharacterType>
		void addCharacters(std::basic_string_view<CharacterType> message, Mode mode);
		std::vector<std::vector<bool>> generateMatrix();
	};
}

#endif