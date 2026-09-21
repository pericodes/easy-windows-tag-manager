# AGENTS.md — Media Tags (easy-windows-tag-manager)

Documento para que otra IA continúe el trabajo. El producto es una app Win32 de Windows 10/11 que añade **Gestionar tags** al menú contextual de fotos y vídeos. Los tags se guardan en `System.Keywords` (propiedades de archivo de Windows). La UI es HTML/JS dentro de WebView2.

Extensiones (lista canónica `Project1/MediaTypes.h`, duplicada en `Package/AppxManifest.xml`). Solo las que el `IPropertyStore` nativo **escribe y relee** `PKEY_Keywords` (Tags del Explorador):

- Imágenes: `.jpg` `.jpeg` `.jpe` `.jfif` `.png` `.tif` `.tiff` `.avif`
- Vídeos: `.mp4` `.m4v` `.mov` `.wmv` `.asf` `.3gp` `.3g2`

No incluir `.webp` `.gif` `.bmp` `.heic` `.mkv` `.avi` `.webm` `.mpg`: `IsPropertyWritable(PKEY_Keywords)` es `S_FALSE` o `SetValue` falla (`STG_E_ACCESSDENIED` / WINCODEC). El menú no filtra en la DLL; el filtro es manifiesto + HKCU. `kRetiredMediaTagExtensions` se borra al reinstalar para quitar verbos viejos.

Responde al usuario en **español**.

## Qué es y qué no es

- **No** es un único `.vcxproj`. Un proyecto de Visual Studio emite un binario. Aquí hacen falta **exe + dll**, así que hay **una solución y tres proyectos**.
- **No** se registra con `regsvr32` como extensión clásica de Explorer. En Windows 11 el menú moderno exige `IExplorerCommand` **con identidad de paquete** (MSIX sparse).
- El Explorador de Windows es **x64**. Compilar Win32/x86 no sirve para el menú.

## Solución canónica

Abrir y compilar **`MediaTags.slnx`** (raíz del repo), plataforma **x64**.

| Proyecto | Tipo | Salida |
|---|---|---|
| `Dll/Dll.vcxproj` | DLL | `out\x64\<Config>\MediaTagsShell.dll` |
| `Project1/Project1.vcxproj` | EXE Win32 + WebView2 | `out\x64\<Config>\MediaTags.exe` |
| `Setup/Setup.vcxproj` | Stub SFX + UAC | `out\x64\<Config>\MediaTagsSetup.exe` |

`Project1` referencia `Dll` **sin enlazar** el `.lib` (`LinkLibraryDependencies=false`). `Setup` referencia `Project1` igual.

Los tres proyectos tienen `/utf-8` en `ClCompile`. Sin eso, los `MessageBox` en español salen como `Â¿Instalar` / `menÃº`.

Los textos de UI en C++ con tildes/ñ/¿ deben ir como escapes Unicode (`L"\u00BFInstalar..."`), no como UTF-8 crudo en el fuente.

### Carpetas que NO son el código activo

Ignóralas salvo para copiar ideas antiguas:

- `MediaTagsManager/` — stub vacío / copia vieja
- `_MediaTagsManager/` — fuentes originales de referencia
- `Dll/Dll.slnx`, `Project1/Project1.slnx` — soluciones individuales antiguas

El código que se mantiene es: `Dll/`, `Project1/`, `Setup/`, `Package/`.

## Cómo compilar

Requisitos: Visual Studio 2022/2026 con C++ de escritorio, Windows SDK (MakeAppx + SignTool), NuGet `Microsoft.Web.WebView2` 1.0.4191.47 (`packages/` no está en git; restaurar con `nuget restore Project1\packages.config -PackagesDirectory Project1\packages`).

```bat
msbuild MediaTags.slnx /p:Configuration=Debug /p:Platform=x64
```

Post-build:

1. `Project1` ejecuta `Package/build-msix.ps1` → `MediaTags.msix` + `MediaTags.cer` junto al exe.
2. `Setup` ejecuta `Package/build-installer.ps1` → concatena stub + zip + footer `MTPK` → **`out\x64\Debug\MediaTagsSetup.exe`**.

Si `MediaTags.exe` o `WebView2Loader.dll` están bloqueados (instalador/UI abiertos), el link falla con LNK1168 / MSB3027. Cerrar `MediaTags.exe` (a veces elevado por UAC) y recompilar.

