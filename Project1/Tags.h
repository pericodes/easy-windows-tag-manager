#pragma once

#include <windows.h>
#include <propkey.h>
#include <propsys.h>

#include <string>
#include <vector>

namespace Tags
{
    std::vector<std::wstring> Read(const std::wstring& file);

    bool Write(
        const std::wstring& file,
        const std::vector<std::wstring>& tags
    );

    bool Add(
        const std::wstring& file,
        const std::vector<std::wstring>& tags
    );

    bool Remove(
        const std::wstring& file,
        const std::vector<std::wstring>& tags
    );

    bool Rename(
        const std::wstring& file,
        const std::wstring& from,
        const std::wstring& to
    );
}
