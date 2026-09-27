#pragma once

#include <string>
#include <vector>

bool CanPatchIsoKeywords(const std::wstring& path);

// Escritura de System.Keywords en MP4/MOV sin copiar el mdat.
// Si Commit deja el archivo a medias y no se llama a Keep, el
// destructor restaura los bytes anteriores.
class IsoKeywordSession
{
public:

    IsoKeywordSession() = default;
    ~IsoKeywordSession();

    IsoKeywordSession(const IsoKeywordSession&) = delete;
    IsoKeywordSession& operator=(const IsoKeywordSession&) = delete;

    bool Commit(
        const std::wstring& path,
        const std::vector<std::wstring>& keywords);

    void Keep();

private:

    struct Backup
    {
        unsigned long long offset = 0;
        std::vector<unsigned char> bytes;
    };

    void Rollback();

    bool m_pending = false;
    std::wstring m_path;
    unsigned long long m_originalSize = 0;
    std::vector<Backup> m_backups;
};
