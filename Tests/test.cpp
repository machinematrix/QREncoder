#include "gtest/gtest.h"
#include "QRCode.h"
#include <string_view>
#include <optional>
#include <unordered_map>
#include <map>
#include <tuple>
#include <algorithm>
#include <execution>

namespace QR
{
	template <typename CharacterType> std::uint8_t GetAlphanumericCode(CharacterType);
	Mode GetMinimalMode(std::uint8_t, std::optional<std::uint8_t> = std::optional<std::uint8_t>());
	template <typename CharacterType> std::uint16_t ToInteger(const std::vector<CharacterType>&);
	bool IsKanji(std::uint16_t);
	unsigned GetSymbolRating(const std::vector<std::vector<bool>> &symbol, SymbolType type);
	std::vector<bool> GetECISequence(unsigned assignmentNumber);
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

	EXPECT_THROW(encoder.addCharacters("\xAE\x8A\xFF", QR::Mode::KANJI), std::invalid_argument); //odd byte count
	EXPECT_NO_THROW(encoder.addCharacters("\xAE\x8A", QR::Mode::KANJI)); //ok
	EXPECT_THROW(encoder.addCharacters("\xFF\xFF", QR::Mode::KANJI), std::invalid_argument); //not Kanji
	EXPECT_THROW(micro1.addCharacters("\xAE\x8A", QR::Mode::KANJI), std::invalid_argument); //Kanji not supported
	EXPECT_THROW(micro2.addCharacters("\xAE\x8A", QR::Mode::KANJI), std::invalid_argument); //Kanji not supported
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
	std::wstring_view alphaNumericTableWide(L"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ $%*+-./:");

	for (decltype(alphaNumericTable)::size_type i = 0; i < alphaNumericTable.size(); ++i)
		EXPECT_EQ(i, QR::GetAlphanumericCode(alphaNumericTable[i]));

	for (decltype(alphaNumericTableWide)::size_type i = 0; i < alphaNumericTableWide.size(); ++i)
		EXPECT_EQ(i, QR::GetAlphanumericCode(alphaNumericTableWide[i]));
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
	EXPECT_EQ(QR::ToInteger<char>({ '0', '1', '2' }), 12);
	EXPECT_EQ(QR::ToInteger<wchar_t>({ '0', '1', '2' }), 12);
}

TEST(ToInteger, Remainder)
{
	EXPECT_EQ(QR::ToInteger<char>({ '6', '7' }), 67);
	EXPECT_EQ(QR::ToInteger<wchar_t>({ '6', '7' }), 67);
	EXPECT_EQ(QR::ToInteger<char>({ '8' }), 8);
	EXPECT_EQ(QR::ToInteger<wchar_t>({ '8' }), 8);
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