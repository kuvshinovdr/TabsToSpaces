// Copyright (c) 2025, D.R.Kuvshinov. All rights reserved.
// This file is part of TabsToSpaces utility 
// See LICENSE file for license and warranty information.
#include "tabs_to_spaces.hpp"
#include <stdexcept>
#include <string_view>
#include <algorithm>
#include <fstream>
#include <cstdint>

#ifdef  TABS_TO_SPACES_TEST_ENABLED
#include <iostream>
#include <iomanip>
#endif//TABS_TO_SPACES_TEST_ENABLED


namespace TabsToSpaces
{

    using namespace std::literals;

    auto tabsToSpaces(
            std::string_view    file, 
            int                 tabWidth,
            LineEndingMode      lineEndingMode
        ) -> std::string
    {
        if (tabWidth < 1) {
            throw std::invalid_argument("tabsToSpaces: tabWidth < 1");
        }

        std::string output(file.size() 
            + std::ranges::count(file, '\t') * tabWidth
            + (lineEndingMode == LineEndingMode::CrLf? std::ranges::count(file, '\n') * 2: 0),
            '\0');
        
        auto       write   = output.data();
        auto       read    = file.data();
        auto const readEnd = read + file.size();
        
        int  column = 0;
        bool hasCr  = false;

        while (read != readEnd) {
            switch (auto const in = *read++) {
            case '\t':
                if (hasCr && lineEndingMode == LineEndingMode::Lf) {
                    *write++ = '\r';
                }

                for (; column < tabWidth; ++column) {
                    *write++ = ' ';
                }

                column = 0;
                hasCr = false;
                break;
                
            case '\n':
                if (lineEndingMode == LineEndingMode::CrLf && !hasCr) {
                    *write++ = '\r';
                }

                *write++ = in;
                column = 0;
                hasCr = false;
                break;

            default:
                if (hasCr && lineEndingMode == LineEndingMode::Lf) {
                    *write++ = '\r';
                } // else writes CR immediately.

                hasCr = in == '\r';
                if (!hasCr || lineEndingMode != LineEndingMode::Lf) {
                    *write++ = in;
                } // else writes CR before the next character that is not LF.

                // CR and NUL are assumed to have zero width.
                column += in != '\0' && in != '\r';
                if (column == tabWidth) {
                    column = 0;
                }
            }
        }

        output.resize(write - output.data());
        return output;
    }

#ifdef  TABS_TO_SPACES_TEST_ENABLED
    [[nodiscard]] auto toString(LineEndingMode lineEndingMode) noexcept
        -> std::string_view
    {
        using enum LineEndingMode;
        switch (lineEndingMode) {
        case Ignore: return "Ignore"sv;
        case Lf:     return "Lf"sv;
        case CrLf:   return "CrLf"sv;
        }

        return "Unknown"sv;
    }

    [[nodiscard]] int test_tabsToSpaces(
            std::string_view    file, 
            int                 tabWidth, 
            LineEndingMode      lineEndingMode,
            std::string_view    expected
        )
    {
        if (auto answer = tabsToSpaces(file, tabWidth, lineEndingMode); answer != expected) {
            std::cerr << "Test failed: tabsToSpaces("sv
                      << std::quoted(file) << ", "sv 
                      << tabWidth << ", "sv
                      << toString(lineEndingMode) << ") ==\n"sv
                      << std::quoted(answer) << "\n!=\n"sv
                      << std::quoted(expected) << '\n';
            
            return 1;
        }

        return 0;
    }

    int test_tabsToSpaces()
    {
        struct TestCase
        {
            int                 tabWidth;
            std::string_view    file;
            std::string_view    expected;
            LineEndingMode      lineEnding = LineEndingMode::Ignore;
        };

        constexpr TestCase testCases[]
        {
            {
                4, ""sv, ""sv
            },

            {
                4, " we have here\n\r\r  no tabs   at all\r\n\n \n\r"sv,
                " we have here\n\r\r  no tabs   at all\r\n\n \n\r"sv
            },

            {
                4, "\t \t"sv, "        "sv
            },

            {
                2, "\t \t"sv, "    "sv
            },

            {
                1, "\t \t"sv, "   "sv
            },

            {
                3, "once\t\n \tupon a\ttime\r\n\twe"sv,
                "once  \n   upon a   time\r\n   we"sv
            },

            {
                4, "..\r\n..\r..\r\r..\n..\n\n..\n\r.."sv,
                "..\r\n..\r..\r\r..\n..\n\n..\n\r.."sv,
                LineEndingMode::Ignore
            },

            {
                4, "..\r\n..\r..\r\r..\n..\n\n..\n\r.."sv,
                "..\n..\r..\r\r..\n..\n\n..\n\r.."sv,
                LineEndingMode::Lf
            },

            {
                4, "..\r\n..\r..\r\r..\n..\n\n..\n\r.."sv,
                "..\r\n..\r..\r\r..\r\n..\r\n\r\n..\r\n\r.."sv,
                LineEndingMode::CrLf
            },

            {
                4, "..\t\r\n\t..\t\r\t..\t\r\r\t..\t\n\t..\t\n\n\t..\t\n\r\t.."sv,
                "..  \r\n    ..  \r    ..  \r\r    ..  \n    ..  \n\n    ..  \n\r    .."sv,
                LineEndingMode::Ignore
            },

            {
                4, "..\t\r\n\t..\t\r\t..\t\r\r\t..\t\n\t..\t\n\n\t..\t\n\r\t.."sv,
                "..  \n    ..  \r    ..  \r\r    ..  \n    ..  \n\n    ..  \n\r    .."sv,
                LineEndingMode::Lf
            },

            {
                4, "..\t\r\n\t..\t\r\t..\t\r\r\t..\t\n\t..\t\n\n\t..\t\n\r\t.."sv,
                "..  \r\n    ..  \r    ..  \r\r    ..  \r\n    ..  \r\n\r\n    ..  \r\n\r    .."sv,
                LineEndingMode::CrLf
            },
        };

        int errors = 0;
        for (auto& testCase : testCases) {
            errors += test_tabsToSpaces(
                    testCase.file, 
                    testCase.tabWidth,
                    testCase.lineEnding,
                    testCase.expected
                );
        }

        return errors;
    }
#endif//TABS_TO_SPACES_TEST_ENABLED


    namespace fs = std::filesystem;

    namespace
    {

        [[nodiscard]] auto loadFileToString(fs::path const& filename)
            -> std::string
        {
            auto const fileSizeUmax = fs::file_size(filename);
            if (fileSizeUmax > SIZE_MAX) {
                throw std::length_error("file is too big: "s + filename.string());
            }

            auto const fileSize = static_cast<std::size_t>(fileSizeUmax);
            std::string bytes(fileSize, '\0');
            std::ifstream file(filename, std::ios::binary);
            file.read(bytes.data(), fileSize);
            return bytes;
        }

    }

    void tabsToSpaces(
            fs::path const& filename, 
            int             tabWidth,
            LineEndingMode  lineEndingMode
        )
    {
        auto input  = loadFileToString(filename);
        auto output = tabsToSpaces(std::string_view{input}, tabWidth, lineEndingMode);

        input = std::string{};

        fs::path outputName = filename / ".tmp";
        std::ofstream file(outputName, std::ios::binary);
        file.write(output.data(), output.size());

        output = std::string{};

        fs::rename(outputName, filename);
    }

}
