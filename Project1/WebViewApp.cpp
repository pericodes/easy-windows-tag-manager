#include "WebViewApp.h"
#include "Install.h"
#include "Tags.h"
#include "Resource.h"
#include "../Common/Loc.h"

#include <windows.h>
#include <shobjidl.h>
#include <shlobj.h>

#include <algorithm>
#include <cstdio>
#include <sstream>
#include <set>

namespace
{
    constexpr wchar_t kWebHost[] = L"app.mediatags";
    constexpr UINT WM_APP_TAGS_READY = WM_APP + 1;
}

using Microsoft::WRL::Callback;
using Microsoft::WRL::ComPtr;

WebViewApp* WebViewApp::s_instance = nullptr;

namespace
{
    std::wstring JsonEscape(
        const std::wstring& value)
    {
        std::wstring result;

        for (wchar_t c : value)
        {
            switch (c)
            {
            case L'\\':
                result += L"\\\\";
                break;

            case L'"':
                result += L"\\\"";
                break;

            case L'\n':
                result += L"\\n";
                break;

            case L'\r':
                result += L"\\r";
                break;

            case L'\t':
                result += L"\\t";
                break;

            default:
                result += c;
                break;
            }
        }

        return result;
    }

    std::vector<std::wstring>
    ParseTagsFromJson(
        const std::wstring& json)
    {
        std::vector<std::wstring> result;

        size_t array = json.find(L"\"tags\"");
        if (array == std::wstring::npos)
            return result;

        array = json.find(L'[', array);
        if (array == std::wstring::npos)
            return result;

        size_t pos = array + 1;

        while (true)
        {
            pos = json.find(L"\"", pos);

            if (pos == std::wstring::npos)
                break;

            ++pos;

            std::wstring value;
            bool escaped = false;

            while (pos < json.size())
            {
                wchar_t c = json[pos++];

                if (escaped)
                {
                    value += c;
                    escaped = false;
                    continue;
                }

                if (c == L'\\')
                {
                    escaped = true;
                    continue;
                }

                if (c == L'"')
                    break;

                value += c;
            }

            if (!value.empty())
                result.push_back(value);

            const size_t close = json.find_first_of(L"]", pos);
            const size_t comma = json.find(L",", pos);
            if (close != std::wstring::npos &&
                (comma == std::wstring::npos || close < comma))
            {
                break;
            }
        }

        return result;
    }

    std::wstring ParseJsonStringField(
        const std::wstring& json,
        const std::wstring& key)
    {
        const std::wstring needle =
            L"\"" + key + L"\":";

        size_t pos = json.find(needle);
        if (pos == std::wstring::npos)
            return {};

        pos += needle.size();

        while (pos < json.size() &&
               (json[pos] == L' ' || json[pos] == L'\t'))
        {
            ++pos;
        }

        if (pos >= json.size() || json[pos] != L'"')
            return {};

        ++pos;

        std::wstring value;
        bool escaped = false;

        while (pos < json.size())
        {
            wchar_t c = json[pos++];

            if (escaped)
            {
                value += c;
                escaped = false;
                continue;
            }

            if (c == L'\\')
            {
                escaped = true;
                continue;
            }

            if (c == L'"')
                break;

            value += c;
        }

        return value;
    }

    std::vector<std::wstring>
    Intersection(
        const std::vector<std::vector<std::wstring>>& all)
    {
        if (all.empty())
            return {};

        std::vector<std::wstring> result =
            all.front();

        for (size_t i = 1; i < all.size(); ++i)
        {
            std::vector<std::wstring> next;

            for (const auto& tag : result)
            {
                if (std::find(
                        all[i].begin(),
                        all[i].end(),
                        tag
                    ) != all[i].end())
                {
                    next.push_back(tag);
                }
            }

            result = std::move(next);
        }

        return result;
    }

    void AppendJsonArray(
        std::wstringstream& json,
        const std::vector<std::wstring>& values)
    {
        json << L"[";

        for (size_t i = 0; i < values.size(); ++i)
        {
            if (i)
                json << L",";

            json << L"\""
                 << JsonEscape(values[i])
                 << L"\"";
        }

        json << L"]";
    }

    std::vector<std::wstring>
    OtherTags(
        const std::vector<std::vector<std::wstring>>& all,
        const std::vector<std::wstring>& common)
    {
        std::set<std::wstring> unique;

        for (const auto& tags : all)
        {
            for (const auto& tag : tags)
            {
                if (std::find(
                        common.begin(),
                        common.end(),
                        tag
                    ) == common.end())
                {
                    unique.insert(tag);
                }
            }
        }

        return {
            unique.begin(),
            unique.end()
        };
    }

