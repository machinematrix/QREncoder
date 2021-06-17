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
	using Symbol = std::vector<std::vector<bool>>;

	class Encoder final
	{
		struct Impl;
		std::unique_ptr<Impl> mImpl;
	public:
		Encoder(SymbolType type, unsigned version, ErrorCorrectionLevel level);
		Encoder(const Encoder&);
		Encoder(Encoder&&) noexcept;
		Encoder& operator=(const Encoder&);
		Encoder& operator=(Encoder&&) noexcept;
		~Encoder();

		void addCharacters(std::string_view message, Mode mode);
		//Clear bit stream
		void clear();
		Symbol generateMatrix() const;
		unsigned getVersion() const;
		SymbolType getSymbolType() const;
		ErrorCorrectionLevel getErrorCorrectionLevel() const;
	};
}

#endif