Entregable para el usuario: **`MediaTagsSetup.exe`** (localmente `out\x64\Release\MediaTagsSetup.exe`, o el adjunto de una GitHub Release), no el exe suelto de Debug.

### Publicar una GitHub Release

El workflow `.github/workflows/build.yml` compila **Release x64** en `windows-latest` (VS 2026 / v145). Un push a `main` o un PR deja el instalador como artifact de Actions. Un tag `vX.Y.Z` crea una **GitHub Release** (pestaña Releases, no Tags) y adjunta `MediaTagsSetup.exe`.

Un tag solo no publica el `.exe`: hay que esperar a que Actions termine en verde. Si el job falla, Releases queda vacío y en Tags solo se ven los zips de código fuente.

Si el tag ya existe y hay que republicar: Actions → *Build and release* → *Run workflow* (`create_release` + tag `v1.0.3`).

La versión del tag debe coincidir con `Package/AppxManifest.xml` (`v1.0.3` → `1.0.3.0`). Si cambias el manifiesto, súbela antes de etiquetar.

```bat
git tag v1.0.3
git push origin v1.0.3
```

El usuario final descarga ese `.exe` y no necesita Visual Studio. El MSIX sigue yendo autofirmado (`CN=MediaTags`); el instalador importa el `.cer` embebido. Defender puede seguir marcando falso positivo.

## Arquitectura en runtime

```
Explorador
  → COM IExplorerCommand (CLSID 7D3B1F20-6F62-4B3A-9162-1842775210A1)
  → MediaTagsShell.dll
  → escribe lista UTF-8 en %LOCALAPPDATA%\MediaTags\{guid}.txt
  → ShellExecute MediaTags.exe --selection "ruta.txt"
       → WebView2 carga Web\index.html
       → lee/escribe PKEY_Keywords
```

CLSID (también en `dllmain.cpp` / `ExplorerCommand.cpp`):

```
{7D3B1F20-6F62-4B3A-9162-1842775210A1}
```

En AppxManifest los GUID van **sin llaves** (el schema no las admite). En el registro clásico van **con llaves**.

### DLL (`Dll/`)

- `dllmain.cpp`: `DllGetClassObject`, `DllCanUnloadNow` (STDAPI, `extern "C"`).
- `Dll.def`: exporta esas dos funciones `PRIVATE`.
- `ClassFactory.cpp`: crea `ExplorerCommand`.
- `ExplorerCommand` implementa `IExplorerCommand` + `IObjectWithSite` + `IObjectWithSelection`.
- `Invoke` **no puede** hacer `if (!items) return E_INVALIDARG`. Explorer a menudo llama con `items == nullptr`. Hay que resolver la selección así:
  1. `items` si viene
  2. `m_selection` (`IObjectWithSelection`)
  3. `IUnknown_QueryService(m_site, SID_SFolderView)` + `IFolderView::Items(SVGIO_SELECTION, ...)`
- El fichero de selección se escribe en **UTF-8** con `CreateFileW`/`WriteFile`. `MediaTags.exe` lo lee con `CP_UTF8`. No usar `std::wofstream` (UTF-16/ANSI → lista vacía y el exe sale en silencio).
- Lanza el exe con `ShellExecuteExW` + `AllowSetForegroundWindow`, no solo `CreateProcess`. Busca primero `%LOCALAPPDATA%\MediaTags\MediaTags.exe` y si no, el directorio de la DLL.
- Si el lanzamiento falla, `MessageBox`. Si el exe arranca y falla al leer archivos, también `MessageBox` (antes no hacía nada).

### EXE (`Project1/`)

Punto de entrada: `Project1.cpp` (`wWinMain`). Argumentos:

| Args | Efecto |
|---|---|
| `--selection <txt>` | UI de tags |
| `--install` / `--install --silent` | instalador |
| `--uninstall` / `--uninstall --silent` | desinstalador |
| (ninguno) | diálogo instalar/reinstalar |

`Project1/main.cpp` es duplicado muerto; no está en el vcxproj.

