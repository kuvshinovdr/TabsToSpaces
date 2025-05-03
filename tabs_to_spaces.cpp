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
#endif//TABS_TO_SPACES_TEST_ENABLED


namespace TabsToSpaces
{

    using namespace std::literals;


    namespace
    {

        [[nodiscard]] auto nlProbe(
                char const*     from,
                char const*     to,
                LineEndingMode  lineEndingMode
            ) -> char const*
        {
            bool hasCr = false;
            while (from != to) {
                auto const in = *from++;
                switch (in) {
                case ' ':
                case '\t':
                    hasCr = false;
                    break;

                case '\r':
                    hasCr = true;
                    break;

                case '\n':
                    if (hasCr && lineEndingMode == LineEndingMode::Ignore)
                        return from - 2;
                    return from - 1;

                default:
                    return nullptr;
                }
            }
        }

    }


    auto tabsToSpaces(
            std::string_view    file, 
            Config              config
        ) -> std::string
    {
        auto const tabWidth = config.tabWidth;
        if (tabWidth < 1) {
            throw std::invalid_argument("tabsToSpaces: tab width must be greater than zero");
        }

        auto const lineEndingMode = config.lineEndingMode;
        std::string output(file.size() 
            + std::ranges::count(file, '\t') * tabWidth
            + (lineEndingMode == LineEndingMode::CrLf? std::ranges::count(file, '\n') * 2: 0),
            '\0');
        
        auto       write   = output.data();
        auto       read    = file.data();
        auto const readEnd = read + file.size();
        
        int  column = 0;
        bool hasCr  = false;

        bool const trim = config.whitespaceBeforeNewLines == WhitespaceBeforeNewLines::Trim;
        
        while (read != readEnd) {
            switch (auto const in = *read++) {
            case '\t':
                if (trim) {
                    if (auto nlPos = nlProbe(read - 1, readEnd, lineEndingMode)) {
                        read = nlPos;
                        continue;
                    }
                }

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
                if (in == ' ' && trim) {
                    if (auto nlPos = nlProbe(read - 1, readEnd, lineEndingMode)) {
                        read = nlPos;
                        continue;
                    }
                }

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

    [[nodiscard]] auto toString(WhitespaceBeforeNewLines whitespaceBeforeNewLines) noexcept
        -> std::string_view
    {
        using enum WhitespaceBeforeNewLines;
        switch (whitespaceBeforeNewLines) {
        case DoNotTrim: return "DoNotTrim"sv;
        case Trim:      return "Trim"sv;
        }

        return "Unknown"sv;
    }


    struct Quoted
    {
        std::string_view data;
    };

    auto operator<<(std::ostream& os, Quoted quoted)
        -> std::ostream&
    {
        os << '"';

        static constexpr std::string_view hex     = "0123456789abcdef"sv;
        static constexpr std::string_view special = "\t\r\n\\\""sv;
        static constexpr std::string_view substitute[]
        {
            "\\t"sv, "\\r"sv, "\\n"sv, "\\\\"sv, "\\\""sv
        };

        constexpr int SPACE = 32; // ASCII

        for (auto in : quoted.data) {
            if (auto sp = special.find(in); sp != special.npos) {
                os << substitute[sp];
            } else if (unsigned char code = in; code < SPACE) {
                os << "\\x"sv << hex[in >> 4] << hex[in & 0xF];
            } else {
                os.put(in);
            }
        }

        os << '"';
        return os;
    }


    [[nodiscard]] int test_tabsToSpaces(
            std::string_view    file, 
            Config              config,
            std::string_view    expected
        )
    {
        if (auto answer = tabsToSpaces(file, config); answer != expected) {
            std::clog << "Test failed: tabsToSpaces("sv
                      << Quoted{ file }                            << ", "sv 
                      << config.tabWidth                           << ", "sv
                      << toString(config.lineEndingMode)           << ", "sv
                      << toString(config.whitespaceBeforeNewLines) << ") ==\n"sv
                      << Quoted{ answer }                          << "\n!=\n"sv
                      << Quoted{ expected }                        << '\n';
            
            return 1;
        }

        return 0;
    }

    int test_tabsToSpaces()
    {
        struct TestCase
        {
            int                         tabWidth;
            std::string_view            file;
            std::string_view            expected;
            LineEndingMode              lineEndingMode              = LineEndingMode::Ignore;
            WhitespaceBeforeNewLines    whitespaceBeforeNewLines    = WhitespaceBeforeNewLines::DoNotTrim;
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

            {
                4, "\tline \t \nanother line\t\r\n"sv,
                "    line\nanother line\r\n"sv,
                LineEndingMode::Ignore, WhitespaceBeforeNewLines::Trim
            },

            {
                4, "\tline \t \nanother line\t\r\n"sv,
                "    line\nanother line\n"sv,
                LineEndingMode::Lf, WhitespaceBeforeNewLines::Trim
            },

            {
                4, "\tline \t \nanother line\t\r\n"sv,
                "    line\r\nanother line\r\n"sv,
                LineEndingMode::CrLf, WhitespaceBeforeNewLines::Trim
            },

            {
                4, "..\t\r\n\t..\t\r\t..\t\r\r\t..\t\n\t..\t\n\n\t..\t\n\r\t.."sv,
                "..\r\n    ..  \r    ..  \r\r    ..\n    ..\n\n    ..\n\r    .."sv,
                LineEndingMode::Ignore, WhitespaceBeforeNewLines::Trim
            },

            {
                4, "..\t\r\n\t..\t\r\t..\t\r\r\t..\t\n\t..\t\n\n\t..\t\n\r\t.."sv,
                "..\n    ..  \r    ..  \r\r    ..\n    ..\n\n    ..\n\r    .."sv,
                LineEndingMode::Lf, WhitespaceBeforeNewLines::Trim
            },

            {
                4, "..\t\r\n\t..\t\r\t..\t\r\r\t..\t\n\t..\t\n\n\t..\t\n\r\t.."sv,
                "..\r\n    ..  \r    ..  \r\r    ..\r\n    ..\r\n\r\n    ..\r\n\r    .."sv,
                LineEndingMode::CrLf, WhitespaceBeforeNewLines::Trim
            },
        };

        int errors = 0;
        for (auto& testCase : testCases) {
            errors += test_tabsToSpaces(
                    testCase.file,
                    {
                        .tabWidth                   = testCase.tabWidth,
                        .lineEndingMode             = testCase.lineEndingMode,
                        .whitespaceBeforeNewLines   = testCase.whitespaceBeforeNewLines
                    },
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
            Config          config
        )
    {
        auto input  = loadFileToString(filename);
        auto output = tabsToSpaces(std::string_view{input}, config);

        if (input != output) {
            input = std::string{};

            fs::path outputName = filename / ".tabs2spaces.tmp";
            std::ofstream file(outputName, std::ios::binary);
            file.write(output.data(), output.size());

            output = std::string{};

            fs::rename(outputName, filename);
        }
    }

}
