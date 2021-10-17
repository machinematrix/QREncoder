#include "gtest/gtest.h"
#include "QREncoder.h"
#include <optional>
#include <concepts>
#include <charconv>
#include <string_view>

namespace QR
{
	std::uint8_t GetAlphanumericCode(std::string::value_type);
	Mode GetMinimalMode(std::uint8_t, std::optional<std::uint8_t> = std::optional<std::uint8_t>());
	std::uint16_t ToInteger(const std::vector<std::string::value_type> &);
	bool IsKanji(std::uint16_t);
	unsigned GetSymbolRating(const std::vector<std::vector<bool>> &symbol, SymbolType type);
	std::vector<bool> GetECISequence(unsigned assignmentNumber);
}

namespace
{
	std::string ToString(const std::vector<bool> &input)
	{
		std::string result;

		for (bool element : input)
			result += std::to_string(element);

		return result;
	}
}

TEST(Encoder_addCharacters, ECI)
{
	QR::Encoder encoder(QR::SymbolType::MICRO_QR, 4, QR::ErrorCorrectionLevel::L);

	EXPECT_THROW(encoder.addCharacters("\\000009\xC1\xC2\xC3\xC4\xC5", QR::Mode::BYTE), std::invalid_argument);
}

TEST(Encoder_addCharacters, NumericMode)
{
	QR::Encoder encoder(QR::SymbolType::QR, 1, QR::ErrorCorrectionLevel::L);

	EXPECT_THROW(encoder.addCharacters("abc", QR::Mode::NUMERIC), std::invalid_argument); //NaN
}

TEST(Encoder_addCharacters, AlphanumericMode)
{
	QR::Encoder encoder(QR::SymbolType::QR, 1, QR::ErrorCorrectionLevel::L);

	EXPECT_THROW(encoder.addCharacters("&|", QR::Mode::ALPHANUMERIC), std::invalid_argument); //Not alphanumeric characters
}

TEST(Encoder_addCharacters, KanjiMode)
{
	QR::Encoder encoder(QR::SymbolType::QR, 3, QR::ErrorCorrectionLevel::L);
	QR::Encoder micro1(QR::SymbolType::MICRO_QR, 1, QR::ErrorCorrectionLevel::ERROR_DETECTION_ONLY);
	QR::Encoder micro2(QR::SymbolType::MICRO_QR, 2, QR::ErrorCorrectionLevel::L);

	EXPECT_THROW(encoder.addCharacters("\x8A\xAE\xFF", QR::Mode::KANJI), std::invalid_argument); //odd byte count
	EXPECT_NO_THROW(encoder.addCharacters("\x8A\xAE", QR::Mode::KANJI)); //ok
	EXPECT_THROW(encoder.addCharacters("\xFF\xFF", QR::Mode::KANJI), std::invalid_argument); //not Kanji
	EXPECT_THROW(micro1.addCharacters("\x8A\xAE", QR::Mode::KANJI), std::invalid_argument); //Kanji not supported
	EXPECT_THROW(micro2.addCharacters("\x8A\xAE", QR::Mode::KANJI), std::invalid_argument); //Kanji not supported
}

TEST(Encoder_addCharacters, SymbolCapacity)
{
	QR::Encoder m1(QR::SymbolType::MICRO_QR, 1, QR::ErrorCorrectionLevel::ERROR_DETECTION_ONLY);
	QR::Encoder m2l(QR::SymbolType::MICRO_QR, 2, QR::ErrorCorrectionLevel::L);
	QR::Encoder m2m(QR::SymbolType::MICRO_QR, 2, QR::ErrorCorrectionLevel::M);

	EXPECT_THROW(m1.addCharacters("012345", QR::Mode::NUMERIC), std::length_error);
	EXPECT_THROW(m1.addCharacters("01234567890", QR::Mode::NUMERIC), std::length_error);
	EXPECT_THROW(m1.addCharacters("012345678", QR::Mode::NUMERIC), std::length_error);
}

