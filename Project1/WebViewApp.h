#pragma once

#include <windows.h>
#include <wrl.h>
#include <WebView2.h>

#include <atomic>
#include <string>
#include <thread>
#include <vector>

class WebViewApp
{
public:

    WebViewApp();
    ~WebViewApp();

    bool Create(
        const std::vector<std::wstring>& files
    );

    void Run();

private:

    HWND m_window = nullptr;

    Microsoft::WRL::ComPtr<
        ICoreWebView2Environment
    > m_environment;

    Microsoft::WRL::ComPtr<
        ICoreWebView2Controller
    > m_controller;

    Microsoft::WRL::ComPtr<
        ICoreWebView2
    > m_webview;

    std::vector<std::wstring> m_files;

    std::wstring m_webDir;
    std::wstring m_initialJson;

    std::thread m_tagThread;
    std::atomic<bool> m_tagsReady{ false };
    std::atomic<bool> m_pageReady{ false };
    std::atomic<bool> m_initialSent{ false };

    void StartLoadingTags();

    void InitWebView();

    void ShowWebViewError(
        const wchar_t* where,
        HRESULT hr
    );

    void NavigateToUi();

    void OnMessage(
        const std::wstring& json
    );

    void SendInitialData();

    void TrySendInitialData();

    std::wstring UiUrl(
        bool virtualHost
    );

    static LRESULT CALLBACK WndProc(
        HWND,
        UINT,
        WPARAM,
        LPARAM
    );

    static WebViewApp* s_instance;
};