- `WebViewApp.cpp`: ventana + WebView2.
- Tras `Navigate`, **`NavigationCompleted` debe llamar a `SendInitialData()`**. Si no, la lista de tags comunes queda vacía aunque los keywords existan en disco.
- Tras `action: add`, `delete` o `rename`, **refrescar** con `SendInitialData()`, **no** cerrar la ventana (`WM_CLOSE` era el comportamiento viejo).
- User data de WebView2: `%LOCALAPPDATA%\MediaTags\WebView2`.
- JSON hacia JS: `PostWebMessageAsJson` `{"files":N,"tags":["..."],"other":["..."]}`. `tags` son los comunes (intersección); `other` los que tiene algún archivo pero no todos.
- JSON desde JS: `{"action":"add"|"delete","tags":[...]}` o `{"action":"rename","from":"...","to":"..."}`. `rename` solo sustituye el tag en los archivos que ya lo tienen; no lo añade al resto. Parsear el array `"tags"` o los campos `"from"`/`"to"`, no todas las cadenas entre comillas.

`Tags.cpp`:

- Write: `GPS_READWRITE` + `PKEY_Keywords` + `Commit`.
- Read: `GPS_HANDLERPROPERTIESONLY | GPS_BESTEFFORT`, fallback `GPS_DEFAULT`.
- Add/Remove leen, modifican, Write.
- Los tags **sí se guardan** en el archivo; si no se ven en la UI, el fallo está en el envío a WebView, no en `Tags::Write`.

UI web (`Project1/Web/`): `index.html`, `app.js`, `style.css`. Se copian al OutDir. No hace falta cambiar JS para el refresh: el host reenvía el mensaje y `app.js` vuelve a pintar.

### Instalador (`Setup/` + `Project1/Install.cpp`)

`MediaTagsSetup.exe` es un autoextraíble:

1. Lee un footer de 16 bytes al final: magic `0x4B50544D` (`MTPK`), `zipOffset`, `zipSize`, reserved.
2. Extrae el zip con `tar.exe` a un temp.
3. Ejecuta `MediaTags.exe --install --silent` (UAC: el stub es `RequireAdministrator`).
4. `/uninstall` o `--silent` / `/S` para silencio.

Instalación real (`InstallMediaTags`):

1. Quita el menú clásico del registro y el paquete Appx (para soltar la DLL).
2. Copia a `%LOCALAPPDATA%\MediaTags\` con **reemplazo si el destino está en uso**: si `copy_file` falla, `MoveFileEx` el destino a `.old` y copia encima (Explorer suele tener `MediaTagsShell.dll` cargada).
3. Importa el `.cer` con **crypt32** (`CertAddCertificateContextToStore` → `TrustedPeople`), pone `AllowAllTrustedApps=1` en HKLM y registra el MSIX con **PackageManager** WinRT (`AddPackageByUriAsync` + `ExternalLocationUri`). **No** lanzar `powershell.exe`, `cmd.exe` ni `Add-AppxPackage`: Defender lo marcaba como `Trojan:Win32/Bearfoos.Alml`.
4. Escribe clave de desinstalación HKCU y verbos clásicos `ExplorerCommandHandler`.

Desinstalación:

- `PackageManager::RemovePackageAsync`, quita registro, borra ficheros salvo el exe en ejecución; lo que quede se marca con `MoveFileEx(..., MOVEFILE_DELAY_UNTIL_REBOOT)`.
- Antes, desinstalar desde Configuración **dejaba** `%LOCALAPPDATA%\MediaTags\MediaTags.exe` porque el exe se borraba a sí mismo mientras mostraba un MessageBox. Eso hacía que el siguiente “instalar” dijera “ya está instalado”.
- Hoy: si hay restos, el diálogo es **¿Reinstalar?** no un callejón sin salida.

No ejecutar `MediaTags.exe` de `out\x64\Debug` como instalador si se puede evitar: usar **`MediaTagsSetup.exe`**.

## Identidad de paquete (MSIX sparse)

`Package/AppxManifest.xml`:

- `uap10:AllowExternalContent=true` (sparse: el msix solo lleva manifiesto + logos 1×1).
- `ProcessorArchitecture="x64"` (Neutral rompe la carga de la DLL en Explorer).
- Versión actual: **1.0.3.0**. Si cambias el manifiesto, súbela.
- Menú Win11: **`desktop5:ItemType` / `desktop5:Verb`** dentro de `desktop4:FileExplorerContextMenus`. `desktop4:ItemType` **no aparece** en el menú moderno.
- Publisher del cert autofirmado: `CN=MediaTags` (debe coincidir con `Identity/@Publisher`).
- `build-msix.ps1` crea/reutiliza el cert en `Cert:\CurrentUser\My`, firma con SignTool, deja `.msix` y `.cer` en OutDir.

Registro clásico de respaldo (HKCU), para “Mostrar más opciones”:

```
HKCU\Software\Classes\CLSID\{7D3B1F20-...}\InProcServer32 = ...\MediaTagsShell.dll
HKCU\Software\Classes\SystemFileAssociations\.jpg\shell\MediaTags
  ExplorerCommandHandler = {7D3B1F20-...}
