#include "QREncoder.h"
#include <stdexcept>
#include <array>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <sstream>
#include <tuple>
#include <bitset>
#include <algorithm>
#include <optional>
#include <regex>

namespace QR
{
	using Symbol = std::vector<std::vector<bool>>;
	using BlockLayoutVector = std::vector<std::tuple<unsigned, unsigned, unsigned>>;

	#ifndef TESTS
	namespace
	{
		#endif
		unsigned GetSymbolSize(SymbolType type, std::uint8_t version)
		{
			unsigned result = 0;

			switch (type)
			{
				case SymbolType::QR:
					if (version > 40)
						throw std::invalid_argument("Invalid version");
					result = 21u + (version - 1) * 4;
					break;

				case SymbolType::MICRO_QR:
					if (version > 4)
						throw std::invalid_argument("Invalid version");
					result = 11u + (version - 1) * 2;
					break;
			}

			return result;
		}

		//Divide by 8 to get data capacity in codewords, do % 8 to get remainder bits. Exceptions are M1 and M3, where the last data codeword is 4 bits long
		unsigned GetDataModuleCount(SymbolType type, std::uint8_t version)
		{
			static const std::array<unsigned, 4> microModuleCount = { 36, 80, 132, 192 };
			static const std::array<unsigned, 40> moduleCount = {
				208, 359, 567, 807, 1079, 1383, 1568, 1936,
				2336, 2768, 3232, 3728, 4256, 4651, 5243, 5867,
				6523, 7211, 7931, 8683, 9252, 10068, 10916, 11796,
				12708, 13652, 14628, 15371, 16411, 17483, 18587, 19723,
				20891, 22091, 23008, 24272, 25568, 26896, 28256, 29648
			};
			unsigned result;

			if (type == SymbolType::MICRO_QR)
				result = microModuleCount[version - 1];
			else
				result = moduleCount[version - 1];

			return result;
		}

		//From table 1, page 18
		unsigned GetRemainderBitCount(SymbolType type, std::uint8_t version)
		{
			unsigned result = 0;

			if (type == SymbolType::QR)
				result = GetDataModuleCount(type, version) % 8;

			return result;
		}

		const std::vector<unsigned>& GetPolynomialCoefficientExponents(unsigned errorCorrectionCodewordCount)
		{
			static const std::unordered_map<unsigned, std::vector<unsigned>> polynomials = {
				{ 2, { 25, 1 } },
				{ 5, { 113, 164, 166, 119, 10 } },
				{ 6, { 166, 0, 134, 5, 176, 15 } },
				{ 7, { 87, 229, 146, 149, 238, 102, 21 } },
				{ 8, { 175, 238, 208, 249, 215, 252, 196, 28 } },
				{ 10, { 251, 67, 46, 61, 118, 70, 64, 94, 32, 45 } },
				{ 13, { 74, 152, 176, 100, 86, 100, 106, 104, 130, 218, 206, 140, 78 } },
				{ 14, { 199, 249, 155, 48, 190, 124, 218, 137, 216, 87, 207, 59, 22, 91 } },
				{ 15, { 8, 183, 61, 91, 202, 37, 51, 58, 58, 237, 140, 124, 5, 99, 105 } },
				{ 16, { 120, 104, 107, 109, 102, 161, 76, 3, 91, 191, 147, 169, 182, 194, 225, 120 } },
				{ 17, { 43, 139, 206, 78, 43, 239, 123, 206, 214, 147, 24, 99, 150, 39, 243, 163, 136 } },
				{ 18, { 215, 234, 158, 94, 184, 97, 118, 170, 79, 187, 152, 148, 252, 179, 5, 98, 96, 153 } },
				{ 20, { 17, 60, 79, 50, 61, 163, 26, 187, 202, 180, 221, 225, 83, 239, 156, 164, 212, 212, 188, 190 } },
				{ 22, { 210, 171, 247, 242, 93, 230, 14, 109, 221, 53, 200, 74, 8, 172, 98, 80, 219, 134, 160, 105, 165, 231 } },
				{ 24, { 229, 121, 135, 48, 211, 117, 251, 126, 159, 180, 169, 152, 192, 226, 228, 218, 111, 0, 117, 232, 87, 96, 227, 21 } },
				{ 26, { 173, 125, 158, 2, 103, 182, 118, 17, 145, 201, 111, 28, 165, 53, 161, 21, 245, 142, 13, 102, 48, 227, 153, 145, 218, 70 } },
				{ 28, { 168, 223, 200, 104, 224, 234, 108, 180, 110, 190, 195, 147, 205, 27, 232, 201, 21, 43, 245, 87, 42, 195, 212, 119, 242, 37, 9, 123 } },
				{ 30, { 41, 173, 145, 152, 216, 31, 179, 182, 50, 48, 110, 86, 239, 96, 222, 125, 42, 173, 226, 193, 224, 130, 156, 37, 251, 216, 238, 40, 192, 180 } },
				{ 32, { 10, 6, 106, 190, 249, 167, 4, 67, 209, 138, 138, 32, 242, 123, 89, 27, 120, 185, 80, 156, 38, 69, 171, 60, 28, 222, 80, 52, 254, 185, 220, 241 }},
				{ 34, { 111, 77, 146, 94, 26, 21, 108, 19, 105, 94, 113, 193, 86, 140, 163, 125, 58, 158, 229, 239, 218, 103, 56, 70, 114, 61, 183, 129, 167, 13, 98, 62, 129, 51 } },
				{ 36, { 200, 183, 98, 16, 172, 31, 246, 234, 60, 152, 115, 0, 167, 152, 113, 248, 238, 107, 18, 63, 218, 37, 87, 210, 105, 177, 120, 74, 121, 196, 117, 251, 113, 233, 30, 120 } },
				{ 40, { 59, 116, 79, 161, 252, 98, 128, 205, 128, 161, 247, 57, 163, 56, 235, 106, 53, 26, 187, 174, 226, 104, 170, 7, 175, 35, 181, 114, 88, 41, 47, 163, 125, 134, 72, 20, 232, 53, 35, 15 } },
				{ 42, { 250, 103, 221, 230, 25, 18, 137, 231, 0, 3, 58, 242, 221, 191, 110, 84, 230, 8, 188, 106, 96, 147, 15, 131, 139, 34, 101, 223, 39, 101, 213, 199, 237, 254, 201, 123, 171, 162, 194, 117, 50, 96 } },
				{ 44, { 190, 7, 61, 121, 71, 246, 69, 55, 168, 188, 89, 243, 191, 25, 72, 123, 9, 145, 14, 247, 1, 238, 44, 78, 143, 62, 224, 126, 118, 114, 68, 163, 52, 194, 217, 147, 204, 169, 37, 130, 113, 102, 73, 181 } },
				{ 46, { 112, 94, 88, 112, 253, 224, 202, 115, 187, 99, 89, 5, 54, 113, 129, 44, 58, 16, 135, 216, 169, 211, 36, 1, 4, 96, 60, 241, 73, 104, 234, 8, 249, 245, 119, 174, 52, 25, 157, 224, 43, 202, 223, 19, 82, 15 } },
				{ 48, { 228, 25, 196, 130, 211, 146, 60, 24, 251, 90, 39, 102, 240, 61, 178, 63, 46, 123, 115, 18, 221, 111, 135, 160, 182, 205, 107, 206, 95, 150, 120, 184, 91, 21, 247, 156, 140, 238, 191, 11, 94, 227, 84, 50, 163, 39, 34, 108 } },
				{ 50, { 232, 125, 157, 161, 164, 9, 118, 46, 209, 99, 203, 193, 35, 3, 209, 111, 195, 242, 203, 225, 46, 13, 32, 160, 126, 209, 130, 160, 242, 215, 242, 75, 77, 42, 189, 32, 113, 65, 124, 69, 228, 114, 235, 175, 124, 170, 215, 232, 133, 205 } },
				{ 52, { 116, 50, 86, 186, 50, 220, 251, 89, 192, 46, 86, 127, 124, 19, 184, 233, 151, 215, 22, 14, 59, 145, 37, 242, 203, 134, 254, 89, 190, 94, 59, 65, 124, 113, 100, 233, 235, 121, 22, 76, 86, 97, 39, 242, 200, 220, 101, 33, 239, 254, 113, 51 } },
				{ 54, { 183, 26, 201, 87, 210, 221, 113, 21, 46, 65, 45, 50, 238, 184, 249, 225, 102, 58, 209, 218, 109, 165, 26, 95, 184, 192, 52, 245, 35, 254, 238, 175, 172, 79, 123, 25, 122, 43, 120, 108, 215, 80, 128, 201, 235, 8, 153, 59, 101, 31, 198, 76, 31, 156 } },
				{ 56, { 106, 120, 107, 157, 164, 216, 112, 116, 2, 91, 248, 163, 36, 201, 202, 229, 6, 144, 254, 155, 135, 208, 170, 209, 12, 139, 127, 142, 182, 249, 177, 174, 190, 28, 10, 85, 239, 184, 101, 124, 152, 206, 96, 23, 163, 61, 27, 196, 247, 151, 154, 202, 207, 20, 61, 10 } },
				{ 58, { 82, 116, 26, 247, 66, 27, 62, 107, 252, 182, 200, 185, 235, 55, 251, 242, 210, 144, 154, 237, 176, 141, 192, 248, 152, 249, 206, 85, 253, 142, 65, 165, 125, 23, 24, 30, 122, 240, 214, 6, 129, 218, 29, 145, 127, 134, 206, 245, 117, 29, 41, 63, 159, 142, 233, 125, 148, 123 } },
				{ 60, { 107, 140, 26, 12, 9, 141, 243, 197, 226, 197, 219, 45, 211, 101, 219, 120, 28, 181, 127, 6, 100, 247, 2, 205, 198, 57, 115, 219, 101, 109, 160, 82, 37, 38, 238, 49, 160, 209, 121, 86, 11, 124, 30, 181, 84, 25, 194, 87, 65, 102, 190, 220, 70, 27, 209, 16, 89, 7, 33, 240 } },
				{ 62, { 65, 202, 113, 98, 71, 223, 248, 118, 214, 94, 0, 122, 37, 23, 2, 228, 58, 121, 7, 105, 135, 78, 243, 118, 70, 76, 223, 89, 72, 50, 70, 111, 194, 17, 212, 126, 181, 35, 221, 117, 235, 11, 229, 149, 147, 123, 213, 40, 115, 6, 200, 100, 26, 246, 182, 218, 127, 215, 36, 186, 110, 106 } },
				{ 64, { 45, 51, 175, 9, 7, 158, 159, 49, 68, 119, 92, 123, 177, 204, 187, 254, 200, 78, 141, 149, 119, 26, 127, 53, 160, 93, 199, 212, 29, 24, 145, 156, 208, 150, 218, 209, 4, 216, 91, 47, 184, 146, 47, 140, 195, 195, 125, 242, 238, 63, 99, 108, 140, 230, 242, 31, 204, 11, 178, 243, 217, 156, 213, 231 } },
				{ 66, { 5, 118, 222, 180, 136, 136, 162, 51, 46, 117, 13, 215, 81, 17, 139, 247, 197, 171, 95, 173, 65, 137, 178, 68, 111, 95, 101, 41, 72, 214, 169, 197, 95, 7, 44, 154, 77, 111, 236, 40, 121, 143, 63, 87, 80, 253, 240, 126, 217, 77, 34, 232, 106, 50, 168, 82, 76, 146, 67, 106, 171, 25, 132, 93, 45, 105 } },
				{ 68, { 247, 159, 223, 33, 224, 93, 77, 70, 90, 160, 32, 254, 43, 150, 84, 101, 190, 205, 133, 52, 60, 202, 165, 220, 203, 151, 93, 84, 15, 84, 253, 173, 160, 89, 227, 52, 199, 97, 95, 231, 52, 177, 41, 125, 137, 241, 166, 225, 118, 2, 54, 32, 82, 215, 175, 198, 43, 238, 235, 27, 101, 184, 127, 3, 5, 8, 163, 238 } }
			};

			return polynomials.at(errorCorrectionCodewordCount);
		}

