// Copyright (c) 2025, D.R.Kuvshinov. All rights reserved.
// This file is part of TabsToSpaces utility 
// See LICENSE file for license and warranty information.
#ifndef TABS_TO_SPACES_HPP
#define TABS_TO_SPACES_HPP

#include <string>
#include <string_view>
#include <filesystem>

#if defined(_DEBUG) || defined(DEBUG)
#define TABS_TO_SPACES_TEST_ENABLED
#endif//DEBUG


namespace TabsToSpaces
{

    enum class LineEndingMode
    {
        Ignore,
        Lf,
        CrLf,
    };

    enum class WhitespaceBeforeNewLines
    {
        DoNotTrim,
        Trim,
    };

    struct Config
    {
        int                      tabWidth                   = 4;
        LineEndingMode           lineEndingMode             = LineEndingMode::Ignore;
        WhitespaceBeforeNewLines whitespaceBeforeNewLines   = WhitespaceBeforeNewLines::DoNotTrim;
    };


    [[nodiscard]] auto tabsToSpaces(
            std::string_view file, 
            Config           config = {}
        ) -> std::string;

    void tabsToSpaces(
            std::filesystem::path const& path, 
            Config                       config    = {}
        );

#ifdef  TABS_TO_SPACES_TEST_ENABLED
    int test_tabsToSpaces();
#endif//TABS_TO_SPACES_TEST_ENABLED

}

#endif//TABS_TO_SPACES_HPP
