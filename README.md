# Media Tags

**English** · [Español](#español)

Windows 10/11 app that adds **Manage tags** to the context menu of photos and videos. Tags are stored in the file properties (`System.Keywords`), the same ones File Explorer shows under Details.

Formats: `.jpg`, `.jpeg`, `.mp4`, and `.mov`.

## Usage

1. Right-click one or more photos/videos.
2. **Manage tags** (on Windows 11, if it is missing, *Show more options*).
3. Add or remove tags. **Common tags** is the intersection across the selected files.

Tags stay on the file: they show up in Explorer, on other PCs, and in apps that read Windows keywords.

## Installation

Requirements:

- Windows 10 (19041+) or Windows 11, **64-bit**
- [WebView2 Runtime](https://developer.microsoft.com/microsoft-edge/webview2/) (usually already installed with Edge)

Run the installer (it prompts for UAC):

```text
out\x64\Debug\MediaTagsSetup.exe
```

or, if you built Release, `out\x64\Release\MediaTagsSetup.exe`.

Do not use the loose `MediaTags.exe` from the build folder: Explorer loads the copy installed in `%LOCALAPPDATA%\MediaTags`.

Uninstall from *Settings → Apps*, or run `MediaTagsSetup.exe /uninstall`.

If Windows Defender flags the installer or the `.exe` (`Trojan:Win32/Bearfoos.Alml`), it is a heuristic **false positive** (unsigned Authenticode installer). Restore or allow the file.

## Build

You need Visual Studio 2022/2026 with Desktop C++ development, the Windows SDK (MakeAppx and SignTool), and NuGet (`Microsoft.Web.WebView2`).

Open **`MediaTags.slnx`** and build **x64** (Win32/x86 will not work: Explorer is x64).

```bat
msbuild MediaTags.slnx /p:Configuration=Debug /p:Platform=x64
```

The deliverable is **`MediaTagsSetup.exe`**. After changing code, install it again; otherwise Explorer keeps using the old DLL from AppData. If the menu does not update, restart the *Windows Explorer* process.

If linking fails with LNK1168, close `MediaTags.exe` (it is sometimes elevated by UAC) and rebuild.

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

Formatos: `.jpg`, `.jpeg`, `.mp4` y `.mov`.

## Uso

1. Clic derecho en una o varias fotos/vídeos.
2. **Gestionar tags** (en Windows 11, si no aparece, *Mostrar más opciones*).
3. Añade o elimina tags. La lista de **tags comunes** es la intersección de los archivos seleccionados.

Los tags quedan en el archivo: se ven en el Explorador, en otras PCs y en programas que lean keywords de Windows.

## Instalación

Requisitos:

- Windows 10 (19041+) o Windows 11, **64 bits**
- [WebView2 Runtime](https://developer.microsoft.com/microsoft-edge/webview2/) (suele venir con Edge)

Ejecuta el instalador (pide UAC):

```text
out\x64\Debug\MediaTagsSetup.exe
```

o, si compilaste Release, `out\x64\Release\MediaTagsSetup.exe`.

No uses `MediaTags.exe` suelto de la carpeta de compilación: el menú del Explorador carga la copia instalada en `%LOCALAPPDATA%\MediaTags`.

Desinstalar: *Configuración → Aplicaciones*, o `MediaTagsSetup.exe /uninstall`.

Si Windows Defender marca el instalador o el `.exe` (`Trojan:Win32/Bearfoos.Alml`), es un **falso positivo** heurístico (instalador sin firma Authenticode). Restaura o permite el archivo.

## Compilar

Necesitas Visual Studio 2022/2026 con desarrollo de escritorio C++, Windows SDK (MakeAppx y SignTool) y NuGet (`Microsoft.Web.WebView2`).

Abre **`MediaTags.slnx`** y compila **x64** (Win32/x86 no sirve: el Explorador es x64).

```bat
msbuild MediaTags.slnx /p:Configuration=Debug /p:Platform=x64
```

El entregable es **`MediaTagsSetup.exe`**. Tras cambiar código, vuelve a instalarlo; si no, el Explorador sigue usando la DLL vieja de AppData. Si el menú no se actualiza, reinicia el proceso *Explorador de Windows*.

Si el link falla con LNK1168, cierra `MediaTags.exe` (a veces está elevado por UAC) y recompila.

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
