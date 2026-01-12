#pragma once

#include <hyprlang.hpp>

Hyprlang::CParseResult gridSizeXKeyword(const char* LHS, const char* RHS);
Hyprlang::CParseResult gridSizeYKeyword(const char* LHS, const char* RHS);
Hyprlang::CParseResult wrapAroundKeyword(const char* LHS, const char* RHS);
Hyprlang::CParseResult hyprgridGestureKeyword(const char* LHS, const char* RHS);

Hyprlang::CParseResult gridSizeKeyword(const char* LHS, const char* RHS, int& target, const char* name);
