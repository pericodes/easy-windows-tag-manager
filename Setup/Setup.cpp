#include <windows.h>
#include <shellapi.h>
#include <stdint.h>
#include <string.h>

#include <string>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

namespace
{
    constexpr uint32_t kMagic = 0x4B50544D;

    struct Footer
    {
        uint32_t magic;
        uint32_t zipOffset;
        uint32_t zipSize;
        uint32_t reserved;
    };

    std::wstring ModulePath()
    {
        wchar_t buffer[MAX_PATH]{};
        GetModuleFileNameW(nullptr, buffer, MAX_PATH);
        return buffer;
    }

    int Run(const std::wstring& command)
    {
        std::vector<wchar_t> buffer(command.begin(), command.end());
        buffer.push_back(L'\0');

        STARTUPINFOW si{};
        si.cb = sizeof(si);

        PROCESS_INFORMATION pi{};
        if (!CreateProcessW(
                nullptr,
                buffer.data(),
                nullptr,
                nullptr,
                FALSE,
                0,
                nullptr,
                nullptr,
                &si,
                &pi))
        {
            return static_cast<int>(GetLastError());
        }

        WaitForSingleObject(pi.hProcess, INFINITE);

        DWORD code = 1;
        GetExitCodeProcess(pi.hProcess, &code);
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        return static_cast<int>(code);
    }

    fs::path ExtractPayload()
    {
        const std::wstring self = ModulePath();
        HANDLE file = CreateFileW(
            self.c_str(),
            GENERIC_READ,
            FILE_SHARE_READ,
            nullptr,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            nullptr);

        if (file == INVALID_HANDLE_VALUE)
            return {};

        const DWORD size = GetFileSize(file, nullptr);
        if (size < sizeof(Footer))
        {
            CloseHandle(file);
            return {};
        }

        Footer footer{};
        SetFilePointer(file, -static_cast<LONG>(sizeof(Footer)), nullptr, FILE_END);

        DWORD read = 0;
        ReadFile(file, &footer, sizeof(footer), &read, nullptr);

        fs::path extractDir =
            fs::temp_directory_path() / L"MediaTagsSetupPayload";

        if (footer.magic != kMagic ||
            footer.zipOffset >= size ||
            footer.zipSize == 0)
        {
            CloseHandle(file);

            fs::path sibling = fs::path(self).parent_path() / L"MediaTags.exe";
            if (fs::exists(sibling))
                return sibling.parent_path();

            return {};
        }

        std::vector<char> zip(footer.zipSize);
        SetFilePointer(file, static_cast<LONG>(footer.zipOffset), nullptr, FILE_BEGIN);
        ReadFile(
            file,
            zip.data(),
            footer.zipSize,
            &read,
            nullptr);
        CloseHandle(file);

        std::error_code ec;
        fs::remove_all(extractDir, ec);
        fs::create_directories(extractDir, ec);

        const fs::path zipPath = extractDir / L"payload.zip";
        HANDLE zipFile = CreateFileW(
            zipPath.c_str(),
            GENERIC_WRITE,
            0,
            nullptr,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            nullptr);

        if (zipFile == INVALID_HANDLE_VALUE)
            return {};

        DWORD written = 0;
        WriteFile(zipFile, zip.data(), footer.zipSize, &written, nullptr);
        CloseHandle(zipFile);

        wchar_t windowsDir[MAX_PATH]{};
        GetWindowsDirectoryW(windowsDir, MAX_PATH);
        const fs::path tar =
            fs::path(windowsDir) / L"System32" / L"tar.exe";

        std::wstring command =
            L"\"" + tar.wstring() + L"\" -xf \"" +
            zipPath.wstring() +
            L"\" -C \"" +
            extractDir.wstring() +
            L"\"";

        if (Run(command) != 0)
            return {};

        return extractDir;
    }

    bool HasArg(int argc, LPWSTR* argv, const wchar_t* name)
    {
        for (int i = 1; i < argc; ++i)
        {
            if (_wcsicmp(argv[i], name) == 0)
                return true;
        }

        return false;
    }
}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);

    const bool uninstall = argv && HasArg(argc, argv, L"/uninstall");
    const bool quiet = argv && (
        HasArg(argc, argv, L"/S") ||
        HasArg(argc, argv, L"--silent"));

    if (argv)
        LocalFree(argv);

    if (!uninstall && !quiet)
    {
        const int choice = MessageBoxW(
            nullptr,
            L"\u00BFInstalar Media Tags?\n\n"
            L"Se a\u00F1adir\u00E1 Gestionar tags al men\u00FA de fotos y v\u00EDdeos.",
            L"Media Tags",
            MB_OKCANCEL | MB_ICONQUESTION);

        if (choice != IDOK)
            return 0;
    }

    fs::path payload = ExtractPayload();
    if (payload.empty() || !fs::exists(payload / L"MediaTags.exe"))
    {
        MessageBoxW(
            nullptr,
            L"Este instalador no contiene los archivos de Media Tags.\n"
            L"Compila la soluci\u00F3n x64 para generar MediaTagsSetup.exe.",
            L"Media Tags",
            MB_OK | MB_ICONERROR);
        return 1;
    }

    const wchar_t* args = uninstall
        ? L"--uninstall --silent"
        : L"--install --silent";

    std::wstring command =
        L"\"" + (payload / L"MediaTags.exe").wstring() + L"\" " + args;

    const int code = Run(command);
    if (code != 0)
    {
        MessageBoxW(
            nullptr,
            L"La instalaci\u00F3n no se complet\u00F3. Acepta el control de cuentas de usuario e int\u00E9ntalo de nuevo.",
            L"Media Tags",
            MB_OK | MB_ICONERROR);
        return code;
    }

    if (!quiet)
    {
        MessageBoxW(
            nullptr,
            uninstall
                ? L"Media Tags se ha desinstalado."
                : L"Media Tags est\u00E1 listo.\n\n"
                  L"Clic derecho en una foto o un v\u00EDdeo: Gestionar tags.",
            L"Media Tags",
            MB_OK | MB_ICONINFORMATION);
    }

    return 0;
}