		unsigned GetAlphaValue(unsigned exponent)
		{
			static const std::array<unsigned, 256> values = {
				1, 2, 4, 8, 16, 32, 64, 128, 29, 58, 116, 232, 205, 135, 19, 38,
				76, 152, 45, 90, 180, 117, 234, 201, 143, 3, 6, 12, 24, 48, 96, 192,
				157, 39, 78, 156, 37, 74, 148, 53, 106, 212, 181, 119, 238, 193, 159, 35,
				70, 140, 5, 10, 20, 40, 80, 160, 93, 186, 105, 210, 185, 111, 222, 161,
				95, 190, 97, 194, 153, 47, 94, 188, 101, 202, 137, 15, 30, 60, 120, 240,
				253, 231, 211, 187, 107, 214, 177, 127, 254, 225, 223, 163, 91, 182, 113, 226,
				217, 175, 67, 134, 17, 34, 68, 136, 13, 26, 52, 104, 208, 189, 103, 206,
				129, 31, 62, 124, 248, 237, 199, 147, 59, 118, 236, 197, 151, 51, 102, 204,
				133, 23, 46, 92, 184, 109, 218, 169, 79, 158, 33, 66, 132, 21, 42, 84,
				168, 77, 154, 41, 82, 164, 85, 170, 73, 146, 57, 114, 228, 213, 183, 115,
				230, 209, 191, 99, 198, 145, 63, 126, 252, 229, 215, 179, 123, 246, 241, 255,
				227, 219, 171, 75, 150, 49, 98, 196, 149, 55, 110, 220, 165, 87, 174, 65,
				130, 25, 50, 100, 200, 141, 7, 14, 28, 56, 112, 224, 221, 167, 83, 166,
				81, 162, 89, 178, 121, 242, 249, 239, 195, 155, 43, 86, 172, 69, 138, 9,
				18, 36, 72, 144, 61, 122, 244, 245, 247, 243, 251, 235, 203, 139, 11, 22,
				44, 88, 176, 125, 250, 233, 207, 131, 27, 54, 108, 216, 173, 71, 142, 1
			};

			return values[exponent];
		}

		unsigned GetAlphaExponent(unsigned value)
		{
			static const std::array<unsigned, 255> exponents = {
				0, 1, 25, 2, 50, 26, 198, 3, 223, 51, 238, 27, 104, 199, 75,
				4, 100, 224, 14, 52, 141, 239, 129, 28, 193, 105, 248, 200, 8, 76, 113,
				5, 138, 101, 47, 225, 36, 15, 33, 53, 147, 142, 218, 240, 18, 130, 69,
				29, 181, 194, 125, 106, 39, 249, 185, 201, 154, 9, 120, 77, 228, 114, 166,
				6, 191, 139, 98, 102, 221, 48, 253, 226, 152, 37, 179, 16, 145, 34, 136,
				54, 208, 148, 206, 143, 150, 219, 189, 241, 210, 19, 92, 131, 56, 70, 64,
				30, 66, 182, 163, 195, 72, 126, 110, 107, 58, 40, 84, 250, 133, 186, 61,
				202, 94, 155, 159, 10, 21, 121, 43, 78, 212, 229, 172, 115, 243, 167, 87,
				7, 112, 192, 247, 140, 128, 99, 13, 103, 74, 222, 237, 49, 197, 254, 24,
				227, 165, 153, 119, 38, 184, 180, 124, 17, 68, 146, 217, 35, 32, 137, 46,
				55, 63, 209, 91, 149, 188, 207, 205, 144, 135, 151, 178, 220, 252, 190, 97,
				242, 86, 211, 171, 20, 42, 93, 158, 132, 60, 57, 83, 71, 109, 65, 162,
				31, 45, 67, 216, 183, 123, 164, 118, 196, 23, 73, 236, 127, 12, 111, 246,
				108, 161, 59, 82, 41, 157, 85, 170, 251, 96, 134, 177, 187, 204, 62, 90,
				203, 89, 95, 176, 156, 169, 160, 81, 11, 245, 22, 235, 122, 117, 44, 215,
				79, 174, 213, 233, 230, 231, 173, 232, 116, 214, 244, 234, 168, 80, 88, 175
			};

			return exponents[value - 1];
		}

