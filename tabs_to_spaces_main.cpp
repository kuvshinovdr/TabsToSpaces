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
    constexpr std::string_view widthParam[] { "-w:"sv, "--width="sv };
    constexpr std::string_view helpParam = "--help"sv;

    if (argc == 1 || std::ranges::contains(argv + 1, argv + argc, helpParam)) {
        std::cout <<
"TabsToSpaces v.0.9 converts files passed as command line parameters by sub-\n"
"stituting each tab with spaces until the next column is reached.\n"
"Column (tab) width is 4 spaces by default but may be selected by using params\n"
"-w:width or --width=width.\n"sv;
    }

    int tabWidth = 4;
    for (int i = 1; i < argc; ++i) {
        std::string_view arg{argv[i]};
        try {
            if (arg.starts_with(widthParam[0])) {
                tabWidth = std::stoi(std::string{arg.substr(widthParam[0].size())});
            } else if (arg.starts_with(widthParam[1])) {
                tabWidth = std::stoi(std::string{arg.substr(widthParam[1].size())});
            } else {
                TabsToSpaces::tabsToSpaces(std::filesystem::path{argv[i]}, tabWidth);
            }
        } catch (std::exception const& e) {
            std::cerr << "On argument "sv << i 
                      << std::quoted(arg) 
                      << " error: "sv
                      << e.what() << '\n';
        }
    }
}
