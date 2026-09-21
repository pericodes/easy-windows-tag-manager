#include "Tags.h"

#include <shobjidl.h>
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
}

namespace Tags
{
    std::vector<std::wstring> Read(
        const std::wstring& file)
    {
        std::vector<std::wstring> result;

        IPropertyStore* store = nullptr;

        HRESULT hr =
            SHGetPropertyStoreFromParsingName(
                file.c_str(),
                nullptr,
                GPS_HANDLERPROPERTIESONLY | GPS_BESTEFFORT,
                IID_PPV_ARGS(&store)
            );

        if (FAILED(hr))
        {
            hr = SHGetPropertyStoreFromParsingName(
                file.c_str(),
                nullptr,
                GPS_DEFAULT,
                IID_PPV_ARGS(&store)
            );
        }

        if (FAILED(hr) || !store)
            return result;

        PROPVARIANT value;
        PropVariantInit(&value);

        hr = store->GetValue(
            PKEY_Keywords,
            &value
        );

        if (SUCCEEDED(hr))
        {
            LPWSTR* strings = nullptr;
            ULONG count = 0;

            hr = PropVariantToStringVectorAlloc(
                value,
                &strings,
                &count
            );

            if (SUCCEEDED(hr))
            {
                for (ULONG i = 0; i < count; ++i)
                {
                    if (strings[i])
                    {
                        auto tag =
                            Normalize(strings[i]);

                        if (!tag.empty())
                            result.push_back(tag);

                        CoTaskMemFree(strings[i]);
                    }
                }

                CoTaskMemFree(strings);
            }
        }

        PropVariantClear(&value);
        store->Release();

        std::sort(
            result.begin(),
            result.end()
        );

        result.erase(
            std::unique(
                result.begin(),
                result.end()
            ),
            result.end()
        );

        return result;
    }

    bool Write(
        const std::wstring& file,
        const std::vector<std::wstring>& tags)
    {
        IPropertyStore* store = nullptr;

        HRESULT hr =
            SHGetPropertyStoreFromParsingName(
                file.c_str(),
                nullptr,
                GPS_READWRITE,
                IID_PPV_ARGS(&store)
            );

        if (FAILED(hr) || !store)
            return false;

        std::vector<std::wstring> clean;

        for (const auto& tag : tags)
        {
            auto normalized = Normalize(tag);

            if (!normalized.empty() &&
                !Contains(clean, normalized))
            {
                clean.push_back(normalized);
            }
        }

        PROPVARIANT value;
        PropVariantInit(&value);

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
                &value
            );

            if (FAILED(hr))
            {
                store->Release();
                return false;
            }
        }

        hr = store->SetValue(
            PKEY_Keywords,
            value
        );

        if (SUCCEEDED(hr))
        {
            hr = store->Commit();
        }

        PropVariantClear(&value);
        store->Release();

        return SUCCEEDED(hr);
    }

    bool Add(
        const std::wstring& file,
        const std::vector<std::wstring>& tags)
    {
        auto current = Read(file);

        for (const auto& tag : tags)
        {
            auto normalized = Normalize(tag);

            if (!normalized.empty() &&
                !Contains(current, normalized))
            {
                current.push_back(normalized);
            }
        }

        return Write(file, current);
    }

    bool Remove(
        const std::wstring& file,
        const std::vector<std::wstring>& tags)
    {
        auto current = Read(file);

        current.erase(
            std::remove_if(
                current.begin(),
                current.end(),
                [&](const std::wstring& value)
                {
                    return Contains(tags, value);
                }
            ),
            current.end()
        );

        return Write(file, current);
    }
}