    std::wstring BuildTagsJson(
        const std::vector<std::wstring>& files)
    {
        std::vector<std::vector<std::wstring>>
            allTags;

        for (const auto& file : files)
        {
            allTags.push_back(
                Tags::Read(file)
            );
        }

        auto common =
            Intersection(allTags);

        auto other =
            OtherTags(allTags, common);

        std::wstringstream json;

        json << L"{\"files\":"
             << files.size()
             << L",\"tags\":";

        AppendJsonArray(json, common);

        json << L",\"other\":";

        AppendJsonArray(json, other);

        json << L",\"lang\":\""
             << LangCode()
             << L"\",\"pref\":\""
             << PrefCode()
             << L"\"}";

        return json.str();
    }
}

WebViewApp::WebViewApp()
{
    s_instance = this;
}

WebViewApp::~WebViewApp()
{
    if (m_tagThread.joinable())
        m_tagThread.join();

    s_instance = nullptr;
}

bool WebViewApp::Create(
    const std::vector<std::wstring>& files)
{
    m_files = files;

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"MediaTagsWindow";
    wc.hCursor = LoadCursorW(
        nullptr,
        IDC_ARROW
    );
    wc.hIcon = static_cast<HICON>(LoadImageW(
        wc.hInstance,
        MAKEINTRESOURCEW(IDI_PROJECT1),
        IMAGE_ICON,
        GetSystemMetrics(SM_CXICON),
        GetSystemMetrics(SM_CYICON),
        0
    ));
    wc.hIconSm = static_cast<HICON>(LoadImageW(
        wc.hInstance,
        MAKEINTRESOURCEW(IDI_SMALL),
        IMAGE_ICON,
        GetSystemMetrics(SM_CXSMICON),
        GetSystemMetrics(SM_CYSMICON),
        0
    ));
    wc.hbrBackground = CreateSolidBrush(
        RGB(0xF7, 0xF7, 0xF7)
    );

    RegisterClassExW(&wc);

    m_window = CreateWindowExW(
        0,
        wc.lpszClassName,
        Loc(Str::ManageTags),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        640,
        720,
        nullptr,
        nullptr,
        wc.hInstance,
        nullptr
    );

    if (!m_window)
        return false;

    StartLoadingTags();
    InitWebView();

    ShowWindow(m_window, SW_SHOWNORMAL);
    UpdateWindow(m_window);
    SetForegroundWindow(m_window);
    BringWindowToTop(m_window);

    return true;
}

void WebViewApp::StartLoadingTags()
{
    m_tagThread = std::thread(
        [this]()
        {
            const HRESULT com =
                CoInitializeEx(
                    nullptr,
                    COINIT_APARTMENTTHREADED
                );

            std::wstring json =
                BuildTagsJson(m_files);

            if (SUCCEEDED(com))
                CoUninitialize();

            m_initialJson = std::move(json);
            m_tagsReady.store(true);

            if (m_window)
            {
                PostMessageW(
                    m_window,
                    WM_APP_TAGS_READY,
                    0,
                    0
                );
            }
        }
    );
}

void WebViewApp::ShowWebViewError(
    const wchar_t* where,
    HRESULT hr)
{
    wchar_t text[512]{};
    swprintf_s(
        text,
        Loc(Str::WebViewErrorFormat),
        where,
        static_cast<unsigned>(hr)
    );

    MessageBoxW(
        m_window,
        text,
        L"Media Tags",
        MB_OK | MB_ICONERROR
    );
}

void WebViewApp::NavigateToUi()
{
    if (!m_webview)
        return;

    ComPtr<ICoreWebView2_3> webview3;
    if (SUCCEEDED(m_webview.As(&webview3)) &&
        !m_webDir.empty())
    {
        const HRESULT mapped =
            webview3->SetVirtualHostNameToFolderMapping(
                kWebHost,
                m_webDir.c_str(),
                COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND_ALLOW
            );

        if (SUCCEEDED(mapped))
        {
            const std::wstring url =
                UiUrl(true);

            m_webview->Navigate(url.c_str());
            return;
        }
    }

    const std::wstring url =
        UiUrl(false);

    m_webview->Navigate(url.c_str());
}

