#pragma once

#include <windows.h>
#include <shobjidl.h>

#include <string>
#include <vector>

class ExplorerCommand final
    : public IExplorerCommand
{
public:

    ExplorerCommand();

    // IUnknown

    HRESULT STDMETHODCALLTYPE QueryInterface(
        REFIID riid,
        void** ppv
    ) override;

    ULONG STDMETHODCALLTYPE AddRef()
        override;

    ULONG STDMETHODCALLTYPE Release()
        override;

    // IExplorerCommand

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

private:

    ~ExplorerCommand();

    volatile LONG m_ref = 1;
};
