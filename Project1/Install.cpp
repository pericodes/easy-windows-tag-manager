#include "Install.h"
#include "MediaTypes.h"

#include <windows.h>
#include <shlobj.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <wincrypt.h>
#include <roapi.h>
#include <winstring.h>
#include <asyncinfo.h>
#include <wrl/client.h>
#include <wrl/wrappers/corewrappers.h>
#include <windows.foundation.h>
#include <windows.management.deployment.h>
#include <windows.applicationmodel.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "runtimeobject.lib")

namespace fs = std::filesystem;

namespace
{
    constexpr wchar_t kAppName[] = L"Media Tags";
    constexpr wchar_t kUninstallKey[] =
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\MediaTags";

    std::wstring ModulePath()
    {
        wchar_t buffer[MAX_PATH]{};
        GetModuleFileNameW(nullptr, buffer, MAX_PATH);
        return buffer;
    }

    fs::path ModuleDir()
    {
        return fs::path(ModulePath()).parent_path();
    }

    fs::path InstallDir()
    {
        PWSTR localAppData = nullptr;
        if (FAILED(SHGetKnownFolderPath(
                FOLDERID_LocalAppData,
                KF_FLAG_DEFAULT,
                nullptr,
                &localAppData)))
        {
            return {};
        }

        fs::path dir = fs::path(localAppData) / L"MediaTags";
        CoTaskMemFree(localAppData);
        return dir;
    }

    bool IsElevated()
    {
        BOOL admin = FALSE;
        PSID group = nullptr;
        SID_IDENTIFIER_AUTHORITY nt = SECURITY_NT_AUTHORITY;

        if (!AllocateAndInitializeSid(
                &nt,
                2,
                SECURITY_BUILTIN_DOMAIN_RID,
                DOMAIN_ALIAS_RID_ADMINS,
                0, 0, 0, 0, 0, 0,
                &group))
        {
            return false;
        }

        CheckTokenMembership(nullptr, group, &admin);
        FreeSid(group);
        return admin == TRUE;
    }

    int RelaunchElevated(const wchar_t* args)
    {
        const std::wstring path = ModulePath();

        SHELLEXECUTEINFOW info{};
        info.cbSize = sizeof(info);
        info.fMask = SEE_MASK_NOCLOSEPROCESS;
        info.lpVerb = L"runas";
        info.lpFile = path.c_str();
        info.lpParameters = args;
        info.nShow = SW_SHOWNORMAL;

        if (!ShellExecuteExW(&info))
            return HRESULT_FROM_WIN32(GetLastError());

        if (info.hProcess)
        {
            WaitForSingleObject(info.hProcess, INFINITE);

            DWORD code = 1;
            GetExitCodeProcess(info.hProcess, &code);
            CloseHandle(info.hProcess);
            return static_cast<int>(code);
        }

        return 1;
    }

    void Show(const wchar_t* text, bool silent, UINT icon = MB_ICONINFORMATION)
    {
        if (silent && icon != MB_ICONERROR)
            return;

        MessageBoxW(nullptr, text, kAppName, MB_OK | icon);
    }

    using Microsoft::WRL::ComPtr;
    using Microsoft::WRL::Wrappers::HStringReference;

    void EnsureWinRt()
    {
        const HRESULT hr = RoInitialize(RO_INIT_SINGLETHREADED);
        if (FAILED(hr) && hr != RPC_E_CHANGED_MODE)
            RoInitialize(RO_INIT_MULTITHREADED);
    }

    std::wstring FileUri(const fs::path& path)
    {
        const std::wstring abs = fs::absolute(path).wstring();
        wchar_t uri[2048]{};
        DWORD length = static_cast<DWORD>(std::size(uri));
        if (FAILED(UrlCreateFromPathW(abs.c_str(), uri, &length, 0)))
            return L"file:///" + abs;
        return uri;
    }

    HRESULT WaitAsync(ABI::Windows::Foundation::IAsyncInfo* info)
    {
        if (!info)
            return E_POINTER;

        ABI::Windows::Foundation::AsyncStatus status =
            ABI::Windows::Foundation::AsyncStatus::Started;
        while (status == ABI::Windows::Foundation::AsyncStatus::Started)
        {
            Sleep(30);
            const HRESULT hr = info->get_Status(&status);
            if (FAILED(hr))
                return hr;
        }

        if (status == ABI::Windows::Foundation::AsyncStatus::Completed)
            return S_OK;

        HRESULT error = E_FAIL;
        info->get_ErrorCode(&error);
        return error;
    }

