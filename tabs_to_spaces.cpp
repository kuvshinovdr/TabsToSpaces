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

    auto tabsToSpaces(std::string_view file, int tabWidth)
        -> std::string
    {
        if (tabWidth < 1) {
            throw std::invalid_argument("tabsToSpaces: tabWidth < 1");
        }

        std::string output(file.size() + std::ranges::count(file, '\t') * tabWidth, '\0');
        
        auto       write   = output.data();
        auto       read    = file.data();
        auto const readEnd = read + file.size();
        
        int column = 0;
        while (read != readEnd) {
            switch (auto const in = *read++) {
            case '\t':
                for (; column != tabWidth; ++column) {
                    *write++ = ' ';
                }

                column = 0;
                break;
                
            case '\n':
                *write++ = in;
                column = 0;
                break;
            
            case '\r':
            case '\0':
                *write++ = in;
                break;

            default:
                *write++ = in;
                if (++column == tabWidth) {
                    column = 0;
                }
            }
        }

        output.resize(write - output.data());
        return output;
    }

#ifdef  TABS_TO_SPACES_TEST_ENABLED
    int test_tabsToSpaces(std::string_view file, int tabWidth, std::string_view expected)
    {
        if (auto answer = tabsToSpaces(file, tabWidth); answer != expected) {
            std::cerr << "Test failed: tabsToSpaces("sv
                      << std::quoted(file) << ", "sv << tabWidth << ") ==\n"sv
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
            int tabWidth;
            std::string_view file;
            std::string_view expected;
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
            }
        };

        int errors = 0;
        for (auto& testCase : testCases) {
            errors += test_tabsToSpaces(testCase.file, testCase.tabWidth, testCase.expected);
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

    void tabsToSpaces(fs::path const& filename, int tabWidth)
    {
        auto input  = loadFileToString(filename);
        auto output = tabsToSpaces(std::string_view{input}, tabWidth);

        input = std::string{};

        fs::path outputName = filename / ".tmp";
        std::ofstream file(outputName, std::ios::binary);
        file.write(output.data(), output.size());

        output = std::string{};

        fs::rename(outputName, filename);
    }

}