TEST(Encoder_addCharacters, OddKanjiByteCount)
{
	QR::Encoder encoder(QR::SymbolType::QR, 1, QR::ErrorCorrectionLevel::L);

	EXPECT_THROW(encoder.addCharacters("\xBE\x8C\xBE", QR::Mode::KANJI), std::invalid_argument);
}

TEST(GetECISequence, General)
{
	EXPECT_EQ(QR::GetECISequence(9), (std::vector<bool>{ 0, 1, 1, 1, 0, 0, 0, 0, 1, 0, 0, 1 }));
	EXPECT_EQ(QR::GetECISequence(16382), (std::vector<bool>{ 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0 }));
	EXPECT_EQ(QR::GetECISequence(999997), (std::vector<bool>{ 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 1, 0, 1 }));
	EXPECT_THROW(QR::GetECISequence(1000000), std::invalid_argument);
}

TEST(GetSymbolRating, General)
{
	std::vector<std::vector<bool>> symbol1 = {
		{ 1,1,1,1,1,1,1,0,1,1,0,0,0,0,1,1,1,1,1,1,1, },
		{ 1,0,0,0,0,0,1,0,1,0,0,1,0,0,1,0,0,0,0,0,1, },
		{ 1,0,1,1,1,0,1,0,1,0,0,1,1,0,1,0,1,1,1,0,1, },
		{ 1,0,1,1,1,0,1,0,1,0,0,0,0,0,1,0,1,1,1,0,1, },
		{ 1,0,1,1,1,0,1,0,1,0,1,0,0,0,1,0,1,1,1,0,1, },
		{ 1,0,0,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,0,0,1, },
		{ 1,1,1,1,1,1,1,0,1,0,1,0,1,0,1,1,1,1,1,1,1, },
		{ 0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0, },
		{ 0,1,1,0,1,0,1,1,0,0,0,0,1,0,1,0,1,1,1,1,1, },
		{ 0,1,0,0,0,0,0,0,1,1,1,1,0,0,0,0,1,0,0,0,1, },
		{ 0,0,1,1,0,1,1,1,0,1,1,0,0,0,1,0,1,1,0,0,0, },
		{ 0,1,1,0,1,1,0,1,0,0,1,1,0,1,0,1,0,1,1,1,0, },
		{ 1,0,0,0,1,0,1,0,1,0,1,1,1,0,1,1,1,0,1,0,1, },
		{ 0,0,0,0,0,0,0,0,1,1,0,1,0,0,1,0,0,0,1,0,1, },
		{ 1,1,1,1,1,1,1,0,1,0,1,0,0,0,0,1,0,1,1,0,0, },
		{ 1,0,0,0,0,0,1,0,0,1,0,1,1,0,1,1,0,1,0,0,0, },
		{ 1,0,1,1,1,0,1,0,1,0,1,0,0,0,1,1,1,1,1,1,1, },
		{ 1,0,1,1,1,0,1,0,0,1,0,1,0,1,0,1,0,0,0,1,0, },
		{ 1,0,1,1,1,0,1,0,1,0,0,0,1,1,1,1,0,1,0,0,1, },
		{ 1,0,0,0,0,0,1,0,1,0,1,1,0,1,0,0,0,1,0,1,1, },
		{ 1,1,1,1,1,1,1,0,0,0,0,0,1,1,1,1,0,0,0,0,1, },
	};

	std::vector<std::vector<bool>> symbol2 = {
		{ 1,1,1,1,1,1,1,0,1,1,0,0,0,0,1,1,1,1,1,1,1, },
		{ 1,0,0,0,0,0,1,0,0,0,1,1,0,0,1,0,0,0,0,0,1, },
		{ 1,0,1,1,1,0,1,0,1,0,0,0,1,0,1,0,1,1,1,0,1, },
		{ 1,0,1,1,1,0,1,0,1,0,0,0,0,0,1,0,1,1,1,0,1, },
		{ 1,0,1,1,1,0,1,0,0,0,0,0,0,0,1,0,1,1,1,0,1, },
		{ 1,0,0,0,0,0,1,0,1,0,1,1,0,0,1,0,0,0,0,0,1, },
		{ 1,1,1,1,1,1,1,0,1,0,1,0,1,0,1,1,1,1,1,1,1, },
		{ 0,0,0,0,0,0,0,0,1,0,1,0,0,0,0,0,0,0,0,0,0, },
		{ 0,1,0,1,0,1,1,1,1,0,0,1,1,1,1,1,0,1,1,0,1, },
		{ 0,1,0,0,0,0,0,0,1,1,1,1,0,0,0,0,1,0,0,0,1, },
		{ 0,1,1,1,1,1,1,0,0,1,0,0,0,1,1,0,0,1,0,1,0, },
		{ 0,1,0,0,1,0,0,1,1,0,1,0,0,1,1,1,0,0,1,1,1, },
		{ 1,0,0,0,1,0,1,0,1,0,1,1,1,0,1,1,1,0,1,0,1, },
		{ 0,0,0,0,0,0,0,0,1,1,1,1,0,1,1,0,1,0,1,1,1, },
		{ 1,1,1,1,1,1,1,0,1,0,1,1,0,0,1,1,0,0,1,0,1, },
		{ 1,0,0,0,0,0,1,0,1,1,0,1,1,0,1,1,0,1,0,0,0, },
		{ 1,0,1,1,1,0,1,0,0,0,0,0,0,1,1,1,0,1,1,0,1, },
		{ 1,0,1,1,1,0,1,0,1,1,0,0,0,1,1,1,0,1,0,1,1, },
		{ 1,0,1,1,1,0,1,0,0,0,0,0,1,1,1,1,0,1,0,0,1, },
		{ 1,0,0,0,0,0,1,0,1,0,0,1,0,0,0,0,1,1,0,0,1, },
		{ 1,1,1,1,1,1,1,0,0,0,0,1,1,1,0,1,0,1,0,0,0, },
	};

	EXPECT_EQ(QR::GetSymbolRating(symbol1, QR::SymbolType::QR), 350);
	EXPECT_EQ(QR::GetSymbolRating(symbol2, QR::SymbolType::QR), 520);
}