```

Igual para el resto de extensiones de `MediaTypes.h`.

## Errores ya vistos y cómo no repetirlos

1. **Menú no sale** — manifiesto `desktop4` en vez de `desktop5`; paquete Neutral; no reinstalar tras cambiar la DLL (Explorer cachea). Reiniciar Explorer ayuda.
2. **Clic en Gestionar tags no hace nada** — `Invoke` con `items` nulo; fichero de selección mal codificado; ventana detrás de Explorer; exe saliendo en silencio.
3. **No se ven tags comunes (pero sí se guardan)** — `SendInitialData` nunca se llamaba. Hace falta `NavigationCompleted`.
4. **Al añadir tag se cierra la ventana** — `PostMessage(WM_CLOSE)` tras add/delete. Ahora se refresca.
5. **Texto `Â¿` `Ã±`** — falta `/utf-8` o literales UTF-8 sin escapes.
6. **“Ya está instalado” tras desinstalar** — restos en `%LOCALAPPDATA%\MediaTags`. Reinstalar o borrar esa carpeta.
7. **“No se pudo copiar MediaTagsShell.dll”** — DLL cargada por Explorer. Renombrar a `.old` y copiar; o reiniciar Explorer e instalar `MediaTagsSetup.exe`.
8. **LNK1168 al compilar** — `MediaTags.exe` abierto (a veces elevado). Cerrar procesos y recompilar.
9. **Defender `Trojan:Win32/Bearfoos.Alml`** — falso positivo heurístico. La causa era `powershell -ExecutionPolicy Bypass` + `cmd.exe` + importar certificado + registrar Appx desde `Install.cpp`. Eso ya no está en el `.exe`. Si sigue saliendo: restaurar/permitir el archivo, usar `MediaTagsSetup.exe` (no el Debug suelto), y si hace falta exclusión o envío a WDSI. Un certificado Authenticode de verdad reduce falsos positivos. El PowerShell de los post-build (`build-msix.ps1`) **no** se incrusta en el binario.
10. **WebP / GIF / BMP no guardan tags nativos** — `IsPropertyWritable(PKEY_Keywords)` es `S_FALSE`. En WebP `SetValue` devuelve `STG_E_ACCESSDENIED` (0x80030005). El Photo Property Handler de Windows solo escribe keywords con política JPEG/TIFF (PNG/AVIF sí). **No** inventar XMP propio: si el Explorador no puede mostrar Tags, no se registra el tipo. Ver `kRetiredMediaTagExtensions`.

MakeAppx exige: `PublisherDisplayName` en una línea, `BackgroundColor` en VisualElements, GUID sin `{}`.

## Flujo de trabajo recomendado al cambiar código

1. Editar `Dll/` y/o `Project1/` (y `Package/AppxManifest.xml` si el menú/COM cambia).
2. Compilar `MediaTags.slnx` x64 Debug (o Release).
3. Entregar/ejecutar **`MediaTagsSetup.exe`** para copiar a AppData. Si no se reinstala, Explorer sigue usando la DLL vieja de AppData.
4. Probar: clic derecho en un `.jpg` → Gestionar tags → lista de comunes → añadir → la lista se actualiza sin cerrar.
5. Si el menú no cambia: Task Manager → Explorador de Windows → Reiniciar.

## Cosas pendientes / mejoras razonables

- Iconos reales del paquete (hoy PNG 1×1).
- Más tipos solo si `IPropertyStore` nativo escribe y el Explorador muestra Tags. `.webp`/`.gif`/`.bmp`/`.mkv` no.
- Certificado de código firmado de verdad (el autofirmado exige confiar el `.cer` / sideload).
- `Project1` sigue llamándose Project1; el TargetName ya es `MediaTags`.
- `g_objects` en la DLL no se incrementa; `DllCanUnloadNow` casi siempre S_OK.
- Comparación de tags comunes es sensible a mayúsculas.
- No hay tests automatizados.

## Convenciones

- C++20, Unicode, toolset v145.
- No crear README extra ni refactorizar las carpetas muertas salvo que lo pidan.
- No hagas `git commit` a menos que lo pidan.
- No uses `regsvr32`. No fusiones exe y dll en un solo vcxproj.
