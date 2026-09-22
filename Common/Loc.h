#pragma once

#include <windows.h>
#include <string.h>

enum class AppLang
{
    Auto,
    En,
    Es
};

enum class Str
{
    ManageTags,
    ManageTagsTooltip,
    CouldNotOpenApp,
    CouldNotReadSelection,
    CouldNotOpenWindow,
    WebViewErrorFormat,
    WebViewHtmlMissing,
    WebViewStartFailed,
    WebViewControllerFailed,
    WebViewGetFailed,
    WebViewLoadFailed,
    CouldNotCreatePrefix,
    MissingFilePrefix,
    MissingFileSuffix,
    CouldNotUpdatePrefix,
    CouldNotUpdateSuffix,
    MissingWebFolder,
    CouldNotCopyWeb,
    CouldNotTrustCert,
    CouldNotStartRegistration,
    WindowsNoModernMenu,
    CouldNotRegister,
    CouldNotFindAppData,
    InstallReady,
    Uninstalled,
    PromptReinstall,
    PromptInstall,
    SetupMissingFiles,
    SetupDidNotFinish
};

namespace loc_detail
{
    constexpr wchar_t kKey[] = L"Software\\MediaTags";
    constexpr wchar_t kValue[] = L"Language";

    inline const wchar_t* En(Str id)
    {
        switch (id)
        {
        case Str::ManageTags:
            return L"Manage tags";
        case Str::ManageTagsTooltip:
            return L"Add or remove tags";
        case Str::CouldNotOpenApp:
            return L"Could not open Media Tags.";
        case Str::CouldNotReadSelection:
            return L"Could not read the selected files.";
        case Str::CouldNotOpenWindow:
            return L"Could not open the Media Tags window.";
        case Str::WebViewErrorFormat:
            return L"%s\nCode: 0x%08X\n\n"
                   L"Install Microsoft Edge WebView2 Runtime if it is missing.";
        case Str::WebViewHtmlMissing:
            return L"Web\\index.html was not found next to the executable.";
        case Str::WebViewStartFailed:
            return L"Could not start WebView2.";
        case Str::WebViewControllerFailed:
            return L"Could not create the WebView2 view.";
        case Str::WebViewGetFailed:
            return L"Could not get WebView2.";
        case Str::WebViewLoadFailed:
            return L"Could not load the UI (status %d).";
        case Str::CouldNotCreatePrefix:
            return L"Could not create ";
        case Str::MissingFilePrefix:
            return L"Missing ";
        case Str::MissingFileSuffix:
            return L". Build the x64 solution to generate the installer.";
        case Str::CouldNotUpdatePrefix:
            return L"Could not update ";
        case Str::CouldNotUpdateSuffix:
            return L". Close File Explorer and try installing again.";
        case Str::MissingWebFolder:
            return L"The Web folder is missing next to the executable.";
        case Str::CouldNotCopyWeb:
            return L"Could not copy the Web UI.";
        case Str::CouldNotTrustCert:
            return L"Could not trust the Media Tags certificate.";
        case Str::CouldNotStartRegistration:
            return L"Could not start Media Tags registration.";
        case Str::WindowsNoModernMenu:
            return L"This Windows version does not support the modern context menu.";
        case Str::CouldNotRegister:
            return L"Could not register Media Tags.";
        case Str::CouldNotFindAppData:
            return L"Could not locate AppData.";
        case Str::InstallReady:
            return L"Media Tags is ready.\n\n"
                   L"Right-click a photo or video: Manage tags.";
        case Str::Uninstalled:
            return L"Media Tags has been uninstalled.";
        case Str::PromptReinstall:
            return L"A previous Media Tags install was found.\n\n"
                   L"Do you want to reinstall it?";
        case Str::PromptInstall:
            return L"Install Media Tags?\n\n"
                   L"Manage tags will be added to the photo and video menu.";
        case Str::SetupMissingFiles:
            return L"This installer does not contain the Media Tags files.\n"
                   L"Build the x64 solution to generate MediaTagsSetup.exe.";
        case Str::SetupDidNotFinish:
            return L"Setup did not finish. Accept the User Account Control prompt and try again.";
        }

        return L"";
    }

