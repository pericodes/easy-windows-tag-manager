#include "ExplorerCommand.h"

#include <shlobj.h>
#include <shellapi.h>

#include <fstream>
#include <string>
#include <vector>

static const CLSID CLSID_MediaTags =
{
    0x7d3b1f20,
    0x6f62,
    0x4b3a,
    {0x91, 0x62, 0x18, 0x42,
     0x77, 0x52, 0x10, 0xA1}
};

ExplorerCommand::ExplorerCommand()
{
}

ExplorerCommand::~ExplorerCommand()
{
}

HRESULT STDMETHODCALLTYPE
ExplorerCommand::QueryInterface(
    REFIID riid,
    void** ppv)
{
    if (!ppv)
        return E_POINTER;

    *ppv = nullptr;

    if (riid == IID_IUnknown ||
        riid == IID_IExplorerCommand)
    {
        *ppv =
            static_cast<IExplorerCommand*>(this);

        AddRef();

        return S_OK;
    }

    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE
ExplorerCommand::AddRef()
{
    return InterlockedIncrement(
        &m_ref
    );
}

ULONG STDMETHODCALLTYPE
ExplorerCommand::Release()
{
    ULONG value =
        InterlockedDecrement(
            &m_ref
        );

    if (value == 0)
        delete this;

    return value;
}

HRESULT STDMETHODCALLTYPE
ExplorerCommand::GetTitle(
    IShellItemArray*,
    PWSTR* title)
{
    if (!title)
        return E_POINTER;

    return SHStrDupW(
        L"Gestionar tags",
        title
    );
}

HRESULT STDMETHODCALLTYPE
ExplorerCommand::GetIcon(
    IShellItemArray*,
    PWSTR* icon)
{
    if (!icon)
        return E_POINTER;

    *icon = nullptr;

    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE
ExplorerCommand::GetToolTip(
    IShellItemArray*,
    PWSTR* tooltip)
{
    if (!tooltip)
        return E_POINTER;

    return SHStrDupW(
        L"Añadir o eliminar tags",
        tooltip
    );
}

HRESULT STDMETHODCALLTYPE
ExplorerCommand::GetCanonicalName(
    GUID* guid)
{
    if (!guid)
        return E_POINTER;

    *guid = CLSID_MediaTags;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE
ExplorerCommand::GetState(
    IShellItemArray*,
    BOOL,
    EXPCMDSTATE* state)
{
    if (!state)
        return E_POINTER;

    *state = ECS_ENABLED;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE
ExplorerCommand::GetFlags(
    EXPCMDFLAGS* flags)
{
    if (!flags)
        return E_POINTER;

    *flags =
        ECF_DEFAULT;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE
ExplorerCommand::EnumSubCommands(
    IEnumExplorerCommand** enumCommands)
{
    if (!enumCommands)
        return E_POINTER;

    *enumCommands = nullptr;

    return E_NOTIMPL;
}

static std::wstring
GetCurrentDllDirectory()
{
    wchar_t buffer[MAX_PATH]{};

    HMODULE module = nullptr;

    GetModuleHandleExW(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCWSTR>(
            &GetCurrentDllDirectory),
        &module
    );

    GetModuleFileNameW(
        module,
        buffer,
        MAX_PATH
    );

    std::wstring path(buffer);

    auto slash =
        path.find_last_of(
            L"\\/"
        );

    if (slash != std::wstring::npos)
        path.resize(slash);

    return path;
}

static std::wstring
CreateSelectionFile(
    IShellItemArray* items)
{
    PWSTR tempPath = nullptr;

    if (FAILED(
        SHGetKnownFolderPath(
            FOLDERID_LocalAppData,
            KF_FLAG_DEFAULT,
            nullptr,
            &tempPath)))
    {
        return L"";
    }

    std::wstring directory =
        tempPath;

    CoTaskMemFree(tempPath);

    directory +=
        L"\\MediaTags";

    CreateDirectoryW(
        directory.c_str(),
        nullptr
    );

    wchar_t filename[100];

    GUID guid;
    CoCreateGuid(&guid);

    StringFromGUID2(
        guid,
        filename,
        100
    );

    std::wstring file =
        directory +
        L"\\" +
        filename +
        L".txt";

    std::wofstream output(
        file,
        std::ios::binary
    );

    if (!output)
        return L"";

    DWORD count = 0;

    if (FAILED(
        items->GetCount(&count)))
    {
        return L"";
    }

    for (DWORD i = 0;
         i < count;
         ++i)
    {
        IShellItem* item = nullptr;

        if (FAILED(
            items->GetItemAt(
                i,
                &item)))
        {
            continue;
        }

        PWSTR path = nullptr;

        if (SUCCEEDED(
            item->GetDisplayName(
                SIGDN_FILESYSPATH,
                &path)))
        {
            output
                << path
                << L"\r\n";

            CoTaskMemFree(path);
        }

        item->Release();
    }

    output.close();

    return file;
}

HRESULT STDMETHODCALLTYPE
ExplorerCommand::Invoke(
    IShellItemArray* items,
    IBindCtx*)
{
    if (!items)
        return E_INVALIDARG;

    std::wstring selection =
        CreateSelectionFile(items);

    if (selection.empty())
        return E_FAIL;

    std::wstring exe =
        GetCurrentDllDirectory();

    exe += L"\\MediaTags.exe";

    std::wstring commandLine =
        L"\"" +
        exe +
        L"\" --selection \"" +
        selection +
        L"\"";

    STARTUPINFOW si{};
    si.cb = sizeof(si);

    PROCESS_INFORMATION pi{};

    std::vector<wchar_t> command(
        commandLine.begin(),
        commandLine.end()
    );

    command.push_back(L'\0');

    BOOL ok =
        CreateProcessW(
            exe.c_str(),
            command.data(),
            nullptr,
            nullptr,
            FALSE,
            0,
            nullptr,
            nullptr,
            &si,
            &pi
        );

    if (!ok)
        return HRESULT_FROM_WIN32(
            GetLastError()
        );

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    return S_OK;
}
