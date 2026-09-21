#include "WebViewApp.h"

#include "Tags.h"

#include <windows.h>
#include <shobjidl.h>

#include <algorithm>
#include <sstream>
#include <set>

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

        size_t pos = 0;

        while (true)
        {
            pos = json.find(
                L"\"",
                pos
            );

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
        }

        return result;
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
}

WebViewApp::WebViewApp()
{
    s_instance = this;
}

WebViewApp::~WebViewApp()
{
    s_instance = nullptr;
}

bool WebViewApp::Create(
    const std::vector<std::wstring>& files)
{
    m_files = files;

    WNDCLASSW wc{};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"MediaTagsWindow";
    wc.hCursor = LoadCursorW(
        nullptr,
        IDC_ARROW
    );

    RegisterClassW(&wc);

    m_window = CreateWindowExW(
        0,
        wc.lpszClassName,
        L"Gestionar tags",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        600,
        500,
        nullptr,
        nullptr,
        wc.hInstance,
        nullptr
    );

    if (!m_window)
        return false;

    ShowWindow(
        m_window,
        SW_SHOW
    );

    UpdateWindow(m_window);

    InitWebView();

    return true;
}

void WebViewApp::InitWebView()
{
    CreateCoreWebView2EnvironmentWithOptions(
        nullptr,
        nullptr,
        nullptr,

        Callback<
            ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [this](
                HRESULT result,
                ICoreWebView2Environment* env
            ) -> HRESULT
            {
                if (FAILED(result))
                    return result;

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
                            if (FAILED(result))
                                return result;

                            m_controller = controller;

                            m_controller->get_CoreWebView2(
                                &m_webview
                            );

                            RECT bounds;

                            GetClientRect(
                                m_window,
                                &bounds
                            );

                            m_controller->put_Bounds(
                                bounds
                            );

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

                            //m_webview->Navigate(
                            //    L"file:///C:/MediaTags/Web/index.html"
                            //);

                            wchar_t exePath[MAX_PATH]{};

                            GetModuleFileNameW(
                                nullptr,
                                exePath,
                                MAX_PATH
                            );

                            std::wstring base(exePath);

                            size_t slash =
                                base.find_last_of(L"\\/");

                            base.resize(slash);

                            std::wstring html =
                                base +
                                L"\\Web\\index.html";

                                std::wstring uri =
                                L"file:///" + html;
                            
                            std::replace(
                                uri.begin(),
                                uri.end(),
                                L'\\',
                                L'/'
                            );
                            
                            m_webview->Navigate(
                                uri.c_str()
                            );

                            return S_OK;
                        }
                    ).Get()
                );
            }
        ).Get()
    );
}

void WebViewApp::SendInitialData()
{
    std::vector<std::vector<std::wstring>>
        allTags;

    for (const auto& file : m_files)
    {
        allTags.push_back(
            Tags::Read(file)
        );
    }

    auto common =
        Intersection(allTags);

    std::wstringstream json;

    json << L"{\"files\":"
         << m_files.size()
         << L",\"tags\":[";

    for (size_t i = 0;
         i < common.size();
         ++i)
    {
        if (i)
            json << L",";

        json << L"\""
             << JsonEscape(common[i])
             << L"\"";
    }

    json << L"]}";

    m_webview->PostWebMessageAsJson(
        json.str().c_str()
    );
}

void WebViewApp::OnMessage(
    const std::wstring& json)
{
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

        PostMessageW(
            m_window,
            WM_CLOSE,
            0,
            0
        );

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
                    { tags.back() }
                );
            }
        }

        PostMessageW(
            m_window,
            WM_CLOSE,
            0,
            0
        );
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
