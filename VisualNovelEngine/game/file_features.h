#pragma once
#include "string_features.h"

bool exists(const std::wstring& name)
{
	if (FILE* file = _wfopen(name.c_str(), L"r")) 
	{
		fclose(file);
		return true;
	}
	else
		return false;
}

std::string get_file_ext_(std::string name)
{
	return find_str311(name, ".");
}

std::wstring get_file_ext(std::wstring name)
{
	return find_str31(name, L".");
}

std::wstring get_filename_without_ext(std::wstring filename)
{
	return find_str41(filename, L".");
}

std::vector<std::string> get_directory_files_name(const std::string& directory, std::string extension_filter)
{
	WIN32_FIND_DATAA findData;
	HANDLE hFind = INVALID_HANDLE_VALUE;

	std::string full_path = directory + "\\";
	std::string full_path_with_filter = full_path + "*";

	std::vector<std::string> dir_list;

	hFind = FindFirstFileA(full_path_with_filter.c_str(), &findData);

	while (FindNextFileA(hFind, &findData) != 0)
	{
		if (get_file_ext_(std::string(findData.cFileName)) == extension_filter || extension_filter == "*")
			dir_list.push_back(std::string(findData.cFileName));
	}

	FindClose(hFind);

	return dir_list;
}

std::vector<std::wstring> get_directory_files_name_w(const std::wstring& directory, std::wstring extension_filter)
{
	WIN32_FIND_DATAW findData;
	HANDLE hFind = INVALID_HANDLE_VALUE;

	std::wstring full_path = directory + L"\\";
	std::wstring full_path_with_filter = full_path + L"*";

	std::vector<std::wstring> dir_list;

	hFind = FindFirstFileW(full_path_with_filter.c_str(), &findData);

	while (FindNextFileW(hFind, &findData) != 0)
	{
		if (get_file_ext(std::wstring(findData.cFileName)) == extension_filter || extension_filter == L"*")
			dir_list.push_back(std::wstring(findData.cFileName));
	}

	FindClose(hFind);

	return dir_list;
}