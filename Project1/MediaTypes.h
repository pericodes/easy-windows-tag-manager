#pragma once

// Solo extensiones donde IPropertyStore GPS_READWRITE persiste
// PKEY_Keywords y el Explorador las muestra en Tags. Comprobado:
// IsPropertyWritable == S_OK, SetValue+Commit, reopen con el handler.
// Mantener alineado con Package/AppxManifest.xml (desktop5:ItemType).
inline constexpr const wchar_t* kMediaTagExtensions[] = {
    L".jpg",  L".jpeg", L".jpe", L".jfif",
    L".png",
    L".tif",  L".tiff",
    L".avif",
    L".mp4",  L".m4v",  L".mov",
    L".wmv",  L".asf",
    L".3gp",  L".3g2",
};

// Registradas en 1.0.2 y retiradas: el handler de Windows no escribe
// System.Keywords (WebP: STG_E_ACCESSDENIED; GIF/BMP: WINCODEC).
inline constexpr const wchar_t* kRetiredMediaTagExtensions[] = {
    L".webp", L".gif",  L".bmp",
    L".heic", L".heif", L".hif",
    L".jxl",  L".dng",  L".jxr",  L".wdp",
};
