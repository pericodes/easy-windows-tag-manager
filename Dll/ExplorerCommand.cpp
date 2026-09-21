#include "pch.h"
#include "ExplorerCommand.h"

#include <shlobj.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <shlguid.h>

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
    if (m_site)
        m_site->Release();

    if (m_selection)
        m_selection->Release();
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
        *ppv = static_cast<IExplorerCommand*>(this);
        AddRef();
        return S_OK;
    }

    if (riid == IID_IObjectWithSite)
    {
        *ppv = static_cast<IObjectWithSite*>(this);
        AddRef();
        return S_OK;
    }

    if (riid == IID_IObjectWithSelection)
    {
        *ppv = static_cast<IObjectWithSelection*>(this);
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
ExplorerCommand::SetSite(
    IUnknown* site)
{
    if (m_site)
    {
        m_site->Release();
        m_site = nullptr;
    }

    m_site = site;
    if (m_site)
        m_site->AddRef();

    return S_OK;
}

HRESULT STDMETHODCALLTYPE
ExplorerCommand::GetSite(
    REFIID riid,
    void** ppv)
{
    if (!ppv)
        return E_POINTER;

    *ppv = nullptr;

    if (!m_site)
        return E_FAIL;

    return m_site->QueryInterface(riid, ppv);
}

HRESULT STDMETHODCALLTYPE
ExplorerCommand::SetSelection(
    IShellItemArray* items)
{
    if (m_selection)
    {
        m_selection->Release();
        m_selection = nullptr;
    }

    m_selection = items;
    if (m_selection)
        m_selection->AddRef();

    return S_OK;
}

HRESULT STDMETHODCALLTYPE
ExplorerCommand::GetSelection(
    REFIID riid,
    void** ppv)
{
    if (!ppv)
        return E_POINTER;

    *ppv = nullptr;

    if (!m_selection)
        return E_FAIL;

    return m_selection->QueryInterface(riid, ppv);
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
        L"A\u00F1adir o eliminar tags",
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
LocalAppData()
{
    PWSTR path = nullptr;
    if (FAILED(SHGetKnownFolderPath(
            FOLDERID_LocalAppData,
            KF_FLAG_DEFAULT,
            nullptr,
            &path)))
    {
        return L"";
    }

    std::wstring result = path;
    CoTaskMemFree(path);
    return result;
}

static std::wstring
GetCurrentDllDirectory()
{
    wchar_t buffer[MAX_PATH]{};
    HMODULE module = nullptr;

    GetModuleHandleExW(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCWSTR>(&GetCurrentDllDirectory),
        &module);

    GetModuleFileNameW(module, buffer, MAX_PATH);

    std::wstring path(buffer);
    const auto slash = path.find_last_of(L"\\/");
    if (slash != std::wstring::npos)
        path.resize(slash);

    return path;
}

static std::wstring
GetAppDirectory()
{
    const std::wstring installed =
        LocalAppData() + L"\\MediaTags";
    const std::wstring installedExe =
        installed + L"\\MediaTags.exe";

    if (GetFileAttributesW(installedExe.c_str()) != INVALID_FILE_ATTRIBUTES)
        return installed;

    return GetCurrentDllDirectory();
}

static HRESULT
ResolveItems(
    IUnknown* site,
    IShellItemArray* passed,
    IShellItemArray* cached,
    IShellItemArray** out)
{
    *out = nullptr;

    if (passed)
    {
        passed->AddRef();
        *out = passed;
        return S_OK;
    }

    if (cached)
    {
        cached->AddRef();
        *out = cached;
        return S_OK;
    }

    if (!site)
        return E_FAIL;

    IFolderView* view = nullptr;
    HRESULT hr = IUnknown_QueryService(
        site,
        SID_SFolderView,
        IID_PPV_ARGS(&view));

    if (FAILED(hr))
        return hr;

    hr = view->Items(
        SVGIO_SELECTION,
        IID_PPV_ARGS(out));

    view->Release();
    return hr;
}

static bool
WriteUtf8File(
    const std::wstring& file,
    const std::wstring& text)
{
    const int bytes = WideCharToMultiByte(
        CP_UTF8,
        0,
        text.c_str(),
        -1,
        nullptr,
        0,
        nullptr,
        nullptr);

    if (bytes <= 1)
        return false;

    std::string utf8(static_cast<size_t>(bytes - 1), '\0');
    WideCharToMultiByte(
        CP_UTF8,
        0,
        text.c_str(),
        -1,
        utf8.data(),
        bytes,
        nullptr,
        nullptr);

    HANDLE handle = CreateFileW(
        file.c_str(),
        GENERIC_WRITE,
        0,
        nullptr,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);

    if (handle == INVALID_HANDLE_VALUE)
        return false;

    DWORD written = 0;
    const BOOL ok = WriteFile(
        handle,
        utf8.data(),
        static_cast<DWORD>(utf8.size()),
        &written,
        nullptr);

    CloseHandle(handle);
    return ok == TRUE;
}

static std::wstring
CreateSelectionFile(
    IShellItemArray* items)
{
    const std::wstring directory =
        LocalAppData() + L"\\MediaTags";

    CreateDirectoryW(directory.c_str(), nullptr);

    wchar_t filename[64]{};
    GUID guid{};
    CoCreateGuid(&guid);
    StringFromGUID2(guid, filename, 64);

    const std::wstring file =
        directory + L"\\" + filename + L".txt";

    DWORD count = 0;
    if (FAILED(items->GetCount(&count)) || count == 0)
        return L"";

    std::wstring content;
    for (DWORD i = 0; i < count; ++i)
    {
        IShellItem* item = nullptr;
        if (FAILED(items->GetItemAt(i, &item)))
            continue;

        PWSTR path = nullptr;
        if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &path)))
        {
            content += path;
            content += L"\r\n";
            CoTaskMemFree(path);
        }

        item->Release();
    }

    if (content.empty() || !WriteUtf8File(file, content))
        return L"";

    return file;
}

static bool
LaunchApp(
    const std::wstring& exe,
    const std::wstring& selection)
{
    const std::wstring params =
        L"--selection \"" + selection + L"\"";

    SHELLEXECUTEINFOW info{};
    info.cbSize = sizeof(info);
    info.fMask = SEE_MASK_NOCLOSEPROCESS;
    info.lpVerb = L"open";
    info.lpFile = exe.c_str();
    info.lpParameters = params.c_str();
    info.nShow = SW_SHOWNORMAL;

    if (!ShellExecuteExW(&info))
        return false;

    if (info.hProcess)
    {
        AllowSetForegroundWindow(GetProcessId(info.hProcess));
        CloseHandle(info.hProcess);
    }

    return true;
}

HRESULT STDMETHODCALLTYPE
ExplorerCommand::Invoke(
    IShellItemArray* items,
    IBindCtx*)
{
    IShellItemArray* resolved = nullptr;
    const HRESULT hr = ResolveItems(
        m_site,
        items,
        m_selection,
        &resolved);

    if (FAILED(hr) || !resolved)
        return FAILED(hr) ? hr : E_FAIL;

    const std::wstring selection =
        CreateSelectionFile(resolved);
    resolved->Release();

    if (selection.empty())
        return E_FAIL;

    const std::wstring exe =
        GetAppDirectory() + L"\\MediaTags.exe";

    if (!LaunchApp(exe, selection))
    {
        MessageBoxW(
            nullptr,
            L"No se pudo abrir Media Tags.",
            L"Media Tags",
            MB_OK | MB_ICONERROR);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    return S_OK;
}
