# Media Tags

**English** · [Español](#español)

Windows 10/11 app that adds **Manage tags** to the context menu of photos and videos. Tags are stored in the file properties (`System.Keywords`), the same ones File Explorer shows under Details.

**Images:** `.jpg`, `.jpeg`, `.jpe`, `.jfif`, `.png`, `.tif`, `.tiff`, `.avif`

**Videos:** `.mp4`, `.m4v`, `.mov`, `.wmv`, `.asf`, `.3gp`, `.3g2`

Only formats where Windows itself persists `System.Keywords` (Explorer Tags). `.webp`, `.gif`, `.bmp`, `.mkv`, `.avi` and similar are omitted: the OS property handler refuses to write tags (`STG_E_ACCESSDENIED` / not writable).

## Usage

1. Right-click one or more photos/videos.
2. **Manage tags** (on Windows 11, if it is missing, *Show more options*).
3. Add or remove tags. **Common tags** is the intersection across the selected files.

Tags stay on the file: they show up in Explorer, on other PCs, and in apps that read Windows keywords.

## Installation

Download **MediaTagsSetup** from the [latest GitHub Release](https://github.com/pericodes/easy-windows-tag-manager/releases/latest) and run it (UAC prompt). You do not need to compile the project.

Requirements:

- Windows 10 (19041+) or Windows 11, **64-bit**
- [WebView2 Runtime](https://developer.microsoft.com/microsoft-edge/webview2/) (usually already installed with Edge)

Do not use a loose `MediaTags.exe` from a build folder: Explorer loads the copy installed in `%LOCALAPPDATA%\MediaTags`.

Uninstall from *Settings → Apps*, or run the same installer with `/uninstall`.

If Windows Defender flags the installer (`Trojan:Win32/Bearfoos.Alml`), it is a heuristic **false positive** (self-signed installer). Restore or allow the file.

## Build

You need Visual Studio 2022/2026 with Desktop C++ development, the Windows SDK (MakeAppx and SignTool), and NuGet (`Microsoft.Web.WebView2`).

Open **`MediaTags.slnx`** and build **x64** (Win32/x86 will not work: Explorer is x64).

```bat
msbuild MediaTags.slnx /p:Configuration=Debug /p:Platform=x64
```

The deliverable is **`MediaTagsSetup.exe`**. After changing code, install it again; otherwise Explorer keeps using the old DLL from AppData. If the menu does not update, restart the *Windows Explorer* process.

If linking fails with LNK1168, close `MediaTags.exe` (it is sometimes elevated by UAC) and rebuild.

To publish a new downloadable installer, push a tag `vX.Y.Z` (for package version `X.Y.Z.0`). GitHub Actions builds Release x64 and attaches `MediaTagsSetup-X.Y.Z.exe` to the release.

## Layout

| Project | Output |
|---|---|
| `Dll/` | `MediaTagsShell.dll` — context-menu verb (`IExplorerCommand`) |
| `Project1/` | `MediaTags.exe` — WebView2 UI and the actual installer |
| `Setup/` | `MediaTagsSetup.exe` — self-extracting stub with UAC |
| `Package/` | Sparse MSIX manifest (package identity required by Windows 11) |

The modern Windows 11 menu is not registered with `regsvr32`. The installer registers a sparse MSIX package and, as a fallback, classic HKCU verbs for *Show more options*.

`MediaTagsManager/` and `_MediaTagsManager/` are old copies; the live code is `Dll/`, `Project1/`, `Setup/`, and `Package/`.

Implementation details, CLSID, and known pitfalls are in [AGENTS.md](AGENTS.md).

---

# Español

[English](#media-tags) · **Español**

App de Windows 10/11 que añade **Gestionar tags** al menú contextual de fotos y vídeos. Los tags se guardan en las propiedades del archivo (`System.Keywords`), las mismas que muestra el Explorador en Detalles.

**Imágenes:** `.jpg`, `.jpeg`, `.jpe`, `.jfif`, `.png`, `.tif`, `.tiff`, `.avif`

**Vídeos:** `.mp4`, `.m4v`, `.mov`, `.wmv`, `.asf`, `.3gp`, `.3g2`

Solo formatos en los que Windows persiste `System.Keywords` (Tags del Explorador). `.webp`, `.gif`, `.bmp`, `.mkv`, `.avi` y similares no están: el handler del sistema no deja escribir tags (`STG_E_ACCESSDENIED` / no escribible).

## Uso

1. Clic derecho en una o varias fotos/vídeos.
2. **Gestionar tags** (en Windows 11, si no aparece, *Mostrar más opciones*).
3. Añade o elimina tags. La lista de **tags comunes** es la intersección de los archivos seleccionados.

Los tags quedan en el archivo: se ven en el Explorador, en otras PCs y en programas que lean keywords de Windows.

## Instalación

Descarga **MediaTagsSetup** de la [última release de GitHub](https://github.com/pericodes/easy-windows-tag-manager/releases/latest) y ejecútalo (pide UAC). No hace falta compilar el proyecto.

Requisitos:

- Windows 10 (19041+) o Windows 11, **64 bits**
- [WebView2 Runtime](https://developer.microsoft.com/microsoft-edge/webview2/) (suele venir con Edge)

No uses `MediaTags.exe` suelto de una carpeta de compilación: el menú del Explorador carga la copia instalada en `%LOCALAPPDATA%\MediaTags`.

Desinstalar: *Configuración → Aplicaciones*, o el mismo instalador con `/uninstall`.

Si Windows Defender marca el instalador (`Trojan:Win32/Bearfoos.Alml`), es un **falso positivo** heurístico (instalador autofirmado). Restaura o permite el archivo.

## Compilar

Necesitas Visual Studio 2022/2026 con desarrollo de escritorio C++, Windows SDK (MakeAppx y SignTool) y NuGet (`Microsoft.Web.WebView2`).

Abre **`MediaTags.slnx`** y compila **x64** (Win32/x86 no sirve: el Explorador es x64).

```bat
msbuild MediaTags.slnx /p:Configuration=Debug /p:Platform=x64
```

El entregable es **`MediaTagsSetup.exe`**. Tras cambiar código, vuelve a instalarlo; si no, el Explorador sigue usando la DLL vieja de AppData. Si el menú no se actualiza, reinicia el proceso *Explorador de Windows*.

Si el link falla con LNK1168, cierra `MediaTags.exe` (a veces está elevado por UAC) y recompila.

Para publicar un instalador descargable, empuja un tag `vX.Y.Z` (versión de paquete `X.Y.Z.0`). GitHub Actions compila Release x64 y adjunta `MediaTagsSetup-X.Y.Z.exe` a la release.

## Estructura

| Proyecto | Qué genera |
|---|---|
| `Dll/` | `MediaTagsShell.dll` — verbo del menú (`IExplorerCommand`) |
| `Project1/` | `MediaTags.exe` — UI WebView2 e instalador real |
| `Setup/` | `MediaTagsSetup.exe` — autoextraíble con UAC |
| `Package/` | Manifiesto MSIX sparse (identidad de paquete que exige Windows 11) |

El menú moderno de Windows 11 no se registra con `regsvr32`. El instalador registra un paquete MSIX sparse y, de respaldo, verbos clásicos en HKCU para *Mostrar más opciones*.

`MediaTagsManager/` y `_MediaTagsManager/` son copias viejas; el código activo es `Dll/`, `Project1/`, `Setup/` y `Package/`.

Detalles de implementación, CLSID y trampas ya vistas están en [AGENTS.md](AGENTS.md).
