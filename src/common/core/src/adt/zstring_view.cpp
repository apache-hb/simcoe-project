#include "stdafx.hpp"

#include "core/adt/zstring_view.hpp"

template class sm::BasicZStringView<char>;
template class sm::BasicZStringView<char8_t>;
template class sm::BasicZStringView<char16_t>;
template class sm::BasicZStringView<char32_t>;
template class sm::BasicZStringView<wchar_t>;
