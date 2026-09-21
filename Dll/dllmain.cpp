#include "pch.h"
#include <windows.h>

#include "ClassFactory.h"

static volatile LONG g_objects = 0;

const CLSID CLSID_MediaTags =
{
    0x7d3b1f20,
    0x6f62,
    0x4b3a,
    {0x91, 0x62, 0x18, 0x42,
     0x77, 0x52, 0x10, 0xA1}
};

BOOL APIENTRY DllMain(
    HMODULE,
    DWORD reason,
    LPVOID)
{
    return TRUE;
}

STDAPI DllGetClassObject(
    REFCLSID clsid,
    REFIID iid,
    void** ppv)
{
    if (clsid != CLSID_MediaTags)
        return CLASS_E_CLASSNOTAVAILABLE;

    auto factory =
        new ClassFactory();

    if (!factory)
        return E_OUTOFMEMORY;

    HRESULT hr =
        factory->QueryInterface(
            iid,
            ppv
        );

    factory->Release();

    return hr;
}

STDAPI DllCanUnloadNow()
{
    return g_objects == 0
        ? S_OK
        : S_FALSE;
}
