#include "IsoKeywords.h"

#include <windows.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

namespace
{
    constexpr unsigned long long kMaxMoov =
        64ull * 1024ull * 1024ull;

    uint32_t ReadBe32(const unsigned char* p)
    {
        return (uint32_t(p[0]) << 24) |
               (uint32_t(p[1]) << 16) |
               (uint32_t(p[2]) << 8) |
               uint32_t(p[3]);
    }

    uint64_t ReadBe64(const unsigned char* p)
    {
        return (uint64_t(ReadBe32(p)) << 32) | ReadBe32(p + 4);
    }

    void WriteBe32(unsigned char* p, uint32_t value)
    {
        p[0] = static_cast<unsigned char>(value >> 24);
        p[1] = static_cast<unsigned char>(value >> 16);
        p[2] = static_cast<unsigned char>(value >> 8);
        p[3] = static_cast<unsigned char>(value);
    }

    void AppendBe32(
        std::vector<unsigned char>& out,
        uint32_t value)
    {
        unsigned char bytes[4];
        WriteBe32(bytes, value);
        out.insert(out.end(), bytes, bytes + 4);
    }

    void AppendBe16(
        std::vector<unsigned char>& out,
        uint16_t value)
    {
        out.push_back(static_cast<unsigned char>(value >> 8));
        out.push_back(static_cast<unsigned char>(value));
    }

    bool IsType(
        const std::vector<unsigned char>& box,
        const char* type)
    {
        return box.size() >= 8 &&
               memcmp(box.data() + 4, type, 4) == 0;
    }

    bool IsTypeAt(
        const char* have,
        const char* type)
    {
        return memcmp(have, type, 4) == 0;
    }

    bool IsPadding(const char* type)
    {
        return IsTypeAt(type, "free") ||
               IsTypeAt(type, "skip") ||
               IsTypeAt(type, "wide");
    }

    bool IsFragile(const char* type)
    {
        return IsTypeAt(type, "moof") ||
               IsTypeAt(type, "mfra") ||
               IsTypeAt(type, "sidx");
    }

    bool IsIsoPath(const std::wstring& path)
    {
        const auto dot = path.find_last_of(L'.');
        if (dot == std::wstring::npos)
            return false;

        const wchar_t* ext = path.c_str() + dot;

        return _wcsicmp(ext, L".mp4") == 0 ||
               _wcsicmp(ext, L".m4v") == 0 ||
               _wcsicmp(ext, L".mov") == 0 ||
               _wcsicmp(ext, L".3gp") == 0 ||
               _wcsicmp(ext, L".3g2") == 0;
    }

    class File
    {
    public:

        ~File()
        {
            Close();
        }

        bool Open(const std::wstring& path)
        {
            Close();

            m_handle = CreateFileW(
                path.c_str(),
                GENERIC_READ | GENERIC_WRITE,
                FILE_SHARE_READ |
                    FILE_SHARE_WRITE |
                    FILE_SHARE_DELETE,
                nullptr,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                nullptr);

            return m_handle != INVALID_HANDLE_VALUE;
        }

        void Close()
        {
            if (m_handle != INVALID_HANDLE_VALUE)
            {
                CloseHandle(m_handle);
                m_handle = INVALID_HANDLE_VALUE;
            }
        }

        unsigned long long Size() const
        {
            LARGE_INTEGER size{};
            if (!GetFileSizeEx(m_handle, &size) ||
                size.QuadPart < 0)
            {
                return 0;
            }

            return static_cast<unsigned long long>(
                size.QuadPart);
        }