TEST(GetAlphanumericCode, ValidCharacters)
{
	std::string_view alphaNumericTable("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ $%*+-./:");

	for (decltype(alphaNumericTable)::size_type i = 0; i < alphaNumericTable.size(); ++i)
		EXPECT_EQ(i, QR::GetAlphanumericCode(alphaNumericTable[i]));
}

TEST(GetAlphanumericCode, InvalidCharacters)
{
	EXPECT_THROW(QR::GetAlphanumericCode('_'), std::invalid_argument);
	EXPECT_THROW(QR::GetAlphanumericCode(';'), std::invalid_argument);
	EXPECT_THROW(QR::GetAlphanumericCode('&'), std::invalid_argument);
}

TEST(GetMinimalMode, General)
{
	std::string_view alphaTable("ABCDEFGHIJKLMNOPQRSTUVWXYZ $%*+-./:");

	for (std::uint8_t c = '0'; c <= '9'; ++c)
		EXPECT_EQ(QR::GetMinimalMode(c), QR::Mode::NUMERIC);

	for (auto c : alphaTable)
		EXPECT_EQ(QR::GetMinimalMode(c), QR::Mode::ALPHANUMERIC);

	for (const std::pair<std::uint16_t, std::uint16_t> &kanjiRange : {
		std::make_pair(0x8140, 0x817E), std::make_pair(0x8180, 0x81FC),
		std::make_pair(0x9F40, 0X9F7E), std::make_pair(0x9F80, 0x9FFC),
		std::make_pair(0xE040, 0xE07E), std::make_pair(0xE080, 0xE0FC),
		std::make_pair(0xEA40, 0xEA7E), std::make_pair(0xEA80, 0xEAFC),
		std::make_pair(0xEB40, 0xEB7E), std::make_pair(0xEB80, 0xEBBF) })
		for (auto i = kanjiRange.first; i <= kanjiRange.second; ++i)
			EXPECT_EQ(QR::GetMinimalMode(i >> 8, i & 0xFF), QR::Mode::KANJI);
}

