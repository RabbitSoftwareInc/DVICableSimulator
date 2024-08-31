#pragma once
#include <string>
#include <codecvt>
#include <vector>
#include <sstream>

#include <Windows.h>

std::wstring s2ws(const std::string& str)
{
	using convert_typeX = std::codecvt_utf8<wchar_t>;
	std::wstring_convert<convert_typeX, wchar_t> converterX;

	return converterX.from_bytes(str);
}

std::string ws2s(const std::wstring& wstr)
{
	using convert_typeX = std::codecvt_utf8<wchar_t>;
	std::wstring_convert<convert_typeX, wchar_t> converterX;

	return converterX.to_bytes(wstr);
}

const char* utf8(const wchar_t* pStr)
{
	static char szBuf[1024];
	WideCharToMultiByte(CP_UTF8, 0, pStr, -1, szBuf, sizeof(szBuf), NULL, NULL);
	return szBuf;
}

bool VectorOfStringGetter(void* data, int n, const char** out_text)
{
	const std::vector<std::string> v = *(std::vector<std::string>*)data;
	*out_text = v[n].c_str();
	return true;
}

std::vector<std::wstring> split_string(std::wstring str, wchar_t delimiter)
{
	std::wstringstream ss(str);

	std::wstring token;
	std::vector<std::wstring> tokens;

	while (getline(ss, token, delimiter))
		tokens.push_back(token);

	return tokens;
}

std::wstring find_str1(std::wstring s, std::wstring del)
{
	int end = s.find(del);

	if (end == std::wstring::npos)
		return L"";

	return s.substr(end + 1);
}

std::wstring find_str21(std::wstring s, std::wstring del)
{
	int end = s.find(del);

	if (end == std::wstring::npos)
		return L"";

	return s.substr(0, end + 1);
}

std::wstring find_str31(std::wstring s, std::wstring del)
{
	int end = s.find(del);

	if (end == std::wstring::npos)
		return L"";

	return s.substr(end, s.length() - end);
}

std::wstring find_str41(std::wstring s, std::wstring del)
{
	int end = s.find(del);

	if (end == std::wstring::npos)
		return L"";

	return s.substr(0, end);
}

std::string find_str311(std::string s, std::string del)
{
	int end = s.find(del);

	if (end == std::string::npos)
		return "";

	return s.substr(end, s.length() - end);
}