        bool ReadAt(
            unsigned long long offset,
            void* data,
            size_t size)
        {
            auto* out = static_cast<unsigned char*>(data);
            size_t done = 0;

            while (done < size)
            {
                LARGE_INTEGER pos{};
                pos.QuadPart = static_cast<LONGLONG>(
                    offset + done);

                if (!SetFilePointerEx(
                        m_handle,
                        pos,
                        nullptr,
                        FILE_BEGIN))
                {
                    return false;
                }

                size_t chunk = size - done;
                if (chunk > (1u << 20))
                    chunk = (1u << 20);

                DWORD got = 0;
                if (!ReadFile(
                        m_handle,
                        out + done,
                        static_cast<DWORD>(chunk),
                        &got,
                        nullptr) ||
                    got == 0)
                {
                    return false;
                }

                done += got;
            }

            return true;
        }

        bool WriteAt(
            unsigned long long offset,
            const void* data,
            size_t size)
        {
            auto* in = static_cast<const unsigned char*>(data);
            size_t done = 0;

            while (done < size)
            {
                LARGE_INTEGER pos{};
                pos.QuadPart = static_cast<LONGLONG>(
                    offset + done);

                if (!SetFilePointerEx(
                        m_handle,
                        pos,
                        nullptr,
                        FILE_BEGIN))
                {
                    return false;
                }

                size_t chunk = size - done;
                if (chunk > (1u << 20))
                    chunk = (1u << 20);

                DWORD wrote = 0;
                if (!WriteFile(
                        m_handle,
                        in + done,
                        static_cast<DWORD>(chunk),
                        &wrote,
                        nullptr) ||
                    wrote == 0)
                {
                    return false;
                }

                done += wrote;
            }

            return true;
        }

        bool Truncate(unsigned long long size)
        {
            LARGE_INTEGER pos{};
            pos.QuadPart = static_cast<LONGLONG>(size);

            if (!SetFilePointerEx(
                    m_handle,
                    pos,
                    nullptr,
                    FILE_BEGIN))
            {
                return false;
            }

            return SetEndOfFile(m_handle) != FALSE;
        }

        bool Flush()
        {
            return FlushFileBuffers(m_handle) != FALSE;
        }

    private:

        HANDLE m_handle = INVALID_HANDLE_VALUE;
    };

    struct Box
    {
        unsigned long long offset = 0;
        unsigned long long size = 0;
        size_t header = 0;
        char type[4]{};
    };

    bool ReadBoxHeader(
        const unsigned char* data,
        size_t available,
        unsigned long long& total,
        size_t& header)
    {
        if (available < 8)
            return false;

        const uint32_t size32 = ReadBe32(data);
        header = 8;

        if (size32 == 1)
        {
            if (available < 16)
                return false;

            total = ReadBe64(data + 8);
            header = 16;
        }
        else if (size32 == 0)
        {
            total = available;
        }
        else
        {
            total = size32;
        }

        return total >= header && total <= available;
    }

    bool ReadBoxAt(
        File& file,
        unsigned long long offset,
        unsigned long long fileSize,
        Box& box)
    {
        if (offset + 8 > fileSize)
            return false;

        unsigned char hdr[16]{};
        if (!file.ReadAt(offset, hdr, 8))
            return false;

        const uint32_t size32 = ReadBe32(hdr);
        memcpy(box.type, hdr + 4, 4);
        box.offset = offset;
        box.header = 8;

        if (size32 == 1)
        {
            if (offset + 16 > fileSize ||
                !file.ReadAt(offset + 8, hdr, 8))
            {
                return false;
            }

            box.size = ReadBe64(hdr);
            box.header = 16;
        }
        else if (size32 == 0)
        {
            box.size = fileSize - offset;
        }
        else
        {
            box.size = size32;
        }

        if (box.size < box.header)
            return false;

        if (box.offset + box.size < box.offset)
            return false;

        return box.offset + box.size <= fileSize;
    }

    bool SplitChildren(
        const unsigned char* data,
        size_t length,
        std::vector<std::vector<unsigned char>>& children)
    {
        size_t pos = 0;

        while (pos + 8 <= length)
        {
            unsigned long long total = 0;
            size_t header = 0;

            if (!ReadBoxHeader(
                    data + pos,
                    length - pos,
                    total,
                    header))
            {
                return false;
            }

            const size_t bytes = static_cast<size_t>(total);
            children.emplace_back(
                data + pos,
                data + pos + bytes);
            pos += bytes;
        }

        return true;
    }