    HRESULT CreateUri(const std::wstring& value, ComPtr<ABI::Windows::Foundation::IUriRuntimeClass>& uri)
    {
        ComPtr<ABI::Windows::Foundation::IUriRuntimeClassFactory> factory;
        HRESULT hr = RoGetActivationFactory(
            HStringReference(RuntimeClass_Windows_Foundation_Uri).Get(),
            IID_PPV_ARGS(&factory));
        if (FAILED(hr))
            return hr;

        return factory->CreateUri(HStringReference(value.c_str()).Get(), &uri);
    }

    bool ImportCertificate(const fs::path& cer)
    {
        std::ifstream input(cer, std::ios::binary);
        if (!input)
            return false;

        const std::vector<BYTE> data(
            (std::istreambuf_iterator<char>(input)),
            std::istreambuf_iterator<char>());

        if (data.empty())
            return false;

        PCCERT_CONTEXT context = CertCreateCertificateContext(
            X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
            data.data(),
            static_cast<DWORD>(data.size()));

        if (!context)
            return false;

        auto addToStore = [&](DWORD flags, const wchar_t* storeName)
        {
            HCERTSTORE store = CertOpenStore(
                CERT_STORE_PROV_SYSTEM_W,
                0,
                0,
                flags,
                storeName);

            if (!store)
                return false;

            const BOOL ok = CertAddCertificateContextToStore(
                store,
                context,
                CERT_STORE_ADD_REPLACE_EXISTING,
                nullptr);

            CertCloseStore(store, 0);
            return ok == TRUE;
        };

        const bool user = addToStore(
            CERT_SYSTEM_STORE_CURRENT_USER,
            L"TrustedPeople");
        addToStore(
            CERT_SYSTEM_STORE_LOCAL_MACHINE,
            L"TrustedPeople");

        CertFreeCertificateContext(context);
        return user;
    }

    void EnableSideloading()
    {
        HKEY key = nullptr;
        if (RegCreateKeyExW(
                HKEY_LOCAL_MACHINE,
                L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppModelUnlock",
                0,
                nullptr,
                0,
                KEY_SET_VALUE,
                nullptr,
                &key,
                nullptr) != ERROR_SUCCESS)
        {
            return;
        }

        DWORD value = 1;
        RegSetValueExW(
            key,
            L"AllowAllTrustedApps",
            0,
            REG_DWORD,
            reinterpret_cast<const BYTE*>(&value),
            sizeof(value));
        RegCloseKey(key);
    }

