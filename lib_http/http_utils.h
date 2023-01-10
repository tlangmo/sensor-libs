// ===========================================================
//
// Copyright (c) 2018 Motesque Inc.  All rights reserved.
//
// ===========================================================
#pragma once
#include <cstring>
#include <string>
#include <map>
#include <cstdio>

namespace motesque {

// maps to turn status code into descriptions, eg. '200 OK'
struct StatusCodes {
    static std::map<int, std::string> codes;
};
// actual conversion functions
std::string to_string(int i);
std::string to_string(int ul);

}; // end ns

// there is a reason those function life outside the motesque ns
// convenience "stream-light" operators. In embedded it is crazy to use ostream b/c of its code size
extern std::string& operator<<(std::string& lh, const int& in);
extern std::string& operator<<(std::string& lh, const std::string& in);