		//From table 9, page 38. <count, total, data>
		BlockLayoutVector GetBlockLayout(SymbolType type, std::uint8_t version, ErrorCorrectionLevel level)
		{
			using std::array;
			using std::vector;
			using std::tuple;
			static const array<BlockLayoutVector, 4> microBlockLayouts = { {
				{ { 1, 5, 3 } },
				{ { 1, 10, 5 }, { 1, 10, 4 } },
				{ { 1, 17, 11 }, { 1, 17, 9 } },
				{ { 1, 24, 16 }, { 1, 24, 14 }, { 1, 24, 10 } },
			} };
			static const array<vector<BlockLayoutVector>, 40> blockLayouts = { {
				{ { { 1, 26, 19 } }, { { 1, 26, 16 } }, { { 1, 26, 13 } }, { { 1, 26, 9 } } },
				{ { { 1, 44, 34 } }, { { 1, 44, 28 } }, { { 1, 44, 22 } }, { { 1, 44, 16 } } },
				{ { { 1, 70, 55 } }, { { 1, 70, 44 } }, { { 2, 35, 17 } }, { { 2, 35, 13 } } },
				{ { { 1, 100, 80 } }, { { 2, 50, 32 } }, { { 2, 50, 24 } }, { { 4, 25, 9 } } },
				{ { { 1, 134, 108 } }, { { 2, 67, 43 } }, { { 2, 33, 15 }, { 2, 34, 16 } }, { { 2, 33, 11 }, { 2, 34, 12 } } },
				{ { { 2, 86, 68 } }, { { 4, 43, 27 } }, { { 4, 43, 19 } }, { { 4, 43, 15 } } },
				{ { { 2, 98, 78 } }, { { 4, 49, 31 } }, { { 2, 32, 14 }, { 4, 33, 15 } }, { { 4, 39, 13 }, { 1, 40, 14 } } },
				{ { { 2, 121, 97 } }, { { 2, 60, 38 }, { 2, 61, 39 } }, { { 4, 40, 18 }, { 2, 41, 19 } }, { { 4, 40, 14 }, { 2, 41, 15 } } },
				{ { { 2, 146, 116 } }, { { 3, 58, 36 }, { 2, 59, 37 } }, { { 4, 36, 16 }, { 4, 37, 17 } }, { { 4, 36, 12 }, { 4, 37, 13 } } },
				{ { { 2, 86, 68 }, { 2, 87, 69 } }, { { 4, 69, 43 }, { 1, 70, 44 } }, { { 6, 43, 19 }, { 2, 44, 20 } }, { { 6, 43, 15 }, { 2, 44, 16 } } },
				{ { { 4, 101, 81 } }, { { 1, 80, 50 }, { 4, 81, 51 } }, { { 4, 50, 22 }, { 4, 51, 23 } }, { { 3, 36, 12 }, { 8, 37, 13 } } },
				{ { { 2, 116, 92 }, { 2, 117, 93 } }, { { 6, 58, 36 }, { 2, 59, 37 } }, { { 4, 46, 20 }, { 6, 47, 21 } }, { { 7, 42, 14 }, { 4, 43, 15 } } },
				{ { { 4, 133, 107 } }, { { 8, 59, 37 }, { 1, 60, 38 } }, { { 8, 44, 20 }, { 4, 45, 21 } }, { { 12, 33, 11 }, { 4, 34, 12 } } },
				{ { { 3, 145, 115 }, { 1, 146, 116 } }, { { 4, 64, 40 }, { 5, 65, 41 } }, { { 11, 36, 16 }, { 5, 37, 17 } }, { { 11, 36, 12 }, { 5, 37, 13 } } },
				{ { { 5, 109, 87 }, { 1, 110, 88 } }, { { 5, 65, 41 }, { 5, 66, 42 } }, { { 5, 54, 24 }, { 7, 55, 25 } }, { { 11, 36, 12 }, { 7, 37, 13 } } },
				{ { { 5, 122, 98 }, { 1, 123, 99 } }, { { 7, 73, 45 }, { 3, 74, 46 } }, { { 15, 43, 19 }, { 2, 44, 20 } }, { { 3, 45, 15 }, { 13, 46, 16 } } },
				{ { { 1, 135, 107 }, { 5, 136, 108 } }, { { 10, 74, 46 }, { 1, 75, 47 } }, { { 1, 50, 22 }, { 15, 51, 23 } }, { { 2, 42, 14 }, { 17, 43, 15 } } },
				{ { { 5, 150, 120 }, { 1, 151, 121 } }, { { 9, 69, 43 }, { 4, 70, 44 } }, { { 17, 50, 22 }, { 1, 51, 23 } }, { { 2, 42, 14 }, { 19, 43, 15 } } },
				{ { { 3, 141, 113 }, { 4, 142, 114 } }, { { 3, 70, 44 }, { 11, 71, 45 } }, { { 17, 47, 21 }, { 4, 48, 22 } }, { { 9, 39, 13 }, { 16, 40, 14 } } },
				{ { { 3, 135, 107 }, { 5, 136, 108 } }, { { 3, 67, 41 }, { 13, 68, 42 } }, { { 15, 54, 24 }, { 5, 55, 25 } }, { { 15, 43, 15 }, { 10, 44, 16 } } },
				{ { { 4, 144, 116 }, { 4, 145, 117 } }, { { 17, 68, 42 } }, { { 17, 50, 22 }, { 6, 51, 23 } }, { { 19, 46, 16 }, { 6, 47, 17 } } },
				{ { { 2, 139, 111 }, { 7, 140, 112 } }, { { 17, 74, 46 } }, { { 7, 54, 24 }, { 16, 55, 25 } }, { { 34, 37, 13 } } },
				{ { { 4, 151, 121 }, { 5, 152, 122 } }, { { 4, 75, 47 }, { 14, 76, 48 } }, { { 11, 54, 24 }, { 14, 55, 25 } }, { { 16, 45, 15 }, { 14, 46, 16 } } },
				{ { { 6, 147, 117 }, { 4, 148, 118 } }, { { 6, 73, 45 }, { 14, 74, 46 } }, { { 11, 54, 24 }, { 16, 55, 25 } }, { { 30, 46, 16 }, { 2, 47, 17 } } },
				{ { { 8, 132, 106 }, { 4, 133, 107 } }, { { 8, 75, 47 }, { 13, 76, 48 } }, { { 7, 54, 24 }, { 22, 55, 25 } }, { { 22, 45, 15 }, { 13, 46, 16 } } },
				{ { { 10, 142, 114 }, { 2, 143, 115 } }, { { 19, 74, 46 }, { 4, 75, 47 } }, { { 28, 50, 22 }, { 6, 51, 23 } }, { { 33, 46, 16 }, { 4, 47, 17 } } },
				{ { { 8, 152, 122 }, { 4, 153, 123 } }, { { 22, 73, 45 }, { 3, 74, 46 } }, { { 8, 53, 23 }, { 26, 54, 24 } }, { { 12, 45, 15 }, { 28, 46, 16 } } },
				{ { { 3, 147, 117 }, { 10, 148, 118 } }, { { 3, 73, 45 }, { 23, 74, 46 } }, { { 4, 54, 24 }, { 31, 55, 25 } }, { { 11, 45, 15 }, { 31, 46, 16 } } },
				{ { { 7, 146, 116 }, { 7, 147, 117 } }, { { 21, 73, 45 }, { 7, 74, 46 } }, { { 1, 53, 23 }, { 37, 54, 24 } }, { { 19, 45, 15 }, { 26, 46, 16 } } },
				{ { { 5, 145, 115 }, { 10, 146, 116 } }, { { 19, 75, 47 }, { 10, 76, 48 } }, { { 15, 54, 24 }, { 25, 55, 25 } }, { { 23, 45, 15 }, { 25, 46, 16 } } },
				{ { { 13, 145, 115 }, { 3, 146, 116 } }, { { 2, 74, 46 }, { 29, 75, 47 } }, { { 42, 54, 24 }, { 1, 55, 25 } }, { { 23, 45, 15 }, { 28, 46, 16 } } },
				{ { { 17, 145, 115 } }, { { 10, 74, 46 }, { 23, 75, 47 } }, { { 10, 54, 24 }, { 35, 55, 25 } }, { { 19, 45, 15 }, { 35, 46, 16 } } },
				{ { { 17, 145, 115 }, { 1, 146, 116 } }, { { 14, 74, 46 }, { 21, 75, 47 } }, { { 29, 54, 24 }, { 19, 55, 25 } }, { { 11, 45, 15 }, { 46, 46, 16 } } },
				{ { { 13, 145, 115 }, { 6, 146, 116 } }, { { 14, 74, 46 }, { 23, 75, 47 } }, { { 44, 54, 24 }, { 7, 55, 25 } }, { { 59, 46, 16 }, { 1, 47, 17 } } },
				{ { { 12, 151, 121 }, { 7, 152, 122 } }, { { 12, 75, 47 }, { 26, 76, 48 } }, { { 39, 54, 24 }, { 14, 55, 25 } }, { { 22, 45, 15 }, { 41, 46, 16 } } },
				{ { { 6, 151, 121 }, { 14, 152, 122 } }, { { 6, 75, 47 }, { 34, 76, 48 } }, { { 46, 54, 24 }, { 10, 55, 25 } }, { { 2, 45, 15 }, { 64, 46, 16 } } },
				{ { { 17, 152, 122 }, { 4, 153, 123 } }, { { 29, 74, 46 }, { 14, 75, 47 } }, { { 49, 54, 24 }, { 10, 55, 25 } }, { { 24, 45, 15 }, { 46, 46,15 } } },
				{ { { 4, 152, 122 }, { 18, 153, 123 } }, { { 13, 74, 46 }, { 32, 75, 47 } }, { { 48, 54, 24 }, { 14, 55, 25 } }, { { 42, 45, 15 }, { 32, 46, 16 } } },
				{ { { 20, 147, 117 }, { 4, 148, 118 } }, { { 40, 75, 47 }, { 7, 76, 48 } }, { { 43, 54, 24 }, { 22, 55, 25 } }, { { 10, 45, 15 }, { 67, 46, 16 } } },
				{ { { 19, 148, 118 }, { 6, 149, 119 } }, { { 18, 75, 47 }, { 31, 76, 48 } }, { { 34, 54, 24 }, { 34, 55, 25 } }, { { 20, 45, 15 }, { 61, 46, 16 } } },
			} };
			vector<tuple<unsigned, unsigned, unsigned>> result;

			if (level == ErrorCorrectionLevel::ERROR_DETECTION_ONLY)
				level = ErrorCorrectionLevel::L;

			if (type == SymbolType::MICRO_QR)
				result = { microBlockLayouts[version - 1][static_cast<decltype(microBlockLayouts)::size_type>(level)] };
			else
				result = blockLayouts[version - 1][static_cast<decltype(microBlockLayouts)::size_type>(level)];

			return result;
		}

