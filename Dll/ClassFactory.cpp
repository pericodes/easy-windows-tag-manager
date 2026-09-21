#include "pch.h"
#include "ClassFactory.h"
#include "ExplorerCommand.h"

ClassFactory::ClassFactory()
{
}    


ClassFactory::~ClassFactory()
{
}

HRESULT STDMETHODCALLTYPE
ClassFactory::QueryInterface(
    REFIID riid,
    void** ppv)
{
    if (!ppv)
        return E_POINTER;

    *ppv = nullptr;

    if (riid == IID_IUnknown ||
        riid == IID_IClassFactory)
    {
        *ppv =
            static_cast<IClassFactory*>(this);

        AddRef();

        return S_OK;
    }

    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE
ClassFactory::AddRef()
{
    return InterlockedIncrement(
        &m_ref
    );
}

ULONG STDMETHODCALLTYPE
ClassFactory::Release()
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
ClassFactory::CreateInstance(
    IUnknown* outer,
    REFIID riid,
    void** ppv)
{
    if (outer)
        return CLASS_E_NOAGGREGATION;

    ExplorerCommand* command =
        new ExplorerCommand();

    if (!command)
        return E_OUTOFMEMORY;

    HRESULT hr =
        command->QueryInterface(
            riid,
            ppv
        );

    command->Release();

    return hr;
}

HRESULT STDMETHODCALLTYPE
ClassFactory::LockServer(
    BOOL)
{
    return S_OK;
}