std::wstring WebViewApp::UiUrl(
    bool virtualHost)
{
    std::wstring url;

    if (virtualHost)
    {
        url = L"https://";
        url += kWebHost;
        url += L"/index.html";
    }
    else
    {
        url = L"file:///" + m_webDir + L"/index.html";
        std::replace(
            url.begin(),
            url.end(),
            L'\\',
            L'/'
        );
    }

    url += L"?lang=";
    url += LangCode();
    url += L"&pref=";
    url += PrefCode();
    return url;
}

void WebViewApp::InitWebView()
{
    PWSTR localAppData = nullptr;
    std::wstring userData;
    if (SUCCEEDED(SHGetKnownFolderPath(
            FOLDERID_LocalAppData,
            KF_FLAG_DEFAULT,
            nullptr,
            &localAppData)))
    {
        userData = localAppData;
        userData += L"\\MediaTags\\WebView2";
        CreateDirectoryW((std::wstring(localAppData) + L"\\MediaTags").c_str(), nullptr);
        CreateDirectoryW(userData.c_str(), nullptr);
        CoTaskMemFree(localAppData);
    }

    wchar_t exePath[MAX_PATH]{};
    GetModuleFileNameW(
        nullptr,
        exePath,
        MAX_PATH
    );

    std::wstring base(exePath);
    size_t slash = base.find_last_of(L"\\/");
    if (slash != std::wstring::npos)
        base.resize(slash);

    m_webDir = base + L"\\Web";
    const std::wstring html =
        m_webDir + L"\\index.html";

    if (GetFileAttributesW(html.c_str()) ==
        INVALID_FILE_ATTRIBUTES)
    {
        ShowWebViewError(
            Loc(Str::WebViewHtmlMissing),
            HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)
        );
        return;
    }

    const HRESULT created =
        CreateCoreWebView2EnvironmentWithOptions(
            nullptr,
            userData.empty() ? nullptr : userData.c_str(),
            nullptr,

            Callback<
                ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
                [this](
                    HRESULT result,
                    ICoreWebView2Environment* env
                ) -> HRESULT
                {
                    if (FAILED(result) || !env)
                    {
                        ShowWebViewError(
                            Loc(Str::WebViewStartFailed),
                            result
                        );
                        return result;
                    }

                    m_environment = env;

                    return env->CreateCoreWebView2Controller(
                        m_window,

                        Callback<
                            ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                            [this](
                                HRESULT result,
                                ICoreWebView2Controller* controller
                            ) -> HRESULT
                            {
                                if (FAILED(result) || !controller)
                                {
                                    ShowWebViewError(
                                        Loc(Str::WebViewControllerFailed),
                                        result
                                    );
                                    return result;
                                }

                                m_controller = controller;

                                ComPtr<ICoreWebView2Controller2> controller2;
                                if (SUCCEEDED(m_controller.As(&controller2)))
                                {
                                    const COREWEBVIEW2_COLOR color{
                                        255, 0xF7, 0xF7, 0xF7
                                    };
                                    controller2->put_DefaultBackgroundColor(
                                        color
                                    );
                                }

                                m_controller->get_CoreWebView2(
                                    &m_webview
                                );

                                if (!m_webview)
                                {
                                    ShowWebViewError(
                                        Loc(Str::WebViewGetFailed),
                                        E_FAIL
                                    );
                                    return E_FAIL;
                                }

                                RECT bounds{};
                                GetClientRect(
                                    m_window,
                                    &bounds
                                );

                                m_controller->put_Bounds(bounds);
                                m_controller->put_IsVisible(TRUE);

                                m_webview->add_WebMessageReceived(
                                    Callback<
                                        ICoreWebView2WebMessageReceivedEventHandler>(
                                        [this](
                                            ICoreWebView2*,
                                            ICoreWebView2WebMessageReceivedEventArgs* args
                                        ) -> HRESULT
                                        {
                                            LPWSTR message = nullptr;

                                            if (SUCCEEDED(
                                                args->get_WebMessageAsJson(
                                                    &message)))
                                            {
                                                OnMessage(message);
                                                CoTaskMemFree(message);
                                            }

                                            return S_OK;
                                        }
                                    ).Get(),
                                    nullptr
                                );

                                m_webview->add_NavigationCompleted(
                                    Callback<
                                        ICoreWebView2NavigationCompletedEventHandler>(
                                        [this](
                                            ICoreWebView2* sender,
                                            ICoreWebView2NavigationCompletedEventArgs* args
                                        ) -> HRESULT
                                        {
                                            BOOL ok = FALSE;
                                            if (args)
                                                args->get_IsSuccess(&ok);

                                            LPWSTR uri = nullptr;
                                            if (sender)
                                                sender->get_Source(&uri);

                                            const bool isUi =
                                                uri &&
                                                (wcsstr(uri, kWebHost) ||
                                                 wcsstr(uri, L"index.html"));

                                            if (uri)
                                                CoTaskMemFree(uri);

                                            if (!isUi)
                                                return S_OK;

                                            if (ok)
                                            {
                                                m_pageReady.store(true);
                                                TrySendInitialData();
                                                return S_OK;
                                            }

                                            COREWEBVIEW2_WEB_ERROR_STATUS status{};
                                            if (args)
                                                args->get_WebErrorStatus(&status);

                                            wchar_t where[128]{};
                                            swprintf_s(
                                                where,
                                                Loc(Str::WebViewLoadFailed),
                                                static_cast<int>(status)
                                            );
                                            ShowWebViewError(where, E_FAIL);
                                            return S_OK;
                                        }
                                    ).Get(),
                                    nullptr
                                );

                                NavigateToUi();
                                return S_OK;
                            }
                        ).Get()
                    );
                }
            ).Get()
        );

    if (FAILED(created))
    {
        ShowWebViewError(
            Loc(Str::WebViewStartFailed),
            created
        );
    }
}