		//From table 9, page 38
		unsigned GetErrorCorrectionCodewordCount(SymbolType type, std::uint8_t version, ErrorCorrectionLevel level)
		{
			static const std::array<std::array<unsigned, 4>, 4> microCodewordCount = { {
				{ 2 }, //This will be used for M1 symbols, ErrorCorrectionLevel will be NONE
				{ 5, 6 },
				{ 6, 8 },
				{ 8, 10, 14 },
			} };
			static const std::array<std::array<unsigned, 4>, 40> codewordCount = { {
				{ 7, 10, 13, 17 },
				{ 10, 16, 22, 28 },
				{ 15, 26, 36, 44 },
				{ 20, 36, 52, 64 },
				{ 26, 48, 72, 88 },
				{ 36, 64, 96, 112 },
				{ 40, 72, 108, 130 },
				{ 48, 88, 132, 156 },
				{ 60, 110, 160, 192 },
				{ 72, 130, 192, 224 },
				{ 80, 150, 224, 264 },
				{ 96, 176, 260, 308 },
				{ 104, 198, 288, 352 },
				{ 120, 216, 320, 384 },
				{ 132, 240, 360, 432 },
				{ 144, 280, 408, 480 },
				{ 168, 308, 448, 532 },
				{ 180, 338, 504, 588 },
				{ 196, 364, 546, 650 },
				{ 224, 416, 600, 700 },
				{ 224, 442, 644, 750 },
				{ 252, 476, 690, 816 },
				{ 270, 504, 750, 900 },
				{ 300, 560, 810, 960 },
				{ 312, 588, 870, 1050 },
				{ 336, 644, 952, 1110 },
				{ 360, 700, 1020, 1200 },
				{ 390, 728, 1050, 1260 },
				{ 420, 784, 1140, 1350 },
				{ 450, 812, 1200, 1440 },
				{ 480, 868, 1290, 1530 },
				{ 510, 924, 1350, 1620 },
				{ 540, 980, 1440, 1710 },
				{ 570, 1036, 1530, 1800 },
				{ 570, 1064, 1590, 1890 },
				{ 600, 1120, 1680, 1980 },
				{ 630, 1204, 1770, 2100 },
				{ 660, 1260, 1860, 2220 },
				{ 720, 1316, 1950, 2310 },
				{ 750, 1372, 2040, 2430 }
			} };
			unsigned result;

			if (type == SymbolType::MICRO_QR)
			{
				if (version == 1)
					result = microCodewordCount[0][0];
				else
					result = microCodewordCount[version - 1][static_cast<decltype(microCodewordCount)::size_type>(level)];
			}
			else
				result = codewordCount[version - 1][static_cast<decltype(codewordCount)::size_type>(level)];

			return result;
		}

		//From table 2, page 23. version parameter is only used for Micro QR
		std::vector<bool> GetModeIndicator(SymbolType type, std::uint8_t version, Mode mode)
		{
			std::vector<bool> result;

			if (type == SymbolType::MICRO_QR)
			{
				result.resize(version - 1);

				if (version > 1)
				{
					if (mode == Mode::ALPHANUMERIC || mode == Mode::KANJI)
						result.back() = 1;

					if (version > 2 && (mode == Mode::BYTE || mode == Mode::KANJI))
						result.at(result.size() - 2) = 1;
				}
			}
			else
			{
				switch (mode)
				{
					case Mode::NUMERIC:
						result = { 0, 0, 0, 1 };
						break;

					case Mode::ALPHANUMERIC:
						result = { 0, 0, 1, 0 };
						break;

					case Mode::BYTE:
						result = { 0, 1, 0, 0 };
						break;

					case Mode::KANJI:
						result = { 1, 0, 0, 0 };
						break;
				}
			}

			return result;
		}

		//From table 3, page 23
		std::vector<bool> GetCharacterCountIndicator(SymbolType type, std::uint8_t version, Mode mode, size_t characterCount)
		{
			using TableType = std::array<std::array<unsigned, 4>, 4>;
			static const TableType microModeLengths = { { { 3 }, { 4, 3 }, { 5, 4, 4, 3 }, { 6, 5, 5, 4 } } };
			static const TableType modeLengths = { { { 10, 9, 8, 8 }, { 12, 11, 16, 10 }, { 14, 13, 16, 12 } } };
			unsigned length = 0;
			std::vector<bool> result;

			if (type == SymbolType::MICRO_QR)
				length = microModeLengths[version - 1][static_cast<size_t>(mode)];
			else
			{
				if (version <= 9)
					length = modeLengths[0][static_cast<size_t>(mode)];
				else
					if (version <= 26)
						length = modeLengths[1][static_cast<size_t>(mode)];
					else
						length = modeLengths[2][static_cast<size_t>(mode)];
			}

			result.resize(length);

			for (decltype(result)::size_type i = 0; i < result.size(); ++i)
				result.at(i) = characterCount & 1 << (length - 1) >> i ? true : false;

			return result;
		}

		//Returns bit sequence containing ECI mode indicator and ECI designator.
		std::vector<bool> GetECISequence(unsigned assignmentNumber)
		{
			std::vector<bool> result = { 0, 1, 1, 1 };

			if (assignmentNumber <= 127)
				result.resize(result.size() + 8);
			else
				if (assignmentNumber <= 16383)
					result.resize(result.size() + 16);
				else
					if (assignmentNumber <= 999999)
						result.resize(result.size() + 24);
					else
						throw std::invalid_argument("Invalid ECI assignment number, max value is 999999");

			for (decltype(result)::size_type i = 4, sz = (result.size() - 4) / 8 - 1; i < sz + 4; ++i)
				result[i] = 1;

			for (decltype(result)::size_type codewords = (result.size() - 4) / 8, bits = result.size() - 4 - codewords, i = 4 + codewords; i < result.size(); ++i)
				result[i] = assignmentNumber & 1 << (bits - 1) >> (i - 4 - codewords);

			return result;
		}

		//version parameter is only used for Micro QR
		std::vector<bool> GetTerminator(SymbolType type, std::uint8_t version)
		{
			return std::vector<bool>(type == SymbolType::MICRO_QR ? 3 + (version - 1) * 2 : 4, false);
		}

		std::bitset<15> GetFormatInformation(SymbolType type, std::uint8_t version, ErrorCorrectionLevel level, size_t maskId)
		{
			static const std::array<std::bitset<15>, 32> formats = {
				0b000000000000000,
				0b000010100110111,
				0b000101001101110,
				0b000111101011001,
				0b001000111101011,
				0b001010011011100,
				0b001101110000101,
				0b001111010110010,
				0b010001111010110,
				0b010011011100001,
				0b010100110111000,
				0b010110010001111,
				0b011001000111101,
				0b011011100001010,
				0b011100001010011,
				0b011110101100100,
				0b100001010011011,
				0b100011110101100,
				0b100100011110101,
				0b100110111000010,
				0b101001101110000,
				0b101011001000111,
				0b101100100011110,
				0b101110000101001,
				0b110000101001101,
				0b110010001111010,
				0b110101100100011,
				0b110111000010100,
				0b111000010100110,
				0b111010110010001,
				0b111101011001000,
				0b111111111111111
			};
			std::bitset<5> result;
			std::bitset<15> mask;

			if (type == SymbolType::QR)
			{
				mask = 21522;

				switch (level)
				{
					case ErrorCorrectionLevel::L:
						result[4] = 0;
						result[3] = 1;
						break;

					case ErrorCorrectionLevel::M:
						result[4] = result[3] = 0;
						break;

					case ErrorCorrectionLevel::Q:
						result[4] = result[3] = 1;
						break;

					case ErrorCorrectionLevel::H:
						result[4] = 1;
						result[3] = 0;
						break;
				}

				result[2] = maskId & 1 << 2;
				result[1] = maskId & 1 << 1;
				result[0] = maskId & 1;
			}
			else
			{
				std::uint8_t symbolNumber = 0;
				mask = 17477;

				switch (version)
				{

					case 1:
					case 2:
						symbolNumber = version - 1;
						break;

					case 3:
						symbolNumber = 3;
						break;

					case 4:
						symbolNumber = 5;
						break;
				}

				if (level != ErrorCorrectionLevel::ERROR_DETECTION_ONLY)
					symbolNumber += static_cast<std::uint8_t>(level);

				result[4] = symbolNumber & 1 << 2;
				result[3] = symbolNumber & 1 << 1;
				result[2] = symbolNumber & 1;
				result[1] = maskId & 1 << 1;
				result[0] = maskId & 1;
			}

			return formats[result.to_ulong()] ^ mask;
		}

		std::bitset<18> GetVersionInformation(std::uint8_t version)
		{
			static const std::array<std::bitset<18>, 34> versionInformation = {
				0b000111110010010100,
				0b001000010110111100,
				0b001001101010011001,
				0b001010010011010011,
				0b001011101111110110,
				0b001100011101100010,
				0b001101100001000111,
				0b001110011000001101,
				0b001111100100101000,
				0b010000101101111000,
				0b010001010001011101,
				0b010010101000010111,
				0b010011010100110010,
				0b010100100110100110,
				0b010101011010000011,
				0b010110100011001001,
				0b010111011111101100,
				0b011000111011000100,
				0b011001000111100001,
				0b011010111110101011,
				0b011011000010001110,
				0b011100110000011010,
				0b011101001100111111,
				0b011110110101110101,
				0b011111001001010000,
				0b100000100111010101,
				0b100001011011110000,
				0b100010100010111010,
				0b100011011110011111,
				0b100100101100001011,
				0b100101010000101110,
				0b100110101001100100,
				0b100111010101000001,
				0b101000110001101001
			};

			return versionInformation[version - 7];
		}

