// Copyright (c) 2025, D.R.Kuvshinov. All rights reserved.
// This file is part of TabsToSpaces utility
// See LICENSE file for license and warranty information.
#include "tabs_to_spaces.hpp"

#include <stdexcept>
#include <string_view>
#include <regex>
#include <algorithm>
#include <ranges>
#include <fstream>
#include <cstdint>

#ifdef  TABS_TO_SPACES_TEST_ENABLED
#include <iostream>
#endif//TABS_TO_SPACES_TEST_ENABLED

#ifdef _WIN32
#define WC(x) L##x
#else
#define WC(x) x
#endif

namespace TabsToSpaces
{

    using namespace std::literals;

    namespace
    {

        // Find the position of a newline character sequence after space characters.
        // Returns nullptr on non-space character.
        [[nodiscard]] auto newlineProbe(
                char const*     from,
                char const*     to,
                LineEndingMode  lineEndingMode
            ) -> char const*
        {
            for (bool hasCr = false; from != to; ++from) {
                switch (auto const in = *from) {
                case ' ':
                case '\t':
                    hasCr = false;
                    break;

                case '\r':
                    hasCr = true;
                    break;

                case '\n':
                    if (hasCr && lineEndingMode == LineEndingMode::Ignore) {
                        return from - 1;
                    }
                    return from;

                default:
                    return nullptr;
                }
            }

            return to;
        }

    }


    [[nodiscard]] auto estimateOutputSize(
            std::string_view    fileContents,
            int                 tabWidth,
            LineEndingMode      lineEndingMode
        ) -> std::size_t
    {
        auto const tabSpaceEstimate  = std::ranges::count(fileContents, '\t') * tabWidth;
        auto const additionalCrCount = std::ranges::count(fileContents, '\n');
        return fileContents.size() + tabSpaceEstimate + additionalCrCount;
    }


    auto tabsToSpaces(
            std::string_view    fileContents,
            Config              config
        ) -> std::string
    {
        auto const tabWidth = config.tabWidth;
        if (tabWidth < 1) {
            throw std::invalid_argument("tabsToSpaces: tab width must be greater than zero");
        }

        auto const lineEndingMode = config.lineEndingMode;
        std::string output(
            estimateOutputSize(fileContents, tabWidth, lineEndingMode),
            '\0');

        auto       write   = output.data();
        auto       read    = fileContents.data();
        auto const readEnd = read + fileContents.size();

        int  column = 0;
        bool hasCr  = false;

        bool const trim = config.whitespaceBeforeNewLines == WhitespaceBeforeNewLines::Trim;
        bool const lf   = lineEndingMode == LineEndingMode::Lf;
        bool const crlf = lineEndingMode == LineEndingMode::CrLf;

        while (read != readEnd) {
            switch (auto const in = *read++) {
            case '\t':
                if (trim) {
                    if (auto nlPos = newlineProbe(read - 1, readEnd, lineEndingMode)) {
                        read = nlPos;
                        continue;
                    }
                }

                if (lf && hasCr) {
                    *write++ = '\r';
                }

                for (; column < tabWidth; ++column) {
                    *write++ = ' ';
                }

                column = 0;
                hasCr  = false;
                break;

            case '\n':
                if (crlf && !hasCr) {
                    *write++ = '\r';
                }

                *write++ = in;
                column   = 0;
                hasCr    = false;
                break;

            default:
                if (trim && in == ' ') {
                    if (auto nlPos = newlineProbe(read - 1, readEnd, lineEndingMode)) {
                        read = nlPos;
                        continue;
                    }
                }

                if (lf && hasCr) {
                    *write++ = '\r';
                } // else writes CR immediately.

                hasCr = in == '\r';
                if (!lf || !hasCr) {
                    *write++ = in;
                } // else writes CR before the next character that is not LF.

                // CR and NUL are assumed to have zero width.
                column += in != '\0' && in != '\r';
                if (column == tabWidth) {
                    column = 0;
                }
            }

        #ifdef  TABS_TO_SPACES_TEST_ENABLED
            if (static_cast<std::size_t>(write - output.data()) > output.size()) {
                throw std::logic_error("tabsToSpaces: invalid output size estimate detected");
            }
        #endif//TABS_TO_SPACES_TEST_ENABLED
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
                throw std::length_error("File is too big: "s + filename.string());
            }

            auto const fileSize = static_cast<std::size_t>(fileSizeUmax);
            std::string bytes(fileSize, '\0');

            std::ifstream file(filename, std::ios::binary);
            if (!file.is_open()) {
                throw std::runtime_error("File read failed: "s + filename.string());
            }

            file.read(bytes.data(), fileSize);
            return bytes;
        }

        void processOneFile(
                fs::path const& filename,
                Config          config
            )
        {
            auto input  = loadFileToString(filename);
            auto output = tabsToSpaces(std::string_view{input}, config);

            if (input != output) {
                input = std::string{};

                fs::path outputName = filename;
                outputName += WC(".tabs2spaces.tmp"sv);

                std::ofstream file(outputName, std::ios::binary);
                file.write(output.data(), output.size());
                file.close();

                output = std::string{};
                fs::rename(outputName, filename);
            }
        }

        [[nodiscard]] bool detectRegexPath(
            fs::path::string_type const& path) noexcept
        {
            return path.find_first_of(WC("*?"sv)) != path.npos;
        }

        [[nodiscard]] auto convertRegexString(
                fs::path::string_type const& path
            ) -> fs::path::string_type
        {
            fs::path::string_type result;
            for (auto ch : path) {
                switch (ch) {
                case WC('.'):
                    result += WC("\\."sv);
                    break;

                case WC('*'):
                    result += WC(".*"sv);
                    break;

                case WC('?'):
                    result += WC('.');
                    break;

                default:
                    result += ch;
                }
            }

            return result;
        }

    }


    void tabsToSpaces(
            fs::path const& path,
            Config          config
        )
    {
    #ifdef  TABS_TO_SPACES_TEST_ENABLED
        std::clog << "Doing "sv << path << '\n';
    #endif

        auto const filename = path.filename();
        if (!detectRegexPath(filename.native())) {
            return processOneFile(path, config);
        }

    #ifdef  TABS_TO_SPACES_TEST_ENABLED
        std::clog << "Regex path detected\n"sv;
    #endif

        std::basic_regex<fs::path::value_type> filenameRegex(
                convertRegexString(filename.native()),
                  std::regex_constants::basic
                | std::regex_constants::optimize
            #ifdef _WIN32
                | std::regex_constants::icase
            #endif
            );

        auto fileCond = [&filenameRegex](fs::directory_entry const& e)
            {
            #ifdef  TABS_TO_SPACES_TEST_ENABLED
                std::clog << "Testing "sv << e.path().filename() << '\n';
            #endif
                return e.is_regular_file()
                    && std::regex_match(e.path().filename().native(), filenameRegex);
            };

        auto fileProcess = [config](fs::directory_entry const& e)
            {
            #ifdef  TABS_TO_SPACES_TEST_ENABLED
                std::clog << "Processing: "sv << e << '\n';
            #endif
                processOneFile(e.path(), config);
            };

        switch (config.directoryWalk) {
        case DirectoryWalk::OneLevel:
            std::ranges::for_each(
                fs::directory_iterator(path.parent_path()) | std::views::filter(fileCond),
                fileProcess);
            break;

        case DirectoryWalk::Nested:
            std::ranges::for_each(
                fs::recursive_directory_iterator(path.parent_path()) | std::views::filter(fileCond),
                fileProcess);
            break;
        }
    }

}
