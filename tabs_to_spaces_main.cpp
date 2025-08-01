// Copyright (c) 2025, D.R.Kuvshinov. All rights reserved.
// This file is part of TabsToSpaces utility
// See LICENSE file for license and warranty information.
#include "tabs_to_spaces.hpp"
#include <iomanip>
#include <iostream>
#include <algorithm>

int main(int argc, char* argv[])
{
#ifdef  TABS_TO_SPACES_TEST_ENABLED
    {
        std::cerr << "Running TabsToSpaces tests.\n";
        int errors = TabsToSpaces::test_tabsToSpaces();
        std::cerr << "Total errors: " << errors << '\n';
        if (errors != 0) {
            return errors;
        }
    }
#endif//TABS_TO_SPACES_TEST_ENABLED

    using namespace std::literals;
    using namespace TabsToSpaces;

    constexpr std::string_view helpParam = "--help"sv;
    constexpr std::string_view widthParam[]
    {
        "-w:"sv,
        "--width="sv
    };

    constexpr std::string_view lfParam     = "--lf"sv;
    constexpr std::string_view crlfParam   = "--crlf"sv;
    constexpr std::string_view trimParam   = "--trim"sv;
    constexpr std::string_view noTrimParam = "--notrim"sv;
    constexpr std::string_view recParam    = "--rec"sv;
    constexpr std::string_view noRecParam  = "--norec"sv;

    if (argc == 1 || std::ranges::contains(argv + 1, argv + argc, helpParam)) {
        std::cout <<
"TabsToSpaces v.1.1b converts files passed as command line parameters by sub-\n"
"stituting each tab with spaces until the next column is reached.\n"
"Column (tab) width is 4 spaces by default but may be selected by using params\n"
"-w:width or --width=width.\n\n"
"Another parameters:\n"
"* --crlf enables conversion of single LF (without preceding CR) into\n"
"CR LF sequences.\n"
"* --lf enables conversion of CR LF to single LFs.\n"
"* --rec enables recursive (nested) directory walk (with subdirectories).\n"
"* --norec disables recursive directory walk (default option).\n"
"* --trim enables deleting all whitespaces before newlines.\n"
"* --notrim disables whitespace trimming (default option).\n"sv;
    }

    Config config;
    int errors = 0;

    for (int i = 1; i < argc; ++i) {
        std::string_view arg{argv[i]};

        auto errorPrologue = [&]
            {
                ++errors;
                std::clog << "On argument "sv << i
                    << std::quoted(arg);
            };

        try {
            if (arg == lfParam) {
                config.lineEndingMode = LineEndingMode::Lf;
            } else if (arg == crlfParam) {
                config.lineEndingMode = LineEndingMode::CrLf;
            } else if (arg == trimParam) {
                config.whitespaceBeforeNewLines = WhitespaceBeforeNewLines::Trim;
            } else if (arg == noTrimParam) {
                config.whitespaceBeforeNewLines = WhitespaceBeforeNewLines::DoNotTrim;
            } else if (arg == recParam) {
                config.directoryWalk = DirectoryWalk::Nested;
            } else if (arg == noRecParam) {
                config.directoryWalk = DirectoryWalk::OneLevel;
            } else if (arg.starts_with(widthParam[0])) {
                config.tabWidth = std::stoi( std::string{arg.substr(widthParam[0].size())} );
            } else if (arg.starts_with(widthParam[1])) {
                config.tabWidth = std::stoi( std::string{arg.substr(widthParam[1].size())} ); 
            } else {
                tabsToSpaces(std::filesystem::path{argv[i]}, config);
            }
        } catch (std::filesystem::filesystem_error const& e) {
            errorPrologue();
            std::clog << " error: "sv << e.what();
            if (!e.path1().empty() || !e.path2().empty()) {
                std::clog << "\nwith: "sv;
                if (!e.path1().empty()) {
                    std::clog << e.path1() << "; "sv;
                }

                if (!e.path2().empty()) {
                    std::clog << e.path2();
                }
            }

            std::clog << std::endl;
        } catch (std::exception const& e) {
            errorPrologue();
            std::clog << " error: "sv << e.what() << std::endl;
        } catch (...) {
            errorPrologue();
            std::clog << " unknown error.\n"sv;
        }
    }

    return errors;
}