		const std::vector<Symbol::size_type>& GetAlignmentPatternCenters(std::uint8_t version)
		{
			static const std::array<std::vector<Symbol::size_type>, 40> centers = { {
				{},
				{ 6, 18 },
				{ 6, 22 },
				{ 6, 26 },
				{ 6, 30 },
				{ 6, 34 },
				{ 6, 22, 38 },
				{ 6, 24, 42 },
				{ 6, 26, 46 },
				{ 6, 28, 50 },
				{ 6, 30, 54 },
				{ 6, 32, 58 },
				{ 6, 34, 62 },
				{ 6, 26, 46, 66 },
				{ 6, 26, 48, 70 },
				{ 6, 26, 50, 74 },
				{ 6, 30, 54, 78 },
				{ 6, 30, 56, 82 },
				{ 6, 30, 58, 86 },
				{ 6, 34, 62, 90 },
				{ 6, 28, 50, 72, 94 },
				{ 6, 26, 50, 74, 98 },
				{ 6, 30, 54, 78, 102 },
				{ 6, 28, 54, 80, 106 },
				{ 6, 32, 58, 84, 110 },
				{ 6, 30, 58, 86, 114 },
				{ 6, 34, 62, 90, 118 },
				{ 6, 26, 50, 74, 98, 122 },
				{ 6, 30, 54, 78, 102, 126 },
				{ 6, 26, 52, 78, 104, 130 },
				{ 6, 30, 56, 82, 108, 134 },
				{ 6, 34, 60, 86, 112, 138 },
				{ 6, 30, 58, 86, 114, 142 },
				{ 6, 34, 62, 90, 118, 146 },
				{ 6, 30, 54, 78, 102, 126, 150 },
				{ 6, 24, 50, 76, 102, 128, 154 },
				{ 6, 28, 54, 80, 106, 132, 158 },
				{ 6, 32, 58, 84, 110, 136, 162 },
				{ 6, 26, 54, 82, 110, 138, 166 },
				{ 6, 30, 58, 86, 114, 142, 170 }
			} };

			return centers.at(version - 1);
		}

		//Returns a matrix with all the bits that correspond to function patterns or version/format information set to true
		std::vector<std::vector<bool>> GetDataRegionMask(SymbolType type, std::uint8_t version)
		{
			using std::vector;
			using std::pair;
			using SizeType = Symbol::size_type;
			struct Rectangle
			{
				struct Point
				{
					Symbol::size_type mX;
					Symbol::size_type mY;
				};
				Point mFrom;
				Point mTo;
			};
			auto symbolSize = GetSymbolSize(type, version);
			vector<vector<bool>> result(symbolSize, vector<bool>(symbolSize));
			auto &centers = GetAlignmentPatternCenters(version);
			vector<Rectangle> functionPatterns;
			SizeType timingRowColumn = type == SymbolType::MICRO_QR ? 0 : 6;

			//Top left finder pattern
			functionPatterns.push_back({ { 0, 0 }, { 7, 7 } });

			//Timing patterns
			functionPatterns.push_back({ { timingRowColumn, 0 }, { timingRowColumn, symbolSize - 1 } });
			functionPatterns.push_back({ { 0, timingRowColumn }, { symbolSize - 1, timingRowColumn } });

			//Top left format information
			functionPatterns.push_back({ { 8, 0 }, { 8, 8 } });
			functionPatterns.push_back({ { 0, 8 }, { 8, 8 } });

			if (type != SymbolType::MICRO_QR)
			{
				//Top right and bottom left finder patterns
				functionPatterns.push_back({ { 0, symbolSize - 8 }, { 7, symbolSize - 1 } });
				functionPatterns.push_back({ { symbolSize - 8, 0 }, { symbolSize - 1, 7 } });

				//Bottom left and top right format infomation
				functionPatterns.push_back({ { 8, symbolSize - 8 }, { 8, symbolSize - 1 } });
				functionPatterns.push_back({ { symbolSize - 8, 8 }, { symbolSize - 1, 8 } });

				//Alignment patterns
				for (unsigned i = 0; i < centers.size(); ++i)
					for (unsigned j = i; j < centers.size(); ++j)
					{
						auto centerX = centers[j], centerY = centers[i];

						if (!(centerY == 6 && centerX == symbolSize - 7 || centerY == symbolSize - 7 && centerX == 6 || centerY == 6 && centerX == 6))
						{
							functionPatterns.push_back({ { centerX - 2, centerY - 2 }, { centerX + 2, centerY + 2 } });
							functionPatterns.push_back({ { centerY - 2, centerX - 2 }, { centerY + 2, centerX + 2 } });
						}
					}

				if (version >= 7)
				{
					functionPatterns.push_back({ { 0, symbolSize - 11 }, { 5, symbolSize - 9 } });
					functionPatterns.push_back({ { symbolSize - 11, 0 }, { symbolSize - 9, 5 } });
				}
			}

			for (const auto &rectangle : functionPatterns)
				for (auto x = rectangle.mFrom.mX; x <= rectangle.mTo.mX; ++x)
					for (auto y = rectangle.mFrom.mY; y <= rectangle.mTo.mY; ++y)
						result[y][x] = true;

			return result;
		}

		//From table 10, page 50.
		bool GetMaskBit(SymbolType type, std::uint8_t maskId, Symbol::size_type i, Symbol::size_type j)
		{
			bool result = false;

			if (type == SymbolType::MICRO_QR)
				switch (maskId)
				{
					case 0b00:
						maskId = 0b001;
						break;

					case 0b01:
						maskId = 0b100;
						break;

					case 0b10:
						maskId = 0b110;
						break;

					case 0b11:
						maskId = 0b111;
						break;
				}

			switch (maskId)
			{
				case 0b000:
					result = !((i + j) % 2);
					break;

				case 0b001:
					result = !(i % 2);
					break;

				case 0b010:
					result = !(j % 3);
					break;

				case 0b011:
					result = !((i + j) % 3);
					break;

				case 0b100:
					result = !((i / 2 + j / 3) % 2);
					break;

				case 0b101:
					result = !((i * j) % 2 + (i * j) % 3);
					break;

				case 0b110:
					result = !(((i * j) % 2 + (i * j) % 3) % 2);
					break;

				case 0b111:
					result = !(((i + j) % 2 + (i * j) % 3) % 2);
					break;
			}

			return result;
		}

		unsigned GetFeature1Points(unsigned adjacentCount)
		{
			unsigned result = 0;

			if (adjacentCount >= 5)
				result += 3 + adjacentCount - 5;

			return result;
		}

		unsigned GetSymbolRating(const Symbol &symbol, SymbolType type)
		{
			if (type == SymbolType::QR)
			{
				static const std::vector<bool> feature3Pattern = { 1, 0, 1, 1, 1, 0, 1 };
				unsigned feature1Score = 0, feature2Score = 0, feature3Score = 0, feature4Score = 0;
				size_t totalModules = symbol.size() * symbol.size(), darkModules = 0;
				int percentage = 0;

				for (Symbol::size_type i = 0; i < symbol.size(); ++i)
				{
					unsigned consecutiveRowCounter = 0, consecutiveColumnCounter = 0, feature3Row = 0, feature3Column = 0;
					bool consecutiveRowValue = false, consecutiveColumnValue = false;

					for (Symbol::value_type::size_type j = 0; j < symbol[i].size(); ++j)
					{
						//Feature 1
						if (symbol[i][j] == consecutiveRowValue)
							++consecutiveRowCounter;
						else
						{
							feature1Score += GetFeature1Points(consecutiveRowCounter);
							consecutiveRowValue = symbol[i][j], consecutiveRowCounter = 1;
						}

						if (symbol[j][i] == consecutiveColumnValue)
							++consecutiveColumnCounter;
						else
						{
							feature1Score += GetFeature1Points(consecutiveColumnCounter);
							consecutiveColumnValue = symbol[j][i], consecutiveColumnCounter = 1;
						}

						//Feature 2
						if (i < symbol.size() - 1 &&
							j < symbol[i].size() - 1 &&
							symbol[i][j] == symbol[i][j + 1] &&
							symbol[i][j] == symbol[i + 1][j] &&
							symbol[i][j] == symbol[i + 1][j + 1])
							feature2Score += 3;

						//Feature 3
						if (symbol[i][j] == feature3Pattern[feature3Row])
							++feature3Row;
						else
						{
							feature3Row = 0;
							if (symbol[i][j] == feature3Pattern[feature3Row])
								++feature3Row;
						}

						if (symbol[j][i] == feature3Pattern[feature3Column])
							++feature3Column;
						else
						{
							feature3Column = 0;
							if (symbol[j][i] == feature3Pattern[feature3Column])
								++feature3Column;
						}

						if (feature3Column == feature3Pattern.size())
						{
							if (i + 4 < symbol.size() && !symbol[i + 1][j] && !symbol[i + 2][j] && !symbol[i + 3][j] && !symbol[i + 4][j] ||
								i >= 10 && !symbol[i - 7][j] && !symbol[i - 8][j] && !symbol[i - 9][j] && !symbol[i - 10][j])
								feature3Score += 40;

							feature3Column = 0;
						}

						if (feature3Row == feature3Pattern.size())
						{
							if (j + 4 < symbol.size() && !symbol[i][j + 1] && !symbol[i][j + 2] && !symbol[i][j + 3] && !symbol[i][j + 4] ||
								j >= 10 && !symbol[i][j - 7] && !symbol[i][j - 8] && !symbol[i][j - 9] && !symbol[i][j - 10])
								feature3Score += 40;

							feature3Row = 0;
						}

						//Feature 4
						if (symbol[j][i])
							++darkModules;
					}

					//Feature 1
					//In case the row/column ended without breaking the chain of consecutive modules
					feature1Score += GetFeature1Points(consecutiveRowCounter);
					feature1Score += GetFeature1Points(consecutiveColumnCounter);
				}

				percentage = static_cast<int>(static_cast<double>(darkModules) / static_cast<double>(totalModules) * 100.);
				feature4Score = std::abs(percentage - 50) / 5 * 10;

				return feature1Score + feature2Score + feature3Score + feature4Score;
			}
			else
			{
				unsigned darkRow = 0, darkColumn = 0;

				//Start at 1 to avoid timing pattern
				for (Symbol::size_type i = 1, sz = symbol.size(); i < sz; ++i)
				{
					if (symbol[i][sz - 1])
						++darkColumn;

					if (symbol[sz - 1][i])
						++darkRow;
				}

				return darkColumn <= darkRow ? darkColumn * 16 + darkRow : darkRow * 16 + darkColumn;
			}
		}

