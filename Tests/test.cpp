#include "gtest/gtest.h"
#include "QRCode.h"
#include <string_view>
#include <optional>

namespace QR
{
	std::uint8_t GetAlphanumericCode(std::uint8_t);
	Mode GetMinimalMode(std::uint8_t leadingByte, std::optional<std::uint8_t> trailerByte = std::optional<std::uint8_t>());
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
}

TEST(Encode, SymbolCapacity)
{
	EXPECT_THROW(QR::Encode("012345", QR::SymbolType::MICRO_QR, 1, QR::ErrorCorrectionLevel::ERROR_DETECTION_ONLY, QR::Mode::NUMERIC), std::length_error);
	EXPECT_THROW(QR::Encode("01234567890", QR::SymbolType::MICRO_QR, 2, QR::ErrorCorrectionLevel::L, QR::Mode::NUMERIC), std::length_error);
	EXPECT_THROW(QR::Encode("012345678", QR::SymbolType::MICRO_QR, 2, QR::ErrorCorrectionLevel::M, QR::Mode::NUMERIC), std::length_error);
}