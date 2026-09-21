#pragma once

#include <windows.h>
#include <shobjidl.h>
#include <ocidl.h>

class ExplorerCommand final
    : public IExplorerCommand
    , public IObjectWithSite
    , public IObjectWithSelection
{
public:

    ExplorerCommand();

    HRESULT STDMETHODCALLTYPE QueryInterface(
        REFIID riid,
        void** ppv
    ) override;

    ULONG STDMETHODCALLTYPE AddRef() override;
    ULONG STDMETHODCALLTYPE Release() override;

    HRESULT STDMETHODCALLTYPE GetTitle(
        IShellItemArray* items,
        PWSTR* title
    ) override;

    HRESULT STDMETHODCALLTYPE GetIcon(
        IShellItemArray* items,
        PWSTR* icon
    ) override;

    HRESULT STDMETHODCALLTYPE GetToolTip(
        IShellItemArray* items,
        PWSTR* tooltip
    ) override;

    HRESULT STDMETHODCALLTYPE GetCanonicalName(
        GUID* guid
    ) override;

    HRESULT STDMETHODCALLTYPE GetState(
        IShellItemArray* items,
        BOOL okToBeSlow,
        EXPCMDSTATE* state
    ) override;

    HRESULT STDMETHODCALLTYPE Invoke(
        IShellItemArray* items,
        IBindCtx* bindCtx
    ) override;

    HRESULT STDMETHODCALLTYPE GetFlags(
        EXPCMDFLAGS* flags
    ) override;

    HRESULT STDMETHODCALLTYPE EnumSubCommands(
        IEnumExplorerCommand** enumCommands
    ) override;

    HRESULT STDMETHODCALLTYPE SetSite(
        IUnknown* site
    ) override;

    HRESULT STDMETHODCALLTYPE GetSite(
        REFIID riid,
        void** ppv
    ) override;

    HRESULT STDMETHODCALLTYPE SetSelection(
        IShellItemArray* items
    ) override;

    HRESULT STDMETHODCALLTYPE GetSelection(
        REFIID riid,
        void** ppv
    ) override;

private:

    ~ExplorerCommand();

    volatile LONG m_ref = 1;
    IUnknown* m_site = nullptr;
    IShellItemArray* m_selection = nullptr;
};