TEST(ToInteger, Triplets)
{
	EXPECT_EQ(QR::ToInteger({ '0', '1', '2' }), 12);
}

TEST(ToInteger, Remainder)
{
	EXPECT_EQ(QR::ToInteger({ '6', '7' }), 67);
	EXPECT_EQ(QR::ToInteger({ '8' }), 8);
}

TEST(IsKanji, General)
{
	EXPECT_TRUE(QR::IsKanji(0x817E));
	EXPECT_FALSE(QR::IsKanji(0x817F));
	EXPECT_TRUE(QR::IsKanji(0xEBBF));
	EXPECT_FALSE(QR::IsKanji(0xEBC0));
	EXPECT_FALSE(QR::IsKanji(0xFFFF));
	EXPECT_TRUE(QR::IsKanji(0x88AE));
}

TEST(BitStream, ECI) //Example in ISO/IEC 18004:2015, section 7.4.2.2
{
	QR::Encoder encoder { QR::SymbolType::QR, 1, QR::ErrorCorrectionLevel::H };

	encoder.addCharacters("\\000009\xA1\xA2\xA3\xA4\xA5", QR::Mode::BYTE);
	EXPECT_EQ(ToString(encoder.getBitStream()), "0111" "00001001" "0100" "00000101" "10100001" "10100010" "10100011" "10100100" "10100101");
}

TEST(BitStream, Numeric) //Example in ISO/IEC 18004:2015, section 7.4.3
{
	using namespace std::string_view_literals;
	QR::Encoder encoder { QR::SymbolType::QR, 1, QR::ErrorCorrectionLevel::H }, microEncoder { QR::SymbolType::MICRO_QR, 3, QR::ErrorCorrectionLevel::M };

	encoder.addCharacters("01234567", QR::Mode::NUMERIC);
	microEncoder.addCharacters("0123456789012345", QR::Mode::NUMERIC);
	EXPECT_EQ(ToString(encoder.getBitStream()), "0001" "0000001000" "0000001100" "0101011001" "1000011");
	EXPECT_EQ(ToString(microEncoder.getBitStream()), "00" "10000" "0000001100" "0101011001" "1010100110" "1110000101" "0011101010" "0101");
}

TEST(BitStream, Alphanumeric) //Example in ISO/IEC 18004:2015, section 7.4.4
{
	QR::Encoder encoder { QR::SymbolType::QR, 1, QR::ErrorCorrectionLevel::H };

	encoder.addCharacters("AC-42", QR::Mode::ALPHANUMERIC);
	EXPECT_EQ(ToString(encoder.getBitStream()), "0010" "000000101" "00111001110" "11100111001" "000010");
}

TEST(BitStream, Byte)
{
	QR::Encoder encoder { QR::SymbolType::QR, 1, QR::ErrorCorrectionLevel::H };

	encoder.addCharacters("\xAB\xA7\xA9\xAD\xAE", QR::Mode::BYTE);
	EXPECT_EQ(ToString(encoder.getBitStream()), "0100" "00000101" "10101011" "10100111" "10101001" "10101101" "10101110");
}

TEST(BitStream, Kanji)
{
	QR::Encoder encoder { QR::SymbolType::QR, 1, QR::ErrorCorrectionLevel::H };

	encoder.addCharacters("\x93\x5F\xE4\xAA\x93\x5F\xE4\xAA", QR::Mode::KANJI);
	EXPECT_EQ(ToString(encoder.getBitStream()), "1000" "00000100" "0110110011111" "1101010101010" "0110110011111" "1101010101010");
}