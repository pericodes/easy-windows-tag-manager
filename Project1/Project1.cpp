#include <windows.h>
#include <shellapi.h>

#include "WebViewApp.h"
#include "Install.h"
#include "../Common/Loc.h"

#include <fstream>
#include <string>
#include <vector>

std::wstring ReadUtf8File(
    const std::wstring& file)
{
    std::ifstream input(file, std::ios::binary);

    if (!input)
        return L"";

    std::string data(
        (std::istreambuf_iterator<char>(input)),
        std::istreambuf_iterator<char>()
    );

    if (data.empty())
        return L"";

    int size = MultiByteToWideChar(
        CP_UTF8,
        0,
        data.data(),
        static_cast<int>(data.size()),
        nullptr,
        0
    );

    std::wstring result(size, L'\0');

    MultiByteToWideChar(
        CP_UTF8,
        0,
        data.data(),
        static_cast<int>(data.size()),
        result.data(),
        size
    );

    return result;
}

std::vector<std::wstring> LoadSelection(
    const std::wstring& file)
{
    std::vector<std::wstring> result;

    std::wstring content =
        ReadUtf8File(file);

    size_t start = 0;

    while (start < content.size())
    {
        size_t end =
            content.find(L'\n', start);

        if (end == std::wstring::npos)
            end = content.size();

        std::wstring line =
            content.substr(
                start,
                end - start
            );

        if (!line.empty() &&
            line.back() == L'\r')
        {
            line.pop_back();
        }

        if (!line.empty())
            result.push_back(line);

        start = end + 1;
    }

    return result;
}

int WINAPI wWinMain(
    HINSTANCE,
    HINSTANCE,
    PWSTR,
    int)
{
    CoInitializeEx(
        nullptr,
        COINIT_APARTMENTTHREADED
    );

    int argc = 0;

    LPWSTR* argv =
        CommandLineToArgvW(
            GetCommandLineW(),
            &argc
        );

    std::wstring selectionFile;
    bool install = false;
    bool uninstall = false;
    bool silent = false;

    for (int i = 1; i < argc; ++i)
    {
        if (wcscmp(argv[i], L"--selection") == 0 && i + 1 < argc)
        {
            selectionFile = argv[++i];
        }
        else if (wcscmp(argv[i], L"--install") == 0)
        {
            install = true;
        }
        else if (wcscmp(argv[i], L"--uninstall") == 0)
        {
            uninstall = true;
        }
        else if (wcscmp(argv[i], L"--silent") == 0)
        {
            silent = true;
        }
    }

    if (argv)
        LocalFree(argv);

    if (install)
    {
        const int code = InstallMediaTags(silent);
        CoUninitialize();
        return code;
    }

    if (uninstall)
    {
        const int code = UninstallMediaTags(silent);
        CoUninitialize();
        return code;
    }

    if (selectionFile.empty())
    {
        const int code = PromptMediaTagsSetup();
        CoUninitialize();
        return code;
    }

    auto files =
        LoadSelection(selectionFile);

    if (files.empty())
    {
        MessageBoxW(
            nullptr,
            Loc(Str::CouldNotReadSelection),
            L"Media Tags",
            MB_OK | MB_ICONERROR);
        CoUninitialize();
        return 1;
    }

    WebViewApp app;

    if (!app.Create(files))
    {
        MessageBoxW(
            nullptr,
            Loc(Str::CouldNotOpenWindow),
            L"Media Tags",
            MB_OK | MB_ICONERROR);
        CoUninitialize();
        return 1;
    }

    app.Run();

    CoUninitialize();

    DeleteFileW(selectionFile.c_str());

    return 0;
}