    std::vector<unsigned char> MakeBox(
        const char* type,
        const std::vector<unsigned char>& payload)
    {
        const unsigned long long size =
            8ull + payload.size();

        if (size > 0xFFFFFFFFull)
            return {};

        std::vector<unsigned char> out(static_cast<size_t>(size));
        WriteBe32(out.data(), static_cast<uint32_t>(size));
        memcpy(out.data() + 4, type, 4);

        if (!payload.empty())
        {
            memcpy(
                out.data() + 8,
                payload.data(),
                payload.size());
        }

        return out;
    }

    std::vector<unsigned char> Join(
        const std::vector<std::vector<unsigned char>>& children)
    {
        std::vector<unsigned char> payload;

        for (const auto& child : children)
        {
            payload.insert(
                payload.end(),
                child.begin(),
                child.end());
        }

        return payload;
    }

    bool IsCategoryName(
        const unsigned char* name,
        uint32_t length)
    {
        static const char kAscii[] = "WM/Category";

        if (length == 11 &&
            memcmp(name, kAscii, 11) == 0)
        {
            return true;
        }

        if (length != 22)
            return false;

        for (int i = 0; i < 11; ++i)
        {
            if (name[i * 2] != static_cast<unsigned char>(kAscii[i]) ||
                name[i * 2 + 1] != 0)
            {
                return false;
            }
        }

        return true;
    }

    std::vector<unsigned char> DefaultCategoryName()
    {
        static const wchar_t kName[] = L"WM/Category";
        std::vector<unsigned char> name;

        for (const wchar_t* p = kName; *p; ++p)
        {
            name.push_back(
                static_cast<unsigned char>(*p & 0xFF));
            name.push_back(
                static_cast<unsigned char>((*p >> 8) & 0xFF));
        }

        return name;
    }

    std::vector<unsigned char> BuildCategoryEntry(
        const std::vector<unsigned char>& name,
        const std::vector<std::wstring>& keywords)
    {
        std::vector<unsigned char> values;
        AppendBe32(
            values,
            static_cast<uint32_t>(keywords.size()));

        for (const auto& keyword : keywords)
        {
            std::vector<unsigned char> data;
            data.reserve((keyword.size() + 1) * 2);

            for (wchar_t ch : keyword)
            {
                data.push_back(
                    static_cast<unsigned char>(ch & 0xFF));
                data.push_back(
                    static_cast<unsigned char>((ch >> 8) & 0xFF));
            }

            data.push_back(0);
            data.push_back(0);

            const uint32_t valueSize =
                6u + static_cast<uint32_t>(data.size());

            AppendBe32(values, valueSize);
            AppendBe16(values, 8);
            values.insert(values.end(), data.begin(), data.end());
        }

        std::vector<unsigned char> entry;
        const uint32_t entrySize =
            8u +
            static_cast<uint32_t>(name.size()) +
            static_cast<uint32_t>(values.size());

        AppendBe32(entry, entrySize);
        AppendBe32(entry, static_cast<uint32_t>(name.size()));
        entry.insert(entry.end(), name.begin(), name.end());
        entry.insert(entry.end(), values.begin(), values.end());
        return entry;
    }