void WebViewApp::SendInitialData()
{
    if (!m_webview)
        return;

    const std::wstring json =
        BuildTagsJson(m_files);

    m_webview->PostWebMessageAsJson(
        json.c_str()
    );
}

void WebViewApp::TrySendInitialData()
{
    if (!m_webview ||
        !m_pageReady.load() ||
        !m_tagsReady.load())
    {
        return;
    }

    if (m_initialSent.exchange(true))
        return;

    m_webview->PostWebMessageAsJson(
        m_initialJson.c_str()
    );
}

void WebViewApp::OnMessage(
    const std::wstring& json)
{
    if (json.find(
            L"\"action\":\"setLang\"")
        != std::wstring::npos)
    {
        auto pref =
            ParseJsonStringField(json, L"lang");

        SetAppLang(ParseLang(pref.c_str()));

        if (m_window)
            SetWindowTextW(m_window, Loc(Str::ManageTags));

        UpdateLocalizedShellVerbs();
        SendInitialData();
        return;
    }

    if (json.find(
            L"\"action\":\"rename\"")
        != std::wstring::npos)
    {
        auto from =
            ParseJsonStringField(json, L"from");

        auto to =
            ParseJsonStringField(json, L"to");

        if (!from.empty() && !to.empty())
        {
            for (const auto& file : m_files)
            {
                Tags::Rename(
                    file,
                    from,
                    to
                );
            }
        }

        SendInitialData();
        return;
    }

    if (json.find(
            L"\"action\":\"delete\"")
        != std::wstring::npos)
    {
        auto tags =
            ParseTagsFromJson(json);

        for (const auto& file : m_files)
        {
            Tags::Remove(
                file,
                tags
            );
        }

        SendInitialData();
        return;
    }

    if (json.find(
            L"\"action\":\"add\"")
        != std::wstring::npos)
    {
        auto tags =
            ParseTagsFromJson(json);

        if (!tags.empty())
        {
            for (const auto& file : m_files)
            {
                Tags::Add(
                    file,
                    tags
                );
            }
        }

        SendInitialData();
    }
}

void WebViewApp::Run()
{
    MSG msg{};

    while (GetMessageW(
        &msg,
        nullptr,
        0,
        0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

LRESULT CALLBACK
WebViewApp::WndProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    if (message == WM_APP_TAGS_READY)
    {
        if (s_instance)
            s_instance->TrySendInitialData();

        return 0;
    }

    if (message == WM_SIZE)
    {
        if (s_instance &&
            s_instance->m_controller)
        {
            RECT bounds;

            GetClientRect(
                hwnd,
                &bounds
            );

            s_instance->m_controller->put_Bounds(
                bounds
            );
        }

        return 0;
    }

    if (message == WM_DESTROY)
    {
        PostQuitMessage(0);
        return 0;
    }

    if (message == WM_NCDESTROY)
    {
        return DefWindowProcW(
            hwnd,
            message,
            wParam,
            lParam
        );
    }

    return DefWindowProcW(
        hwnd,
        message,
        wParam,
        lParam
    );
}