    void RemoveExistingPackage()
    {
        EnsureWinRt();

        ComPtr<IInspectable> inspectable;
        if (FAILED(RoActivateInstance(
                HStringReference(
                    RuntimeClass_Windows_Management_Deployment_PackageManager)
                    .Get(),
                &inspectable)))
        {
            return;
        }

        ComPtr<ABI::Windows::Management::Deployment::IPackageManager> manager;
        if (FAILED(inspectable.As(&manager)))
            return;

        ComPtr<ABI::Windows::Foundation::Collections::IIterable<
            ABI::Windows::ApplicationModel::Package*>>
            packages;
        if (FAILED(manager->FindPackagesByNamePublisher(
                HStringReference(L"MediaTags").Get(),
                HStringReference(L"CN=MediaTags").Get(),
                &packages)) ||
            !packages)
        {
            return;
        }

        ComPtr<ABI::Windows::Foundation::Collections::IIterator<
            ABI::Windows::ApplicationModel::Package*>>
            iterator;
        if (FAILED(packages->First(&iterator)) || !iterator)
            return;

        boolean hasCurrent = FALSE;
        iterator->get_HasCurrent(&hasCurrent);

        std::vector<std::wstring> names;
        while (hasCurrent)
        {
            ComPtr<ABI::Windows::ApplicationModel::IPackage> package;
            if (SUCCEEDED(iterator->get_Current(&package)) && package)
            {
                ComPtr<ABI::Windows::ApplicationModel::IPackageId> id;
                if (SUCCEEDED(package->get_Id(&id)) && id)
                {
                    HSTRING name{};
                    if (SUCCEEDED(id->get_Name(&name)))
                    {
                        UINT32 length = 0;
                        const wchar_t* raw = WindowsGetStringRawBuffer(name, &length);
                        if (raw && std::wstring(raw, length) == L"MediaTags")
                        {
                            HSTRING full{};
                            if (SUCCEEDED(id->get_FullName(&full)))
                            {
                                UINT32 fullLength = 0;
                                const wchar_t* fullRaw =
                                    WindowsGetStringRawBuffer(full, &fullLength);
                                if (fullRaw)
                                    names.emplace_back(fullRaw, fullLength);
                                WindowsDeleteString(full);
                            }
                        }
                        WindowsDeleteString(name);
                    }
                }
            }

            if (FAILED(iterator->MoveNext(&hasCurrent)))
                break;
        }

        for (const auto& fullName : names)
        {
            ComPtr<ABI::Windows::Foundation::IAsyncOperationWithProgress<
                ABI::Windows::Management::Deployment::DeploymentResult*,
                ABI::Windows::Management::Deployment::DeploymentProgress>>
                operation;
            if (FAILED(manager->RemovePackageAsync(
                    HStringReference(fullName.c_str()).Get(),
                    &operation)) ||
                !operation)
            {
                continue;
            }

            ComPtr<ABI::Windows::Foundation::IAsyncInfo> info;
            if (SUCCEEDED(operation.As(&info)))
                WaitAsync(info.Get());
        }
    }

    bool CopyReplaceFile(
        const fs::path& src,
        const fs::path& dest,
        std::error_code& ec)
    {
        ec.clear();
        fs::copy_file(
            src,
            dest,
            fs::copy_options::overwrite_existing,
            ec);

        if (!ec)
            return true;

        if (!fs::exists(dest))
            return false;

        const fs::path backup =
            dest.wstring() + L".old";

        std::error_code ignore;
        fs::remove(backup, ignore);

        if (!MoveFileExW(
                dest.c_str(),
                backup.c_str(),
                MOVEFILE_REPLACE_EXISTING))
        {
            return false;
        }

        ec.clear();
        fs::copy_file(
            src,
            dest,
            fs::copy_options::overwrite_existing,
            ec);

        if (ec)
            return false;

        if (!DeleteFileW(backup.c_str()))
        {
            MoveFileExW(
                backup.c_str(),
                nullptr,
                MOVEFILE_DELAY_UNTIL_REBOOT);
        }

        return true;
    }

    bool CopyPayload(const fs::path& from, const fs::path& to, std::wstring& error)
    {
        std::error_code ec;
        if (fs::exists(to) && fs::equivalent(from, to, ec) && !ec)
            return true;

        fs::create_directories(to, ec);
        if (ec)
        {
            error = L"No se pudo crear " + to.wstring();
            return false;
        }

        const wchar_t* required[] = {
            L"MediaTags.exe",
            L"MediaTagsShell.dll",
            L"MediaTags.msix",
            L"MediaTags.cer"
        };

        for (const wchar_t* name : required)
        {
            fs::path src = from / name;
            if (!fs::exists(src))
            {
                error =
                    L"Falta " +
                    src.wstring() +
                    L". Compila la soluci\u00F3n x64 para generar el instalador.";
                return false;
            }

            if (!CopyReplaceFile(src, to / name, ec))
            {
                error =
                    L"No se pudo actualizar " +
                    std::wstring(name) +
                    L". Cierra el Explorador de archivos y vuelve a instalar.";
                return false;
            }
        }

        fs::path loader = from / L"WebView2Loader.dll";
        if (fs::exists(loader))
            CopyReplaceFile(loader, to / L"WebView2Loader.dll", ec);

        fs::path webFrom = from / L"Web";
        fs::path webTo = to / L"Web";
        if (!fs::exists(webFrom))
        {
            error = L"Falta la carpeta Web junto al ejecutable.";
            return false;
        }

        fs::create_directories(webTo, ec);
        fs::copy(
            webFrom,
            webTo,
            fs::copy_options::overwrite_existing |
                fs::copy_options::recursive,
            ec);

        if (ec)
        {
            error = L"No se pudo copiar la interfaz Web.";
            return false;
        }

        return true;
    }

