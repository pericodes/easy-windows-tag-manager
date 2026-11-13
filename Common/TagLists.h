#pragma once

#include <windows.h>

#include <algorithm>
#include <string>
#include <vector>

namespace tag_lists
{
    constexpr wchar_t kKey[] = L"Software\\MediaTags";
    constexpr wchar_t kSaved[] = L"SavedTags";
    constexpr wchar_t kRecent[] = L"RecentTags";
    constexpr wchar_t kShowLibrary[] = L"ShowLibrary";
    constexpr size_t kRecentMax = 20;

    inline std::vector<std::wstring> ReadList(const wchar_t* valueName)
    {
        DWORD bytes = 0;

        const LONG query = RegGetValueW(
            HKEY_CURRENT_USER,
            kKey,
            valueName,
            RRF_RT_REG_MULTI_SZ,
            nullptr,
            nullptr,
            &bytes);

        if (query != ERROR_SUCCESS || bytes < sizeof(wchar_t) * 2)
            return {};

        std::vector<wchar_t> buf(bytes / sizeof(wchar_t) + 2, L'\0');

        if (RegGetValueW(
                HKEY_CURRENT_USER,
                kKey,
                valueName,
                RRF_RT_REG_MULTI_SZ,
                nullptr,
                buf.data(),
                &bytes) != ERROR_SUCCESS)
        {
            return {};
        }

        std::vector<std::wstring> tags;
        const wchar_t* cursor = buf.data();
        const wchar_t* end = buf.data() + buf.size();

        while (cursor < end && *cursor)
        {
            const std::wstring tag(cursor);
            cursor += tag.size() + 1;

            if (!tag.empty())
                tags.push_back(tag);
        }

        return tags;
    }

    inline bool WriteList(
        const wchar_t* valueName,
        const std::vector<std::wstring>& tags)
    {
        std::wstring packed;

        for (const auto& tag : tags)
        {
            if (tag.empty())
                continue;

            packed.append(tag);
            packed.push_back(L'\0');
        }

        if (packed.empty())
            packed.push_back(L'\0');

        packed.push_back(L'\0');

        return RegSetKeyValueW(
            HKEY_CURRENT_USER,
            kKey,
            valueName,
            REG_MULTI_SZ,
            packed.data(),
            static_cast<DWORD>(packed.size() * sizeof(wchar_t))) == ERROR_SUCCESS;
    }

    inline std::vector<std::wstring> ReadSaved()
    {
        std::vector<std::wstring> unique;

        for (const auto& tag : ReadList(kSaved))
        {
            if (std::find(unique.begin(), unique.end(), tag) == unique.end())
                unique.push_back(tag);
        }

        std::sort(unique.begin(), unique.end());
        return unique;
    }

    inline std::vector<std::wstring> ReadRecent()
    {
        std::vector<std::wstring> unique;

        for (const auto& tag : ReadList(kRecent))
        {
            if (std::find(unique.begin(), unique.end(), tag) != unique.end())
                continue;

            unique.push_back(tag);

            if (unique.size() == kRecentMax)
                break;
        }

        return unique;
    }

    inline void SaveTag(const std::wstring& tag)
    {
        if (tag.empty())
            return;

        auto saved = ReadSaved();

        if (std::find(saved.begin(), saved.end(), tag) != saved.end())
            return;

        saved.push_back(tag);
        std::sort(saved.begin(), saved.end());
        WriteList(kSaved, saved);
    }

    inline void UnsaveTag(const std::wstring& tag)
    {
        if (tag.empty())
            return;

        auto saved = ReadSaved();
        const auto end = std::remove(saved.begin(), saved.end(), tag);

        if (end == saved.end())
            return;

        saved.erase(end, saved.end());
        WriteList(kSaved, saved);
    }

    inline bool LibraryVisible()
    {
        wchar_t buf[8]{};
        DWORD size = sizeof(buf);

        if (RegGetValueW(
                HKEY_CURRENT_USER,
                kKey,
                kShowLibrary,
                RRF_RT_REG_SZ,
                nullptr,
                buf,
                &size) != ERROR_SUCCESS)
        {
            return true;
        }

        return buf[0] != L'0';
    }

    inline void SetLibraryVisible(bool visible)
    {
        const wchar_t* value = visible ? L"1" : L"0";

        RegSetKeyValueW(
            HKEY_CURRENT_USER,
            kKey,
            kShowLibrary,
            REG_SZ,
            value,
            static_cast<DWORD>((wcslen(value) + 1) * sizeof(wchar_t)));
    }

    inline void RememberRecent(const std::vector<std::wstring>& tags)
    {
        auto recent = ReadRecent();

        for (auto it = tags.rbegin(); it != tags.rend(); ++it)
        {
            if (it->empty())
                continue;

            recent.erase(
                std::remove(recent.begin(), recent.end(), *it),
                recent.end());

            recent.insert(recent.begin(), *it);
        }

        if (recent.size() > kRecentMax)
            recent.resize(kRecentMax);

        WriteList(kRecent, recent);
    }
}
