#include "stdafx.hpp"

#include "core/adt/zstring_view.hpp"

template class sm::ZStringViewBase<char>;
template class sm::ZStringViewBase<char8_t>;
template class sm::ZStringViewBase<char16_t>;
template class sm::ZStringViewBase<char32_t>;
template class sm::ZStringViewBase<wchar_t>;