    void WriteUninstallKey(const fs::path& dir)
    {
        HKEY key = nullptr;
        if (RegCreateKeyExW(
                HKEY_CURRENT_USER,
                kUninstallKey,
                0,
                nullptr,
                0,
                KEY_WRITE,
                nullptr,
                &key,
                nullptr) != ERROR_SUCCESS)
        {
            return;
        }

        std::wstring exe = (dir / L"MediaTags.exe").wstring();
        std::wstring uninstall = L"\"" + exe + L"\" --uninstall";
        std::wstring display = kAppName;
        std::wstring publisher = L"Media Tags";
        std::wstring version = L"1.0.6";
        DWORD noModify = 1;

        auto setSz = [&](const wchar_t* name, const std::wstring& value)
        {
            RegSetValueExW(
                key,
                name,
                0,
                REG_SZ,
                reinterpret_cast<const BYTE*>(value.c_str()),
                static_cast<DWORD>((value.size() + 1) * sizeof(wchar_t)));
        };

        setSz(L"DisplayName", display);
        setSz(L"Publisher", publisher);
        setSz(L"DisplayVersion", version);
        setSz(L"InstallLocation", dir.wstring());
        setSz(L"DisplayIcon", exe);
        setSz(L"UninstallString", uninstall);

        RegSetValueExW(
            key,
            L"NoModify",
            0,
            REG_DWORD,
            reinterpret_cast<const BYTE*>(&noModify),
            sizeof(noModify));
        RegSetValueExW(
            key,
            L"NoRepair",
            0,
            REG_DWORD,
            reinterpret_cast<const BYTE*>(&noModify),
            sizeof(noModify));

        RegCloseKey(key);
    }

    void DeleteUninstallKey()
    {
        RegDeleteTreeW(HKEY_CURRENT_USER, kUninstallKey);
    }

    bool SetKeyValue(
        const std::wstring& subkey,
        const wchar_t* name,
        const std::wstring& value)
    {
        HKEY key = nullptr;
        if (RegCreateKeyExW(
                HKEY_CURRENT_USER,
                subkey.c_str(),
                0,
                nullptr,
                0,
                KEY_WRITE,
                nullptr,
                &key,
                nullptr) != ERROR_SUCCESS)
        {
            return false;
        }

        const LONG result = RegSetValueExW(
            key,
            name,
            0,
            REG_SZ,
            reinterpret_cast<const BYTE*>(value.c_str()),
            static_cast<DWORD>((value.size() + 1) * sizeof(wchar_t)));

        RegCloseKey(key);
        return result == ERROR_SUCCESS;
    }

    void DeleteClassicVerbs(const wchar_t* const* extensions, size_t count)
    {
        for (size_t i = 0; i < count; ++i)
        {
            const std::wstring verb =
                std::wstring(
                    L"Software\\Classes\\SystemFileAssociations\\") +
                extensions[i] +
                L"\\shell\\MediaTags";

            RegDeleteTreeW(HKEY_CURRENT_USER, verb.c_str());
        }
    }

    void RegisterClassicMenu(const fs::path& dir)
    {
        const std::wstring clsid =
            L"{7D3B1F20-6F62-4B3A-9162-1842775210A1}";
        const std::wstring dll =
            (dir / L"MediaTagsShell.dll").wstring();
        const std::wstring exe =
            (dir / L"MediaTags.exe").wstring();
        const std::wstring clsidKey =
            L"Software\\Classes\\CLSID\\" + clsid;
        const std::wstring inproc = clsidKey + L"\\InProcServer32";

        SetKeyValue(clsidKey, nullptr, L"Media Tags");
        SetKeyValue(inproc, nullptr, dll);
        SetKeyValue(inproc, L"ThreadingModel", L"Apartment");

        for (const wchar_t* ext : kMediaTagExtensions)
        {
            const std::wstring verb =
                std::wstring(
                    L"Software\\Classes\\SystemFileAssociations\\") +
                ext +
                L"\\shell\\MediaTags";

            SetKeyValue(verb, nullptr, L"Gestionar tags");
            SetKeyValue(verb, L"MUIVerb", L"Gestionar tags");
            SetKeyValue(verb, L"Icon", exe);
            SetKeyValue(verb, L"ExplorerCommandHandler", clsid);
            SetKeyValue(verb, L"MultiSelectModel", L"Player");
        }

        DeleteClassicVerbs(
            kRetiredMediaTagExtensions,
            std::size(kRetiredMediaTagExtensions));

        SHChangeNotify(
            SHCNE_ASSOCCHANGED,
            SHCNF_IDLIST,
            nullptr,
            nullptr);
    }

