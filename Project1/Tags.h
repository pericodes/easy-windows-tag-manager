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

    enum class EditKind
    {
        Add,
        Remove,
        Rename
    };

    struct EditRequest
    {
        EditKind kind = EditKind::Add;
        std::vector<std::wstring> tags;
        std::wstring from;
        std::wstring to;
    };

    // Lee, modifica y escribe en una sola pasada. En vídeo ISO
    // intenta no copiar el archivo. result queda con los tags finales.
    bool Edit(
        const std::wstring& file,
        const EditRequest& request,
        std::vector<std::wstring>& result
    );
}