		std::uint16_t ToInteger(const std::vector<std::string::value_type> &characters)
		{
			std::uint16_t result = 0, multiplier = 1;

			for (std::vector<std::string::value_type>::const_reverse_iterator it = characters.crbegin(); it != characters.crend(); ++it, multiplier *= 10)
			{
				if (*it < 0x30 || *it > 0x39)
				{
					using namespace std::string_literals;
					std::ostringstream stream;

					stream << std::hex << std::uppercase << (static_cast<int>(*it) & 0xFF);
					throw std::invalid_argument("Character 0x" + stream.str() + " can't be encoded in numeric mode");
				}

				result += (*it - 0x30) * multiplier;
			}

			return result;
		}

		std::uint8_t GetAlphanumericCode(std::string_view::value_type character)
		{
			std::uint8_t result = 0;

			if (character >= 0x30 && character <= 0x39)
				result = character - 0x30;
			else if (character >= 0x41 && character <= 0x5A)
				result = character - 0x41 + 10;
			else if (character >= 0x61 && character <= 0x7A)
				result = character - 0x61 + 10;
			else
			{
				static const std::array<decltype(character), 9> specialCharacters = { 0x20/*space*/, 0x24/*$*/, 0x25/*%*/, 0x2A/***/, 0x2B/*+*/, 0x2D/*-*/, 0x2E/*.*/, 0x2F/*/*/, 0x3A/*:*/ };
				auto it = std::find(specialCharacters.cbegin(), specialCharacters.cend(), character);

				if (it != specialCharacters.cend())
					result = 36 + (it - specialCharacters.cbegin());
				else
				{
					using namespace std::string_literals;
					std::ostringstream stream;

					stream << std::hex << std::uppercase << (static_cast<int>(character) & 0xFF);
					throw std::invalid_argument("Character 0x" + stream.str() + " can't be encoded in alphanumeric mode");
				}
			}

			return result;
		}

		//From figure H.1, page 93
		bool IsKanji(std::uint16_t character)
		{
			std::uint8_t leadingByte = character >> 8, trailerByte = character & 0xFF;

			return (leadingByte >= 0x81 && leadingByte <= 0x9F || leadingByte >= 0xE0 && leadingByte <= 0xEA)
				&& (trailerByte >= 0x40 && trailerByte <= 0x7E || trailerByte >= 0x80 && trailerByte <= 0xFC)
				|| leadingByte == 0xEB && (trailerByte >= 0x40 && trailerByte <= 0x7E || trailerByte >= 0x80 && trailerByte <= 0xBF);
		}

		//Returns the most compact Mode in which the character can be encoded in.
		Mode GetMinimalMode(std::uint8_t leadingByte, std::optional<std::uint8_t> trailerByte = std::optional<std::uint8_t>())
		{
			static const std::unordered_set<std::uint8_t> characters = { ' ', '$', '%', '*', '+', '-', '.', '/', ':' };
			Mode result;

			if (leadingByte >= 0x30 && leadingByte <= 0x39)
				result = Mode::NUMERIC;
			else
				if (leadingByte >= 0x41 && leadingByte <= 0x5A || leadingByte >= 0x61 && leadingByte <= 0x7A || characters.count(leadingByte))
					result = Mode::ALPHANUMERIC;
				else
					if (trailerByte.has_value() && IsKanji(leadingByte << 8 | trailerByte.value()))
						result = Mode::KANJI;
					else
						result = Mode::BYTE;

			return result;
		}

		const std::regex& GetECIRegex()
		{
			static const std::regex eciFormat("(\\\\)?\\\\([^\\\\]{0,6})");

			return eciFormat;
		}

		void DrawFinderPattern(Symbol &symbol, Symbol::size_type startingRow, Symbol::size_type startingColumn)
		{
			for (decltype(startingRow) i = startingRow; i < startingRow + 7; ++i)
				for (decltype(startingColumn) j = startingColumn; j < startingColumn + 7; ++j)
				{
					switch (i - startingRow)
					{
						case 0:
						case 6:
							symbol[i][j] = true;
							break;

						case 1:
						case 5:
							if (j - startingColumn == 0 || j - startingColumn == 6)
								symbol[i][j] = true;
							break;

						case 2:
						case 3:
						case 4:
							if (j - startingColumn == 0 || j - startingColumn == 2 || j - startingColumn == 3 || j - startingColumn == 4 || j - startingColumn == 6)
								symbol[i][j] = true;
							break;
					}
				}
		}

		void DrawTimingPatterns(Symbol &symbol, SymbolType type, std::uint8_t version)
		{
			auto size = GetSymbolSize(type, version);
			unsigned rowColumn = type == SymbolType::MICRO_QR ? 0 : 6;

			for (unsigned i = 8; i < (type == SymbolType::MICRO_QR ? size : size - 8); ++i)
				symbol[i][rowColumn] = symbol[rowColumn][i] = !(i % 2);
		}

		void DrawAlignmentPatterns(Symbol &symbol, std::uint8_t version)
		{
			const std::vector<Symbol::size_type> &centers = GetAlignmentPatternCenters(version);
			auto symbolSize = GetSymbolSize(QR::SymbolType::QR, version);

			for (decltype(centers.size()) i = 0; i < centers.size(); ++i)
				for (auto j = i; j < centers.size(); ++j)
				{
					auto centerX = centers[j], centerY = centers[i];

					if (!(centerY == 6 && centerX == symbolSize - 7 || centerY == symbolSize - 7 && centerX == 6 || centerY == 6 && centerX == 6))
						for (auto startX = centerX - 2, x = startX; x < centerX + 3; ++x)
							for (auto startY = centerY - 2, y = startY; y < centerY + 3; ++y)
							{
								switch (y - startY)
								{
									case 0:
									case 4:
										symbol[y][x] = symbol[x][y] = true;
										break;

									case 2:
										if (x - startX == 0 || x - startX == 2 || x - startX == 4)
										{
											symbol[y][x] = symbol[x][y] = true;
											break;
										}
										[[fallthrough]];

									case 1:
									case 3:
										if (x - startX == 0 || x - startX == 4)
											symbol[y][x] = symbol[x][y] = true;
										break;
								}
							}
				}
		}

		void DrawFormatInformation(Symbol &symbol, SymbolType type, std::uint8_t version, ErrorCorrectionLevel level, size_t maskId)
		{
			auto formatInfo = GetFormatInformation(type, version, level, maskId);
			unsigned timingPatternRowColumn = type == SymbolType::MICRO_QR ? 0 : 6, symbolSize = GetSymbolSize(type, version), bitIndex = 0;

			for (unsigned i = 0; i < 8; ++i)
				if (i != timingPatternRowColumn)
					symbol[i][8] = formatInfo[bitIndex++];

			for (unsigned i = 9; i--;)
				if (i != timingPatternRowColumn)
					symbol[8][i] = formatInfo[bitIndex++];

			if (type == SymbolType::QR)
			{
				symbol[symbolSize - 8][8] = 1; //This module is always dark

				for (bitIndex = 0; bitIndex < formatInfo.size(); ++bitIndex)
					if (bitIndex <= 7)
						symbol[8][symbolSize - 1 - bitIndex] = formatInfo[bitIndex];
					else
						symbol[symbolSize - formatInfo.size() + bitIndex][8] = formatInfo[bitIndex];
			}
		}