    void UnregisterClassicMenu()
    {
        const std::wstring clsid =
            L"{7D3B1F20-6F62-4B3A-9162-1842775210A1}";

        RegDeleteTreeW(
            HKEY_CURRENT_USER,
            (L"Software\\Classes\\CLSID\\" + clsid).c_str());

        DeleteClassicVerbs(
            kMediaTagExtensions,
            std::size(kMediaTagExtensions));
        DeleteClassicVerbs(
            kRetiredMediaTagExtensions,
            std::size(kRetiredMediaTagExtensions));

        SHChangeNotify(
            SHCNE_ASSOCCHANGED,
            SHCNF_IDLIST,
            nullptr,
            nullptr);
    }

    int RegisterPackage(const fs::path& dir, bool silent)
    {
        if (!ImportCertificate(dir / L"MediaTags.cer"))
        {
            Show(
                L"No se pudo confiar el certificado de Media Tags.",
                silent,
                MB_ICONERROR);
            return 1;
        }

        EnableSideloading();
        RemoveExistingPackage();
        EnsureWinRt();

        ComPtr<IInspectable> inspectable;
        HRESULT hr = RoActivateInstance(
            HStringReference(
                RuntimeClass_Windows_Management_Deployment_PackageManager)
                .Get(),
            &inspectable);
        if (FAILED(hr))
        {
            Show(
                L"No se pudo iniciar el registro de Media Tags.",
                silent,
                MB_ICONERROR);
            return static_cast<int>(hr);
        }

        ComPtr<ABI::Windows::Management::Deployment::IPackageManager9> manager;
        hr = inspectable.As(&manager);
        if (FAILED(hr))
        {
            Show(
                L"Este Windows no admite el registro del men\u00FA moderno.",
                silent,
                MB_ICONERROR);
            return static_cast<int>(hr);
        }

        ComPtr<IInspectable> optionsInspectable;
        hr = RoActivateInstance(
            HStringReference(
                RuntimeClass_Windows_Management_Deployment_AddPackageOptions)
                .Get(),
            &optionsInspectable);
        if (FAILED(hr))
            return static_cast<int>(hr);

        ComPtr<ABI::Windows::Management::Deployment::IAddPackageOptions> options;
        hr = optionsInspectable.As(&options);
        if (FAILED(hr))
            return static_cast<int>(hr);

        const std::wstring location = FileUri(dir);
        const std::wstring msix = FileUri(dir / L"MediaTags.msix");

        ComPtr<ABI::Windows::Foundation::IUriRuntimeClass> locationUri;
        ComPtr<ABI::Windows::Foundation::IUriRuntimeClass> packageUri;
        hr = CreateUri(location, locationUri);
        if (SUCCEEDED(hr))
            hr = CreateUri(msix, packageUri);
        if (FAILED(hr))
            return static_cast<int>(hr);

        hr = options->put_ExternalLocationUri(locationUri.Get());
        if (FAILED(hr))
            return static_cast<int>(hr);

        ComPtr<ABI::Windows::Foundation::IAsyncOperationWithProgress<
            ABI::Windows::Management::Deployment::DeploymentResult*,
            ABI::Windows::Management::Deployment::DeploymentProgress>>
            operation;
        hr = manager->AddPackageByUriAsync(
            packageUri.Get(),
            options.Get(),
            &operation);
        if (FAILED(hr) || !operation)
        {
            Show(
                L"No se pudo registrar Media Tags.",
                silent,
                MB_ICONERROR);
            return static_cast<int>(hr);
        }

        ComPtr<ABI::Windows::Foundation::IAsyncInfo> info;
        hr = operation.As(&info);
        if (SUCCEEDED(hr))
            hr = WaitAsync(info.Get());

        ComPtr<ABI::Windows::Management::Deployment::IDeploymentResult> result;
        if (SUCCEEDED(hr))
            hr = operation->GetResults(&result);

        HRESULT extended = hr;
        std::wstring details;
        if (result)
        {
            result->get_ExtendedErrorCode(&extended);
            HSTRING text{};
            if (SUCCEEDED(result->get_ErrorText(&text)) && text)
            {
                UINT32 length = 0;
                const wchar_t* raw = WindowsGetStringRawBuffer(text, &length);
                if (raw)
                    details.assign(raw, length);
                WindowsDeleteString(text);
            }
        }

        if (FAILED(extended))
        {
            std::wstring message = L"No se pudo registrar Media Tags.";
            if (!details.empty())
                message += L"\n\n" + details;
            Show(message.c_str(), silent, MB_ICONERROR);
            return static_cast<int>(extended);
        }

        return 0;
    }

