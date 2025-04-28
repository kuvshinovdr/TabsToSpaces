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
    std::cerr << "Running TabsToSpaces tests.\n";
    int errors = TabsToSpaces::test_tabsToSpaces();
    std::cerr << "Total errors: " << errors << '\n';
#endif//TABS_TO_SPACES_TEST_ENABLED

    using namespace std::literals;
    using namespace TabsToSpaces;
    using enum LineEndingMode;
    
    constexpr std::string_view helpParam = "--help"sv;
    constexpr std::string_view widthParam[] 
    { 
        "-w:"sv, 
        "--width="sv
    };
    
    constexpr std::string_view lfParam   = "--lf"sv;
    constexpr std::string_view crlfParam = "--crlf"sv;

    if (argc == 1 || std::ranges::contains(argv + 1, argv + argc, helpParam)) {
        std::cout <<
"TabsToSpaces v.1.0 converts files passed as command line parameters by sub-\n"
"stituting each tab with spaces until the next column is reached.\n"
"Column (tab) width is 4 spaces by default but may be selected by using params\n"
"-w:width or --width=width.\n"sv
"Parameter --lf enables conversion of CR LF to single LFs.\n"
"Parameter --crlf enables conversion of single LF (without preceding CR) into\n"
"CR LF sequences.\n"sv;
    }

    int tabWidth = 4;
    LineEndingMode lineEndingMode = Ignore;
    
    for (int i = 1; i < argc; ++i) {
        std::string_view arg{argv[i]};
        try {
            if (arg == lfParam) {
                lineEndingMode = Lf;
            } else if (arg == crlfParam) {
                lineEndingMode = CrLf;
            } else if (arg.starts_with(widthParam[0])) {
                tabWidth = std::stoi(std::string{arg.substr(widthParam[0].size())});
            } else if (arg.starts_with(widthParam[1])) {
                tabWidth = std::stoi(std::string{arg.substr(widthParam[1].size())});
            } else {
                tabsToSpaces(std::filesystem::path{argv[i]}, tabWidth, lineEndingMode);
            }
        } catch (std::exception const& e) {
            std::cerr << "On argument "sv << i 
                      << std::quoted(arg) 
                      << " error: "sv
                      << e.what() << '\n';
        }
    }
}