		void DrawVersionInformation(Symbol &symbol, SymbolType type, std::uint8_t version)
		{
			auto versionInfo = GetVersionInformation(version);
			auto symbolSize = GetSymbolSize(type, version);

			for (unsigned i = symbolSize - 11, j = 0, bitIndex = 0; bitIndex < versionInfo.size(); ++i, ++bitIndex)
			{
				if (!(bitIndex % 3) && bitIndex)
					i -= 3, ++j;

				symbol[i][j] = symbol[j][i] = versionInfo[bitIndex];
			}
		}

		void ValidateArguments(SymbolType type, std::uint8_t version, ErrorCorrectionLevel level)
		{
			if (!version)
				throw std::invalid_argument("Minimum version for QR and Micro QR symbols is 1");

			if (type == SymbolType::MICRO_QR)
			{
				if (version > 4)
					throw std::invalid_argument("Max version for Micro QR symbols is M4");

				if (version == 1 && level != ErrorCorrectionLevel::ERROR_DETECTION_ONLY)
					throw std::invalid_argument("M1 symbols don't support error correction");

				if (version != 1 && level == ErrorCorrectionLevel::ERROR_DETECTION_ONLY)
					throw std::invalid_argument("ERROR_DETECTION_ONLY is only for M1 symbols");

				if (level == ErrorCorrectionLevel::Q && version != 4)
					throw std::invalid_argument("Level Q error correction in Micro QR symbols is only supported in version M4");

				if (level == ErrorCorrectionLevel::H)
					throw std::invalid_argument("Level H error correction is not supported in Micro QR symbols");
			}
			else if (type == SymbolType::QR)
			{
				if (version > 40)
					throw std::invalid_argument("Max version for QR symbols is 40");

				if (level == ErrorCorrectionLevel::ERROR_DETECTION_ONLY)
					throw std::invalid_argument("ERROR_DETECTION_ONLY is only for M1 symbols");
			}
		}

		#ifndef TESTS
	}
	#endif
}

struct QR::Encoder::Impl
{
	std::vector<bool> mBitStream;
	unsigned mVersion;
	SymbolType mType;
	ErrorCorrectionLevel mLevel;
};

QR::Encoder::Encoder(SymbolType type, unsigned version, ErrorCorrectionLevel level)
	:mImpl(new Impl{ {}, version, type, level })
{
	ValidateArguments(type, version, level);
}

QR::Encoder::Encoder(const Encoder &other)
	:mImpl(new Impl{ *other.mImpl })
{}

QR::Encoder::Encoder(Encoder&&) noexcept = default;

QR::Encoder &QR::Encoder::operator=(const Encoder &other)
{
	*mImpl = *other.mImpl;

	return *this;
}

QR::Encoder &QR::Encoder::operator=(Encoder&&) noexcept = default;

QR::Encoder::~Encoder() = default;

void QR::Encoder::addCharacters(std::string_view message, Mode mode)
{
	using std::get;
	const std::regex &eciFormat = GetECIRegex();
	std::vector<bool> modeIndicator = GetModeIndicator(mImpl->mType, mImpl->mVersion, mode), dataBits;
	std::vector<std::tuple<size_t, size_t, std::optional<unsigned>>> ranges; //<index, byte count, ECI>
	unsigned dataModuleCount = GetDataModuleCount(mImpl->mType, mImpl->mVersion) - GetRemainderBitCount(mImpl->mType, mImpl->mVersion) - GetErrorCorrectionCodewordCount(mImpl->mType, mImpl->mVersion, mImpl->mLevel) * 8;
	unsigned doubleSlashCount = 0;

	for (std::regex_iterator<decltype(message)::const_iterator> it(message.cbegin(), message.cend(), eciFormat), end; it != end; ++it)
	{
		if (auto &results = *it; !results[1].matched)
		{
			if (results[2].length() != 6)
				throw std::invalid_argument("Invalid ECI sequence");
			try
			{
				if (!ranges.empty())
					get<1>(ranges.back()) = results.position() - get<0>(ranges.back());
				else
					if (results.position())
						ranges.emplace_back(0, results.position(), std::optional<unsigned>());

				ranges.emplace_back(results.position() + results.length(), message.size() - (results.position() + results.length()), std::stoul(results[2].str()));
			}
			catch (const std::invalid_argument&)
			{
				throw std::invalid_argument("Invalid ECI sequence");
			}
		}
		else
			++doubleSlashCount;
	}

	if (ranges.empty())
		ranges.emplace_back(0, message.size(), std::optional<unsigned>());
	else
		if (mImpl->mType == QR::SymbolType::MICRO_QR)
			throw std::invalid_argument("ECI is not supported in Micro QR symbols");

	for (const auto &range : ranges)
	{
		auto [index, byteCount, eci] = range;
		auto characterCount = GetCharacterCountIndicator(mImpl->mType, mImpl->mVersion, mode, mode == Mode::KANJI ? byteCount / 2 : byteCount - doubleSlashCount);

		if (mode == Mode::KANJI && byteCount % 2)
			throw std::invalid_argument("Invalid Kanji sequence");

		if (eci)
		{
			auto eciBits = GetECISequence(eci.value());

			dataBits.insert(dataBits.end(), eciBits.begin(), eciBits.end());
		}

		dataBits.insert(dataBits.end(), modeIndicator.begin(), modeIndicator.end());
		dataBits.insert(dataBits.end(), characterCount.begin(), characterCount.end());

		switch (mode)
		{
			case Mode::NUMERIC:
			{
				std::vector<decltype(message)::value_type> digits;

				for (decltype(message)::size_type i = index; i < index + byteCount; ++i)
				{
					digits.push_back(message[i]);

					if (digits.size() == 3 || i == (index + byteCount) - 1 && !digits.empty())
					{
						unsigned encodedDigits = ToInteger(digits);

						for (size_t bit = 0, bitCount = digits.size() * 3 + 1; bit < bitCount; ++bit)
							dataBits.push_back(encodedDigits & 1 << (bitCount - 1) >> bit);

						digits.clear();
					}
				}

				break;
			}

			case Mode::ALPHANUMERIC:
			{
				std::vector<decltype(message)::value_type> characters;

				if (mImpl->mType == SymbolType::MICRO_QR && mImpl->mVersion < 2)
					throw std::invalid_argument("Alphanumeric mode is not supported in M1 symbols");

				for (decltype(message)::size_type i = index; i < index + byteCount; ++i)
				{
					characters.push_back(message[i]);

					if (characters.size() == 2 || i == (index + byteCount) - 1 && !characters.empty())
					{
						unsigned encodedCharacters = characters.size() == 2 ? GetAlphanumericCode(characters[0]) * 45 + GetAlphanumericCode(characters[1]) : GetAlphanumericCode(characters[0]);

						for (size_t bit = 0, bitCount = characters.size() == 2 ? 11 : 6; bit < bitCount; ++bit)
							dataBits.push_back(encodedCharacters & 1 << (bitCount - 1) >> bit);

						characters.clear();
					}
				}

				break;
			}

			case Mode::BYTE:
			{
				if (mImpl->mType == SymbolType::MICRO_QR && mImpl->mVersion < 3)
					throw std::invalid_argument("Byte mode is not supported in M1 and M2 symbols");

				for (size_t i = index; i < index + byteCount; ++i)
				{
					for (size_t bit = 0; bit < 8; ++bit)
						dataBits.push_back(*(message.data() + i) & 1 << 7 >> bit);

					if (message[i] == 0x5C)
						++i;
				}

				break;
			}

			case Mode::KANJI:
			{
				if (mImpl->mType == SymbolType::MICRO_QR && mImpl->mVersion < 3)
					throw std::invalid_argument("Kanji mode is not supported in M1 and M2 symbols");

				for (decltype(message)::size_type i = index; i < index + byteCount; i += 2)
				{
					std::uint16_t kanjiCharacter = message[i] << 8 | message[i + 1] & 0xFF;

					if (!IsKanji(kanjiCharacter))
					{
						using namespace std::string_literals;
						std::ostringstream stream;

						stream << std::hex << std::uppercase << (static_cast<int>(kanjiCharacter) & 0xFFFF);
						throw std::invalid_argument("Character 0x" + stream.str() + " can't be encoded in Kanji mode");
					}

					if (kanjiCharacter >= 0x8140 && kanjiCharacter <= 0x9FFC)
						kanjiCharacter -= 0x8140;
					else
						if (kanjiCharacter >= 0xE040 && kanjiCharacter <= 0xEBBF)
							kanjiCharacter -= 0xC140;

					kanjiCharacter = (kanjiCharacter >> 8) * 0xC0 + (kanjiCharacter & 0xFF);

					for (size_t bit = 0; bit < 13; ++bit)
						dataBits.push_back(kanjiCharacter & 1 << 12 >> bit);
				}

				break;
			}
		}
	}

	if (mImpl->mBitStream.size() + dataBits.size() <= dataModuleCount)
		mImpl->mBitStream.insert(mImpl->mBitStream.end(), dataBits.begin(), dataBits.end());
	else
		throw std::length_error("Data bit stream would exceed the symbol's capacity");
}