    bool RebuildXtraPayload(
        const unsigned char* data,
        size_t length,
        const std::vector<std::wstring>& keywords,
        std::vector<unsigned char>& nameUsed,
        std::vector<unsigned char>& out)
    {
        size_t pos = 0;
        std::vector<unsigned char> kept;
        bool sawName = false;

        while (pos + 8 <= length)
        {
            const uint32_t entrySize = ReadBe32(data + pos);

            if (entrySize < 8 || pos + entrySize > length)
                return false;

            const uint32_t nameLen = ReadBe32(data + pos + 4);

            if (8u + nameLen > entrySize)
                return false;

            const unsigned char* name = data + pos + 8;

            if (IsCategoryName(name, nameLen))
            {
                if (!sawName)
                {
                    nameUsed.assign(name, name + nameLen);
                    sawName = true;
                }
            }
            else
            {
                kept.insert(
                    kept.end(),
                    data + pos,
                    data + pos + entrySize);
            }

            pos += entrySize;
        }

        if (!sawName)
            nameUsed = DefaultCategoryName();

        out = std::move(kept);

        if (!keywords.empty())
        {
            auto entry = BuildCategoryEntry(nameUsed, keywords);
            out.insert(out.end(), entry.begin(), entry.end());
        }

        return true;
    }

    bool UpdateXtraBox(
        std::vector<unsigned char>& xtra,
        const std::vector<std::wstring>& keywords)
    {
        unsigned long long total = 0;
        size_t header = 0;

        if (!ReadBoxHeader(
                xtra.data(),
                xtra.size(),
                total,
                header) ||
            total != xtra.size() ||
            header != 8)
        {
            return false;
        }

        std::vector<unsigned char> name;
        std::vector<unsigned char> payload;

        if (!RebuildXtraPayload(
                xtra.data() + header,
                xtra.size() - header,
                keywords,
                name,
                payload))
        {
            return false;
        }

        if (payload.empty())
        {
            xtra.clear();
            return true;
        }

        xtra = MakeBox("Xtra", payload);
        return !xtra.empty();
    }

    bool UpdateUdta(
        std::vector<unsigned char>& udta,
        const std::vector<std::wstring>& keywords)
    {
        unsigned long long total = 0;
        size_t header = 0;

        if (!ReadBoxHeader(
                udta.data(),
                udta.size(),
                total,
                header) ||
            total != udta.size())
        {
            return false;
        }

        std::vector<std::vector<unsigned char>> children;

        if (!SplitChildren(
                udta.data() + header,
                udta.size() - header,
                children))
        {
            return false;
        }

        int xtraIndex = -1;

        for (size_t i = 0; i < children.size(); ++i)
        {
            if (IsType(children[i], "Xtra"))
                xtraIndex = static_cast<int>(i);
        }

        if (xtraIndex >= 0)
        {
            if (!UpdateXtraBox(children[xtraIndex], keywords))
                return false;

            if (children[xtraIndex].empty())
            {
                children.erase(
                    children.begin() + xtraIndex);
            }
        }
        else if (!keywords.empty())
        {
            std::vector<unsigned char> name;
            std::vector<unsigned char> payload;

            if (!RebuildXtraPayload(
                    nullptr,
                    0,
                    keywords,
                    name,
                    payload))
            {
                return false;
            }

            auto box = MakeBox("Xtra", payload);
            if (box.empty())
                return false;

            children.push_back(std::move(box));
        }

        udta = MakeBox("udta", Join(children));
        return !udta.empty();
    }

