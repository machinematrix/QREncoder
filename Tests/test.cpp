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

TEST(Encode, SymbolCapacity)
{
	EXPECT_THROW(QR::Encode<char>("012345", QR::SymbolType::MICRO_QR, 1, QR::ErrorCorrectionLevel::ERROR_DETECTION_ONLY, QR::Mode::NUMERIC), std::length_error);
	EXPECT_THROW(QR::Encode<char>("01234567890", QR::SymbolType::MICRO_QR, 2, QR::ErrorCorrectionLevel::L, QR::Mode::NUMERIC), std::length_error);
	EXPECT_THROW(QR::Encode<char>("012345678", QR::SymbolType::MICRO_QR, 2, QR::ErrorCorrectionLevel::M, QR::Mode::NUMERIC), std::length_error);
}

TEST(Encode, OutputEquality)
{
	std::unordered_map<QR::Mode, std::tuple<std::string_view, std::wstring_view>> inputMap = {
		{ QR::Mode::NUMERIC, { "01234", L"01234" } },
		{ QR::Mode::ALPHANUMERIC, { "ABC123$", L"ABC123$" } },
		{ QR::Mode::BYTE, { "\x16\x59", L"\x5916" } },
		{ QR::Mode::KANJI, { "\xBE\x8C\xBE\x8C", L"\x8CBE\x8CBE" } }
	};
	std::vector<std::uint8_t> versions;

	for (std::uint8_t version = 1; version <= 4; ++version)
		EXPECT_EQ(QR::Encode(std::get<0>(inputMap.at(QR::Mode::NUMERIC)), QR::SymbolType::MICRO_QR, version, version == 1 ? QR::ErrorCorrectionLevel::ERROR_DETECTION_ONLY : QR::ErrorCorrectionLevel::L, QR::Mode::NUMERIC),
				  QR::Encode(std::get<1>(inputMap.at(QR::Mode::NUMERIC)), QR::SymbolType::MICRO_QR, version, version == 1 ? QR::ErrorCorrectionLevel::ERROR_DETECTION_ONLY : QR::ErrorCorrectionLevel::L, QR::Mode::NUMERIC));

	for (std::uint8_t version = 1; version <= 40; ++version)
		versions.push_back(version);

	for (auto mode : { QR::Mode::KANJI, QR::Mode::NUMERIC, QR::Mode::ALPHANUMERIC, QR::Mode::BYTE })
		std::for_each(std::execution::par, versions.cbegin(), versions.cend(), [mode, &inputMap](std::uint8_t version) {
		EXPECT_EQ(QR::Encode(std::get<0>(inputMap.at(mode)), QR::SymbolType::QR, version, QR::ErrorCorrectionLevel::L, mode),
				  QR::Encode(std::get<1>(inputMap.at(mode)), QR::SymbolType::QR, version, QR::ErrorCorrectionLevel::L, mode));
		});
}

TEST(Encode, KanjiOddByteCount)
{
	EXPECT_THROW(QR::Encode<char>("\xBE\x8C\xBE", QR::SymbolType::QR, 1, QR::ErrorCorrectionLevel::L, QR::Mode::KANJI), std::invalid_argument);
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