    void ScheduleDelete(const fs::path& dir)
    {
        if (dir.empty() || !fs::exists(dir))
            return;

        std::error_code ec;
        for (const auto& entry : fs::recursive_directory_iterator(dir, ec))
        {
            if (!ec)
            {
                MoveFileExW(
                    entry.path().c_str(),
                    nullptr,
                    MOVEFILE_DELAY_UNTIL_REBOOT);
            }
        }

        MoveFileExW(
            dir.c_str(),
            nullptr,
            MOVEFILE_DELAY_UNTIL_REBOOT);
    }

    void DeleteInstallFiles(const fs::path& dir)
    {
        if (dir.empty() || !fs::exists(dir))
            return;

        const std::wstring self = ModulePath();
        std::error_code ec;
        std::vector<fs::path> entries;

        for (const auto& entry : fs::directory_iterator(dir, ec))
        {
            if (!ec)
                entries.push_back(entry.path());
        }

        for (const auto& path : entries)
        {
            std::error_code skip;
            if (fs::equivalent(path, fs::path(self), skip) && !skip)
            {
                MoveFileExW(
                    path.c_str(),
                    nullptr,
                    MOVEFILE_DELAY_UNTIL_REBOOT);
                continue;
            }

            fs::remove_all(path, skip);
        }
    }
}

int InstallMediaTags(bool silent)
{
    if (!IsElevated())
        return RelaunchElevated(silent ? L"--install --silent" : L"--install");

    const fs::path source = ModuleDir();
    const fs::path dest = InstallDir();
    if (dest.empty())
    {
        Show(L"No se pudo localizar AppData.", silent, MB_ICONERROR);
        return 1;
    }

    UnregisterClassicMenu();
    RemoveExistingPackage();

    std::wstring error;
    if (!CopyPayload(source, dest, error))
    {
        Show(error.c_str(), silent, MB_ICONERROR);
        return 1;
    }

    const int code = RegisterPackage(dest, silent);
    if (code != 0)
        return code;

    WriteUninstallKey(dest);
    RegisterClassicMenu(dest);
    Show(
        L"Media Tags est\u00E1 listo.\n\n"
        L"Clic derecho en una foto o un v\u00EDdeo: Gestionar tags.",
        silent);

    return 0;
}

int UninstallMediaTags(bool silent)
{
    if (!IsElevated())
        return RelaunchElevated(silent ? L"--uninstall --silent" : L"--uninstall");

    const fs::path dest = InstallDir();
    RemoveExistingPackage();
    UnregisterClassicMenu();
    DeleteUninstallKey();
    DeleteInstallFiles(dest);

    Show(L"Media Tags se ha desinstalado.", silent);

    if (!dest.empty() && fs::exists(dest))
        ScheduleDelete(dest);

    return 0;
}

int PromptMediaTagsSetup()
{
    const fs::path dest = InstallDir();
    const bool leftover =
        !dest.empty() && fs::exists(dest / L"MediaTags.exe");

    const int choice = MessageBoxW(
        nullptr,
        leftover
            ? L"Hay una instalaci\u00F3n anterior de Media Tags.\n\n"
              L"\u00BFQuieres reinstalarla?"
            : L"\u00BFInstalar Media Tags?\n\n"
              L"Se a\u00F1adir\u00E1 Gestionar tags al men\u00FA de fotos y v\u00EDdeos.",
        kAppName,
        MB_OKCANCEL | MB_ICONQUESTION);

    if (choice != IDOK)
        return 0;

    return InstallMediaTags(false);
}