    bool UpdateMoovKeywords(
        std::vector<unsigned char>& moov,
        const std::vector<std::wstring>& keywords)
    {
        unsigned long long total = 0;
        size_t header = 0;

        if (moov.size() < 8 ||
            memcmp(moov.data() + 4, "moov", 4) != 0 ||
            !ReadBoxHeader(
                moov.data(),
                moov.size(),
                total,
                header) ||
            total != moov.size())
        {
            return false;
        }

        std::vector<std::vector<unsigned char>> children;

        if (!SplitChildren(
                moov.data() + header,
                moov.size() - header,
                children))
        {
            return false;
        }

        bool placed = false;

        for (auto& child : children)
        {
            const std::vector<std::wstring> none;

            if (IsType(child, "udta"))
            {
                if (!UpdateUdta(
                        child,
                        placed ? none : keywords))
                {
                    return false;
                }

                placed = true;
            }
            else if (IsType(child, "Xtra"))
            {
                if (!UpdateXtraBox(
                        child,
                        placed ? none : keywords))
                {
                    return false;
                }

                placed = true;
            }
        }

        children.erase(
            std::remove_if(
                children.begin(),
                children.end(),
                [](const std::vector<unsigned char>& child)
                {
                    return child.empty();
                }),
            children.end());

        if (!placed)
        {
            if (keywords.empty())
                return false;

            std::vector<unsigned char> name;
            std::vector<unsigned char> payload;

            if (!RebuildXtraPayload(
                    nullptr,
                    0,
                    keywords,
                    name,
                    payload))
            {
                return false;
            }

            auto xtra = MakeBox("Xtra", payload);
            auto udta = MakeBox("udta", xtra);

            if (xtra.empty() || udta.empty())
                return false;

            children.push_back(std::move(udta));
        }

        auto rebuilt = MakeBox("moov", Join(children));
        if (rebuilt.empty())
            return false;

        moov.swap(rebuilt);
        return true;
    }

    bool PadMoov(
        std::vector<unsigned char>& moov,
        size_t target)
    {
        if (moov.size() > target || moov.size() < 8)
            return false;

        if (moov.size() == target)
            return true;

        const size_t gap = target - moov.size();

        if (gap >= 8)
        {
            std::vector<unsigned char> freeBox(gap, 0);
            WriteBe32(
                freeBox.data(),
                static_cast<uint32_t>(gap));
            memcpy(freeBox.data() + 4, "free", 4);
            moov.insert(
                moov.end(),
                freeBox.begin(),
                freeBox.end());
        }
        else
        {
            moov.resize(target, 0);
        }

        if (moov.size() != target ||
            moov.size() > 0xFFFFFFFFull)
        {
            return false;
        }

        WriteBe32(
            moov.data(),
            static_cast<uint32_t>(moov.size()));

        return memcmp(moov.data() + 4, "moov", 4) == 0;
    }
}

bool CanPatchIsoKeywords(const std::wstring& path)
{
    return IsIsoPath(path);
}

IsoKeywordSession::~IsoKeywordSession()
{
    Rollback();
}

void IsoKeywordSession::Keep()
{
    m_pending = false;
    m_backups.clear();
}

void IsoKeywordSession::Rollback()
{
    if (!m_pending)
        return;

    m_pending = false;

    File file;
    if (!file.Open(m_path))
        return;

    for (size_t i = m_backups.size(); i-- > 0;)
    {
        const auto& backup = m_backups[i];

        if (!backup.bytes.empty())
        {
            file.WriteAt(
                backup.offset,
                backup.bytes.data(),
                backup.bytes.size());
        }
    }

    file.Truncate(m_originalSize);
    file.Flush();
    m_backups.clear();
}

