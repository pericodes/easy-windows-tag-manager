#include "Tags.h"
#include "IsoKeywords.h"

#include <shobjidl.h>
#include <shlobj.h>
#include <propvarutil.h>

#pragma comment(lib, "propsys.lib")

#include <algorithm>

namespace
{
    std::wstring Normalize(const std::wstring& value)
    {
        std::wstring result = value;

        while (!result.empty() &&
               iswspace(result.front()))
        {
            result.erase(result.begin());
        }

        while (!result.empty() &&
               iswspace(result.back()))
        {
            result.pop_back();
        }

        return result;
    }

    bool Contains(
        const std::vector<std::wstring>& values,
        const std::wstring& value)
    {
        return std::find(
            values.begin(),
            values.end(),
            value
        ) != values.end();
    }

    void SortUnique(std::vector<std::wstring>& tags)
    {
        std::sort(tags.begin(), tags.end());

        tags.erase(
            std::unique(tags.begin(), tags.end()),
            tags.end());
    }

    void NotifyChanged(const std::wstring& file)
    {
        SHChangeNotify(
            SHCNE_UPDATEITEM,
            SHCNF_PATHW | SHCNF_FLUSHNOWAIT,
            file.c_str(),
            nullptr);
    }

    bool OpenStore(
        const std::wstring& file,
        GETPROPERTYSTOREFLAGS flags,
        IPropertyStore** store)
    {
        return SUCCEEDED(SHGetPropertyStoreFromParsingName(
            file.c_str(),
            nullptr,
            flags,
            IID_PPV_ARGS(store))) && *store;
    }

    std::vector<std::wstring> ReadStore(
        IPropertyStore* store)
    {
        std::vector<std::wstring> result;

        PROPVARIANT value;
        PropVariantInit(&value);

        const HRESULT hr = store->GetValue(
            PKEY_Keywords,
            &value);

        if (SUCCEEDED(hr))
        {
            LPWSTR* strings = nullptr;
            ULONG count = 0;

            if (SUCCEEDED(PropVariantToStringVectorAlloc(
                    value,
                    &strings,
                    &count)))
            {
                for (ULONG i = 0; i < count; ++i)
                {
                    if (strings[i])
                    {
                        auto tag = Normalize(strings[i]);

                        if (!tag.empty())
                            result.push_back(std::move(tag));

                        CoTaskMemFree(strings[i]);
                    }
                }

                CoTaskMemFree(strings);
            }
        }

        PropVariantClear(&value);
        SortUnique(result);
        return result;
    }

    bool WriteStore(
        IPropertyStore* store,
        const std::vector<std::wstring>& tags)
    {
        std::vector<std::wstring> clean;

        for (const auto& tag : tags)
        {
            auto normalized = Normalize(tag);

            if (!normalized.empty() &&
                !Contains(clean, normalized))
            {
                clean.push_back(std::move(normalized));
            }
        }

        PROPVARIANT value;
        PropVariantInit(&value);

        HRESULT hr = S_OK;

        if (clean.empty())
        {
            value.vt = VT_EMPTY;
        }
        else
        {
            std::vector<LPCWSTR> ptrs;

            for (const auto& tag : clean)
                ptrs.push_back(tag.c_str());

            hr = InitPropVariantFromStringVector(
                ptrs.data(),
                static_cast<ULONG>(ptrs.size()),
                &value);

            if (FAILED(hr))
                return false;
        }

        hr = store->SetValue(PKEY_Keywords, value);

        if (SUCCEEDED(hr))
            hr = store->Commit();

        PropVariantClear(&value);
        return SUCCEEDED(hr);
    }

    bool ApplyEdit(
        std::vector<std::wstring>& current,
        const Tags::EditRequest& request)
    {
        if (request.kind == Tags::EditKind::Add)
        {
            bool changed = false;

            for (const auto& tag : request.tags)
            {
                auto normalized = Normalize(tag);

                if (!normalized.empty() &&
                    !Contains(current, normalized))
                {
                    current.push_back(std::move(normalized));
                    changed = true;
                }
            }

            return changed;
        }

        if (request.kind == Tags::EditKind::Remove)
        {
            std::vector<std::wstring> drop;

            for (const auto& tag : request.tags)
            {
                auto normalized = Normalize(tag);

                if (!normalized.empty())
                    drop.push_back(std::move(normalized));
            }

            const auto before = current.size();

            current.erase(
                std::remove_if(
                    current.begin(),
                    current.end(),
                    [&](const std::wstring& value)
                    {
                        return Contains(drop, value);
                    }),
                current.end());

            return current.size() != before;
        }

        auto from = Normalize(request.from);
        auto to = Normalize(request.to);

        if (from.empty() || to.empty() || from == to)
            return false;

        if (!Contains(current, from))
            return false;

        current.erase(
            std::remove(current.begin(), current.end(), from),
            current.end());

        if (!Contains(current, to))
            current.push_back(std::move(to));

        return true;
    }

