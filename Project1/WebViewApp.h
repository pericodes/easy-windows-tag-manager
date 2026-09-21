#pragma once

#include <windows.h>
#include <wrl.h>
#include <WebView2.h>

#include <string>
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

    void InitWebView();

    void OnMessage(
        const std::wstring& json
    );

    void SendInitialData();

    static LRESULT CALLBACK WndProc(
        HWND,
        UINT,
        WPARAM,
        LPARAM
    );

    static WebViewApp* s_instance;
};
