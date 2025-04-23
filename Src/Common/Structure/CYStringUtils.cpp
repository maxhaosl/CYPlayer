#include "Common/Structure/CYStringUtils.hpp"
#include "Common/Exception/CYException.hpp"

#include <wchar.h>

CYPLAYER_NAMESPACE_BEGIN

/**
 * @brief wstring convert to string.
*/
std::string CYStringUtils::WString2String(const std::wstring strSrc)
{
    const wchar_t* begin = strSrc.c_str();
    const wchar_t* end = begin + strSrc.size();

    std::string strLoc = setlocale(LC_ALL, nullptr);
    setlocale(LC_ALL, "chs");

    size_t nSize = std::wcsrtombs(nullptr, &begin, 0, nullptr);

    IfTrueThrow(nSize == static_cast<size_t>(-1), TEXT("Failed to convert wide string to multi-byte string"));

    std::string strResult(nSize, '\0');
    std::wcsrtombs(const_cast<char*>(strResult.data()), &begin, nSize, nullptr);

    setlocale(LC_ALL, strLoc.c_str());
    return strResult;
}

/**
 * @brief string convert to wstring.
*/
std::wstring CYStringUtils::String2WString(const std::string strSrc)
{
    std::string strLoc = setlocale(LC_ALL, nullptr);
    setlocale(LC_ALL, "chs");

    size_t nSize = mbstowcs(nullptr, strSrc.c_str(), 0);
    IfTrueThrow(nSize == static_cast<size_t>(-1), TEXT("Failed to convert multi-byte string to wide string"));

    std::wstring wstrResult(nSize, L'\0');
    mbstowcs(const_cast<wchar_t*>(wstrResult.data()), strSrc.c_str(), nSize);

    setlocale(LC_ALL, strLoc.c_str());
    return wstrResult;
}

CYPLAYER_NAMESPACE_END