    bool SameTags(
        std::vector<std::wstring> left,
        std::vector<std::wstring> right)
    {
        SortUnique(left);
        SortUnique(right);
        return left == right;
    }

    bool EditOnStore(
        const std::wstring& file,
        const Tags::EditRequest& request,
        std::vector<std::wstring>& result)
    {
        IPropertyStore* store = nullptr;

        if (!OpenStore(
                file,
                GPS_READWRITE | GPS_HANDLERPROPERTIESONLY,
                &store))
        {
            if (!OpenStore(file, GPS_READWRITE, &store))
            {
                result = Tags::Read(file);
                return false;
            }
        }

        auto current = ReadStore(store);
        auto updated = current;

        if (!ApplyEdit(updated, request))
        {
            store->Release();
            result = std::move(current);
            return true;
        }

        SortUnique(updated);

        if (!WriteStore(store, updated))
        {
            store->Release();

            if (!Tags::Write(file, updated))
            {
                result = std::move(current);
                return false;
            }

            NotifyChanged(file);
            result = std::move(updated);
            return true;
        }

        store->Release();
        NotifyChanged(file);
        result = std::move(updated);
        return true;
    }
}

namespace Tags
{
    std::vector<std::wstring> Read(
        const std::wstring& file)
    {
        IPropertyStore* store = nullptr;

        if (!OpenStore(
                file,
                GPS_HANDLERPROPERTIESONLY | GPS_BESTEFFORT,
                &store))
        {
            if (!OpenStore(file, GPS_DEFAULT, &store))
                return {};
        }

        auto result = ReadStore(store);
        store->Release();
        return result;
    }

    bool Write(
        const std::wstring& file,
        const std::vector<std::wstring>& tags)
    {
        const GETPROPERTYSTOREFLAGS modes[] = {
            GPS_READWRITE | GPS_HANDLERPROPERTIESONLY,
            GPS_READWRITE
        };

        for (auto flags : modes)
        {
            IPropertyStore* store = nullptr;

            if (!OpenStore(file, flags, &store))
                continue;

            const bool ok = WriteStore(store, tags);
            store->Release();

            if (ok)
                return true;
        }

        return false;
    }

    bool Edit(
        const std::wstring& file,
        const EditRequest& request,
        std::vector<std::wstring>& result)
    {
        if (!CanPatchIsoKeywords(file))
            return EditOnStore(file, request, result);

        auto current = Read(file);
        auto updated = current;

        if (!ApplyEdit(updated, request))
        {
            result = std::move(current);
            return true;
        }

        SortUnique(updated);

        {
            IsoKeywordSession session;

            if (session.Commit(file, updated))
            {
                auto check = Read(file);

                if (SameTags(check, updated))
                {
                    session.Keep();
                    NotifyChanged(file);
                    result = std::move(check);
                    return true;
                }
            }
        }

        if (!Write(file, updated))
        {
            result = std::move(current);
            return false;
        }

        NotifyChanged(file);
        result = std::move(updated);
        return true;
    }

    bool Add(
        const std::wstring& file,
        const std::vector<std::wstring>& tags)
    {
        EditRequest request;
        request.kind = EditKind::Add;
        request.tags = tags;

        std::vector<std::wstring> result;
        return Edit(file, request, result);
    }

    bool Remove(
        const std::wstring& file,
        const std::vector<std::wstring>& tags)
    {
        EditRequest request;
        request.kind = EditKind::Remove;
        request.tags = tags;

        std::vector<std::wstring> result;
        return Edit(file, request, result);
    }

    bool Rename(
        const std::wstring& file,
        const std::wstring& from,
        const std::wstring& to)
    {
        EditRequest request;
        request.kind = EditKind::Rename;
        request.from = from;
        request.to = to;

        std::vector<std::wstring> result;
        return Edit(file, request, result);
    }
}