void QR::Encoder::clear()
{
	mImpl->mBitStream.clear();
}

std::vector<std::vector<bool>> QR::Encoder::generateMatrix() const
{
	using std::vector; using std::tuple; using std::bitset; using std::get;
	vector<vector<bool>> result(GetSymbolSize(mImpl->mType, mImpl->mVersion), vector<bool>(GetSymbolSize(mImpl->mType, mImpl->mVersion))), mask = GetDataRegionMask(mImpl->mType, mImpl->mVersion);
	vector<Symbol> maskedSymbols;
	vector<unsigned> maskedSymbolScores;
	vector<bool> dataBitStream = mImpl->mBitStream;
	vector<vector<bitset<8>>> dataBlocks, errorCorrectionBlocks;
	decltype(dataBitStream)::size_type bitIndex = 0;
	int currentRow = GetSymbolSize(mImpl->mType, mImpl->mVersion) - 1, currentColumn = currentRow, delta = -1;
	unsigned quietZoneWidth = mImpl->mType == SymbolType::MICRO_QR ? 2 : 4;
	unsigned dataModuleCount = GetDataModuleCount(mImpl->mType, mImpl->mVersion) - GetRemainderBitCount(mImpl->mType, mImpl->mVersion) - GetErrorCorrectionCodewordCount(mImpl->mType, mImpl->mVersion, mImpl->mLevel) * 8;
	size_t maskId;

	DrawFinderPattern(result, 0, 0);
	if (mImpl->mType != SymbolType::MICRO_QR)
	{
		DrawFinderPattern(result, 0, GetSymbolSize(mImpl->mType, mImpl->mVersion) - 7);
		DrawFinderPattern(result, GetSymbolSize(mImpl->mType, mImpl->mVersion) - 7, 0);
		DrawAlignmentPatterns(result, mImpl->mVersion);
	}
	DrawTimingPatterns(result, mImpl->mType, mImpl->mVersion);

	//Add terminator and pad codewords
	if (dataBitStream.size() <= dataModuleCount)
	{
		auto terminator = GetTerminator(mImpl->mType, mImpl->mVersion);

		dataBitStream.insert(dataBitStream.end(), terminator.begin(), terminator.begin() + std::min(dataModuleCount - dataBitStream.size(), terminator.size()));

		if (dataBitStream.size() < dataModuleCount && dataBitStream.size() % 8)
		{
			dataBitStream.resize(dataBitStream.size() - dataBitStream.size() % 8 + 8);

			//If I got past the limit after resizing to an 8 bit boundary, then it must be because of the 4 bit codeword in M1 or M3 symbols
			if (dataBitStream.size() > dataModuleCount)
				dataBitStream.resize(dataModuleCount);
		}

		if (dataBitStream.size() < dataModuleCount)
		{
			vector<vector<bool>> padCodewords;
			decltype(padCodewords)::size_type counter = 0;

			if (mImpl->mType == SymbolType::MICRO_QR && (mImpl->mVersion == 1 || mImpl->mVersion == 3))
				padCodewords = { { 0, 0, 0, 0 } }; //Pad codeword for M1 and M3
			else
				padCodewords = { { 1, 1, 1, 0, 1, 1, 0, 0 }, { 0, 0, 0, 1, 0, 0, 0, 1 } };

			while (dataBitStream.size() < dataModuleCount)
				dataBitStream.insert(dataBitStream.end(), padCodewords[counter % padCodewords.size()].begin(), padCodewords[counter % padCodewords.size()].end()), ++counter;
		}
	}
	else
		throw std::length_error("Message exceeds symbol capacity");

	//Split bit stream into data blocks and generate the corresponding error correction blocks
	for (const auto &blockLayout : GetBlockLayout(mImpl->mType, mImpl->mVersion, mImpl->mLevel))
	{
		auto blockCounter = std::get<0>(blockLayout);

		while (blockCounter--)
		{
			decltype(dataBlocks)::value_type block(get<2>(blockLayout)), errorBlock;
			auto &generatorPolynomial = GetPolynomialCoefficientExponents(get<1>(blockLayout) - get<2>(blockLayout));
			unsigned n = get<2>(blockLayout);

			for (auto &codeword : block)
			{
				decltype(dataBitStream)::size_type lastBit = 0;

				if (mImpl->mType == SymbolType::MICRO_QR && (mImpl->mVersion == 1 || mImpl->mVersion == 3) && &codeword == &block.back())
					lastBit = 4;

				for (decltype(dataBitStream)::size_type i = 8; i-- > lastBit;)
					codeword[i] = dataBitStream[bitIndex++];
			}

			errorBlock = block;
			errorBlock.resize(get<1>(blockLayout));

			while (n--)
			{
				auto current = errorBlock.front().to_ulong();

				errorBlock.erase(errorBlock.begin());

				if (current)
				{
					current = GetAlphaExponent(current);

					for (unsigned i = 0; i < generatorPolynomial.size(); ++i)
					{
						auto exponentSum = current + generatorPolynomial[i];

						if (exponentSum > 255)
							exponentSum %= 255;

						errorBlock[i] = errorBlock[i].to_ulong() ^ GetAlphaValue(exponentSum);
					}
				}
			}

			dataBlocks.push_back(block);
			errorCorrectionBlocks.push_back(errorBlock);
		}
	}

	//Place bits in symbol
	for (const decltype(dataBlocks) &blocks : { std::ref(dataBlocks), std::ref(errorCorrectionBlocks) })
	{
		auto maxLength = std::max_element(blocks.begin(), blocks.end(), [](const decltype(dataBlocks)::value_type &lhs, const decltype(dataBlocks)::value_type &rhs) { return lhs.size() < rhs.size(); })->size();

		for (decltype(maxLength) codewordIndex = 0; codewordIndex < maxLength; ++codewordIndex)
		{
			for (const auto &block : blocks)
			{
				if (codewordIndex < block.size())
				{
					auto lastBit = 0;

					if (mImpl->mType == SymbolType::MICRO_QR && (mImpl->mVersion == 1 || mImpl->mVersion == 3) && &block[codewordIndex] == &block.back() && &blocks == &dataBlocks)
						lastBit = 4;

					for (auto bitIndex = 8; bitIndex > lastBit;)
					{
						if (!mask[currentRow][currentColumn])
							result[currentRow][currentColumn] = block[codewordIndex][--bitIndex];

						if (mImpl->mType == SymbolType::MICRO_QR && currentColumn % 2 ||
							mImpl->mType == SymbolType::QR && currentColumn > 6 && currentColumn % 2 ||
							mImpl->mType == SymbolType::QR && currentColumn < 6 && !(currentColumn % 2))
						{
							if (!currentRow && delta != 1)
								delta = 1, currentColumn -= 2;
							else
								if (currentRow == GetSymbolSize(mImpl->mType, mImpl->mVersion) - 1 && delta != -1)
									delta = -1, currentColumn -= 2;
								else
									currentRow += delta;

							++currentColumn;

							if (currentColumn == 6 && mImpl->mType != SymbolType::MICRO_QR)
								currentColumn = 5;
						}
						else
							--currentColumn;
					}
				}
			}
		}
	}

	for (unsigned maskId = 0, sz = mImpl->mType == SymbolType::MICRO_QR ? 4 : 8; maskId < sz; ++maskId)
	{
		maskedSymbols.push_back(result);

		for (Symbol::size_type i = 0, sz = maskedSymbols.back().size(); i < sz; ++i)
			for (Symbol::value_type::size_type j = 0, sz = maskedSymbols.back()[i].size(); j < sz; ++j)
				if (!mask[i][j])
					maskedSymbols.back()[i][j] = maskedSymbols.back()[i][j] ^ GetMaskBit(mImpl->mType, maskId, i, j);

		maskedSymbolScores.push_back(GetSymbolRating(maskedSymbols.back(), mImpl->mType));
	}

	if (mImpl->mType == SymbolType::QR)
		maskId = std::min_element(maskedSymbolScores.begin(), maskedSymbolScores.end()) - maskedSymbolScores.begin();
	else
		maskId = std::max_element(maskedSymbolScores.begin(), maskedSymbolScores.end()) - maskedSymbolScores.begin();

	result = maskedSymbols[maskId];

	DrawFormatInformation(result, mImpl->mType, mImpl->mVersion, mImpl->mLevel, maskId);
	if (mImpl->mVersion >= 7)
		DrawVersionInformation(result, mImpl->mType, mImpl->mVersion);

	//Add quiet zone
	for (auto &row : result)
	{
		row.insert(row.begin(), quietZoneWidth, false);
		row.insert(row.end(), quietZoneWidth, false);
	}

	for (unsigned i = 0; i < quietZoneWidth; ++i)
	{
		result.insert(result.begin(), std::vector<bool>(GetSymbolSize(mImpl->mType, mImpl->mVersion) + quietZoneWidth * 2));
		result.insert(result.end(), std::vector<bool>(GetSymbolSize(mImpl->mType, mImpl->mVersion) + quietZoneWidth * 2));
	}

	return result;
}