    inline const wchar_t* Es(Str id)
    {
        switch (id)
        {
        case Str::ManageTags:
            return L"Gestionar tags";
        case Str::ManageTagsTooltip:
            return L"A\u00F1adir o eliminar tags";
        case Str::CouldNotOpenApp:
            return L"No se pudo abrir Media Tags.";
        case Str::CouldNotReadSelection:
            return L"No se pudieron leer los archivos seleccionados.";
        case Str::CouldNotOpenWindow:
            return L"No se pudo abrir la ventana de Media Tags.";
        case Str::WebViewErrorFormat:
            return L"%s\nC\u00F3digo: 0x%08X\n\n"
                   L"Instala Microsoft Edge WebView2 Runtime si falta.";
        case Str::WebViewHtmlMissing:
            return L"No se encontr\u00F3 Web\\index.html junto al ejecutable.";
        case Str::WebViewStartFailed:
            return L"No se pudo iniciar WebView2.";
        case Str::WebViewControllerFailed:
            return L"No se pudo crear el visor WebView2.";
        case Str::WebViewGetFailed:
            return L"No se pudo obtener WebView2.";
        case Str::WebViewLoadFailed:
            return L"No se pudo cargar la interfaz (estado %d).";
        case Str::CouldNotCreatePrefix:
            return L"No se pudo crear ";
        case Str::MissingFilePrefix:
            return L"Falta ";
        case Str::MissingFileSuffix:
            return L". Compila la soluci\u00F3n x64 para generar el instalador.";
        case Str::CouldNotUpdatePrefix:
            return L"No se pudo actualizar ";
        case Str::CouldNotUpdateSuffix:
            return L". Cierra el Explorador de archivos y vuelve a instalar.";
        case Str::MissingWebFolder:
            return L"Falta la carpeta Web junto al ejecutable.";
        case Str::CouldNotCopyWeb:
            return L"No se pudo copiar la interfaz Web.";
        case Str::CouldNotTrustCert:
            return L"No se pudo confiar el certificado de Media Tags.";
        case Str::CouldNotStartRegistration:
            return L"No se pudo iniciar el registro de Media Tags.";
        case Str::WindowsNoModernMenu:
            return L"Este Windows no admite el registro del men\u00FA moderno.";
        case Str::CouldNotRegister:
            return L"No se pudo registrar Media Tags.";
        case Str::CouldNotFindAppData:
            return L"No se pudo localizar AppData.";
        case Str::InstallReady:
            return L"Media Tags est\u00E1 listo.\n\n"
                   L"Clic derecho en una foto o un v\u00EDdeo: Gestionar tags.";
        case Str::Uninstalled:
            return L"Media Tags se ha desinstalado.";
        case Str::PromptReinstall:
            return L"Hay una instalaci\u00F3n anterior de Media Tags.\n\n"
                   L"\u00BFQuieres reinstalarla?";
        case Str::PromptInstall:
            return L"\u00BFInstalar Media Tags?\n\n"
                   L"Se a\u00F1adir\u00E1 Gestionar tags al men\u00FA de fotos y v\u00EDdeos.";
        case Str::SetupMissingFiles:
            return L"Este instalador no contiene los archivos de Media Tags.\n"
                   L"Compila la soluci\u00F3n x64 para generar MediaTagsSetup.exe.";
        case Str::SetupDidNotFinish:
            return L"La instalaci\u00F3n no se complet\u00F3. Acepta el control de cuentas de usuario e int\u00E9ntalo de nuevo.";
        }

        return L"";
    }
}

inline AppLang GetPreferredLang()
{
    wchar_t buf[16]{};
    DWORD size = sizeof(buf);
    if (RegGetValueW(
            HKEY_CURRENT_USER,
            loc_detail::kKey,
            loc_detail::kValue,
            RRF_RT_REG_SZ,
            nullptr,
            buf,
            &size) != ERROR_SUCCESS)
    {
        return AppLang::Auto;
    }

    if (_wcsicmp(buf, L"es") == 0)
        return AppLang::Es;

    if (_wcsicmp(buf, L"en") == 0)
        return AppLang::En;

    return AppLang::Auto;
}

inline AppLang EffectiveLang()
{
    const AppLang preferred = GetPreferredLang();
    if (preferred == AppLang::Es)
        return AppLang::Es;

    if (preferred == AppLang::En)
        return AppLang::En;

    return PRIMARYLANGID(GetUserDefaultUILanguage()) == LANG_SPANISH
        ? AppLang::Es
        : AppLang::En;
}

inline const wchar_t* LangCode()
{
    return EffectiveLang() == AppLang::Es ? L"es" : L"en";
}

inline const wchar_t* PrefCode()
{
    switch (GetPreferredLang())
    {
    case AppLang::Es:
        return L"es";
    case AppLang::En:
        return L"en";
    default:
        return L"auto";
    }
}

inline AppLang ParseLang(const wchar_t* value)
{
    if (!value || !value[0])
        return AppLang::Auto;

    if (_wcsicmp(value, L"es") == 0)
        return AppLang::Es;

    if (_wcsicmp(value, L"en") == 0)
        return AppLang::En;

    return AppLang::Auto;
}

inline void SetAppLang(AppLang lang)
{
    const wchar_t* value = L"auto";
    if (lang == AppLang::Es)
        value = L"es";
    else if (lang == AppLang::En)
        value = L"en";

    RegSetKeyValueW(
        HKEY_CURRENT_USER,
        loc_detail::kKey,
        loc_detail::kValue,
        REG_SZ,
        value,
        static_cast<DWORD>((wcslen(value) + 1) * sizeof(wchar_t)));
}

inline const wchar_t* Loc(Str id)
{
    return EffectiveLang() == AppLang::Es
        ? loc_detail::Es(id)
        : loc_detail::En(id);
}