bool IsoKeywordSession::Commit(
    const std::wstring& path,
    const std::vector<std::wstring>& keywords)
{
    if (m_pending)
        Rollback();

    if (!IsIsoPath(path))
        return false;

    if (keywords.size() > 256)
        return false;

    for (const auto& keyword : keywords)
    {
        if (keyword.size() > 4096)
            return false;
    }

    File file;
    if (!file.Open(path))
        return false;

    const unsigned long long fileSize = file.Size();
    if (fileSize < 8)
        return false;

    std::vector<Box> boxes;
    unsigned long long pos = 0;
    bool fragile = false;

    while (pos + 8 <= fileSize)
    {
        Box box;
        if (!ReadBoxAt(file, pos, fileSize, box))
            return false;

        if (IsFragile(box.type))
            fragile = true;

        boxes.push_back(box);

        if (box.size == 0)
            return false;

        pos += box.size;
    }

    if (pos != fileSize)
        return false;

    const Box* moov = nullptr;

    for (const auto& box : boxes)
    {
        if (IsTypeAt(box.type, "moov"))
        {
            moov = &box;
            break;
        }
    }

    if (!moov ||
        moov->size < 8 ||
        moov->size > kMaxMoov)
    {
        return false;
    }

    std::vector<unsigned char> updated(
        static_cast<size_t>(moov->size));

    if (!file.ReadAt(
            moov->offset,
            updated.data(),
            updated.size()))
    {
        return false;
    }

    const std::vector<unsigned char> original = updated;

    if (!UpdateMoovKeywords(updated, keywords) ||
        updated == original ||
        updated.size() < 8 ||
        updated.size() > kMaxMoov)
    {
        return false;
    }

    const unsigned long long oldSize = moov->size;
    const unsigned long long moovOffset = moov->offset;

    auto snapshot =
        [&](unsigned long long offset, size_t size) -> bool
    {
        if (size == 0 || offset >= fileSize)
            return true;

        size_t bytes = size;
        if (offset + bytes > fileSize)
            bytes = static_cast<size_t>(fileSize - offset);

        Backup backup;
        backup.offset = offset;
        backup.bytes.resize(bytes);

        if (!file.ReadAt(
                offset,
                backup.bytes.data(),
                backup.bytes.size()))
        {
            return false;
        }

        m_backups.push_back(std::move(backup));
        return true;
    };

    auto fail = [&]() -> bool
    {
        m_path = path;
        m_originalSize = fileSize;
        m_pending = true;
        file.Close();
        Rollback();
        return false;
    };

    m_backups.clear();
    m_path = path;
    m_originalSize = fileSize;

    if (updated.size() <= oldSize)
    {
        if (!PadMoov(updated, static_cast<size_t>(oldSize)))
            return false;

        if (!snapshot(moovOffset, updated.size()) ||
            !file.WriteAt(
                moovOffset,
                updated.data(),
                updated.size()))
        {
            return fail();
        }
    }
    else
    {
        const unsigned long long delta =
            updated.size() - oldSize;

        const Box* next = nullptr;
        for (size_t i = 0; i < boxes.size(); ++i)
        {
            if (boxes[i].offset == moovOffset &&
                i + 1 < boxes.size())
            {
                next = &boxes[i + 1];
                break;
            }
        }

        const bool moovIsLast =
            moovOffset + oldSize == fileSize;

        const bool canGrowIntoNext =
            next &&
            next->header == 8 &&
            IsPadding(next->type) &&
            next->size >= delta + 8 &&
            next->size - delta <= 0xFFFFFFFFull;

        if (canGrowIntoNext)
        {
            const size_t touched =
                updated.size() + 8;

            unsigned char freeHeader[8];
            WriteBe32(
                freeHeader,
                static_cast<uint32_t>(next->size - delta));
            memcpy(freeHeader + 4, next->type, 4);

            if (!snapshot(moovOffset, touched) ||
                !file.WriteAt(
                    moovOffset,
                    updated.data(),
                    updated.size()) ||
                !file.WriteAt(
                    moovOffset + updated.size(),
                    freeHeader,
                    8))
            {
                return fail();
            }
        }
        else if (moovIsLast)
        {
            if (!snapshot(
                    moovOffset,
                    static_cast<size_t>(oldSize)) ||
                !file.WriteAt(
                    moovOffset,
                    updated.data(),
                    updated.size()))
            {
                return fail();
            }
        }
        else if (!fragile)
        {
            static const char kFree[4] = {
                'f', 'r', 'e', 'e'
            };

            if (!snapshot(moovOffset + 4, 4) ||
                !file.WriteAt(
                    fileSize,
                    updated.data(),
                    updated.size()) ||
                !file.WriteAt(
                    moovOffset + 4,
                    kFree,
                    4))
            {
                return fail();
            }
        }
        else
        {
            return false;
        }
    }

    if (!file.Flush())
        return fail();

    file.Close();
    m_pending = true;
    return true;
}
