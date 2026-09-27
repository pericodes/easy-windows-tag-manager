let commonTags = [];
let otherTags = [];
let fileCountValue = 0;
let currentLang = "en";

const strings = {
    en: {
        manageTags: "Manage tags",
        commonTags: "Common tags",
        add: "Add",
        otherTags: "Other tags",
        addTag: "Add tag",
        placeholder: "Type a tag",
        cancel: "Cancel",
        emptyCommon: "No common tags.",
        remove: "Remove",
        addToAll: "Add to all",
        loading: "Loading tags…",
        updating: "Updating tags…"
    },
    es: {
        manageTags: "Gestionar tags",
        commonTags: "Tags comunes",
        add: "Añadir",
        otherTags: "Otros tags",
        addTag: "Añadir tag",
        placeholder: "Escribe un tag",
        cancel: "Cancelar",
        emptyCommon: "No hay tags comunes.",
        remove: "Eliminar",
        addToAll: "Añadir a todos",
        loading: "Cargando tags…",
        updating: "Actualizando tags…"
    }
};

const tagsContainer =
    document.getElementById("tags");

const otherSection =
    document.getElementById("otherSection");

const otherTagsContainer =
    document.getElementById("otherTags");

const fileCount =
    document.getElementById("fileCount");

const modal =
    document.getElementById("modal");

const newTag =
    document.getElementById("newTag");

const langSelect =
    document.getElementById("lang");

const busyOverlay =
    document.getElementById("busy");

const busyText =
    document.getElementById("busyText");

const statusLine =
    document.getElementById("status");

let busy = false;


function t(key) {

    const table =
        strings[currentLang] || strings.en;

    return table[key] || strings.en[key] || key;
}


function filesLabel(count) {

    if (currentLang === "es") {

        return count === 1
            ? "1 archivo seleccionado"
            : `${count} archivos seleccionados`;
    }

    return count === 1
        ? "1 file selected"
        : `${count} files selected`;
}


function applyI18n(lang, paintTags) {

    currentLang =
        lang === "es" ? "es" : "en";

    document.documentElement.lang =
        currentLang;

    document.title = t("manageTags");

    document.querySelectorAll("[data-i18n]").forEach(
        el => {
            el.textContent = t(el.dataset.i18n);
        }
    );

    document.querySelectorAll("[data-i18n-placeholder]").forEach(
        el => {
            el.placeholder =
                t(el.dataset.i18nPlaceholder);
        }
    );

    if (fileCount && (paintTags !== false || fileCountValue))
        fileCount.textContent =
            filesLabel(fileCountValue);

    if (paintTags !== false)
        renderTags();
}


function progressLabel(done, total, mode) {

    const action = t(
        mode === "update" ? "updating" : "loading"
    );

    return `${action} ${done} / ${total}`;
}


function failedLabel(count) {

    if (currentLang === "es") {

        return count === 1
            ? "No se pudo actualizar 1 archivo."
            : `No se pudieron actualizar ${count} archivos.`;
    }

    return count === 1
        ? "Could not update 1 file."
        : `Could not update ${count} files.`;
}


function showBusy(done, total, mode) {

    busy = true;
    document.body.classList.add("is-busy");

    if (busyOverlay)
        busyOverlay.classList.remove("hidden");

    if (!busyText)
        return;

    if (total > 0)
        busyText.textContent =
            progressLabel(done, total, mode);
    else
        busyText.textContent =
            t(mode === "update" ? "updating" : "loading");
}


function hideBusy() {

    busy = false;
    document.body.classList.remove("is-busy");

    if (busyOverlay)
        busyOverlay.classList.add("hidden");
}


function showStatus(failed) {

    if (!statusLine)
        return;

    if (!failed) {

        statusLine.textContent = "";
        statusLine.classList.add("hidden");
        return;
    }

    statusLine.textContent = failedLabel(failed);
    statusLine.classList.remove("hidden");
}


function beginWork() {

    if (busy)
        return false;

    if (statusLine)
        statusLine.classList.add("hidden");

    showBusy(0, fileCountValue, "update");
    return true;
}


function postToHost(message) {

    if (!(window.chrome && window.chrome.webview))
        return;

    chrome.webview.postMessage(message);
}


function postAction(action, tags) {

    if (!beginWork())
        return;

    postToHost({
        action,
        tags
    });
}


function postRename(from, to) {

    if (!beginWork())
        return;

    postToHost({
        action: "rename",
        from,
        to
    });
}


function beginEdit(nameEl, original) {

    const input =
        document.createElement("input");

    input.type = "text";
    input.className = "tag-row-input";
    input.value = original;

    let done = false;

    function restore() {

        input.replaceWith(nameEl);
    }

    function finish(save) {

        if (done)
            return;

        done = true;

        const next =
            input.value.trim();

        if (save && next && next !== original) {

            postRename(original, next);
            return;
        }

        restore();
    }

    input.addEventListener(
        "keydown",
        event => {

            if (event.key === "Enter") {

                event.preventDefault();
                input.blur();
            }

            if (event.key === "Escape") {

                event.preventDefault();
                done = true;
                restore();
            }
        }
    );

    input.addEventListener(
        "blur",
        () => finish(true)
    );

    nameEl.replaceWith(input);
    input.focus();
    input.select();
}


function createActionButton(label, className, onClick) {

    const button =
        document.createElement("button");

    button.className = className;
    button.textContent = label;
    button.addEventListener("click", onClick);

    return button;
}


function createTagRow(tag, actions) {

    const row =
        document.createElement("div");

    row.className = "tag-row";

    const name =
        document.createElement("span");

    name.className = "tag-row-name";
    name.textContent = tag;
    name.title = tag;

    name.addEventListener(
        "click",
        () => beginEdit(name, tag)
    );

    const group =
        document.createElement("div");

    group.className = "tag-row-actions";

    for (const action of actions)
        group.appendChild(action);

    row.appendChild(name);
    row.appendChild(group);

    return row;
}


function renderCommonTags() {

    tagsContainer.innerHTML = "";

    if (commonTags.length === 0) {

        const empty =
            document.createElement("div");

        empty.textContent = t("emptyCommon");

        empty.style.color =
            "#777";

        tagsContainer.appendChild(
            empty
        );

        return;
    }

    for (const tag of commonTags) {

        tagsContainer.appendChild(
            createTagRow(tag, [
                createActionButton(
                    t("remove"),
                    "danger small",
                    () => postAction("delete", [tag])
                )
            ])
        );
    }
}


function renderOtherTags() {

    if (!otherSection || !otherTagsContainer)
        return;

    otherTagsContainer.innerHTML = "";

    if (otherTags.length === 0) {

        otherSection.classList.add(
            "hidden"
        );

        return;
    }

    otherSection.classList.remove(
        "hidden"
    );

    for (const tag of otherTags) {

        otherTagsContainer.appendChild(
            createTagRow(tag, [
                createActionButton(
                    t("remove"),
                    "danger small",
                    () => postAction("delete", [tag])
                ),
                createActionButton(
                    t("addToAll"),
                    "primary small",
                    () => postAction("add", [tag])
                )
            ])
        );
    }
}


function renderTags() {

    renderCommonTags();
    renderOtherTags();
}


document
    .getElementById("add")
    .addEventListener(
        "click",
        () => {

            newTag.value = "";

            modal.classList.remove(
                "hidden"
            );

            newTag.focus();
        }
    );


document
    .getElementById("cancel")
    .addEventListener(
        "click",
        () => {

            modal.classList.add(
                "hidden"
            );
        }
    );


document
    .getElementById("confirmAdd")
    .addEventListener(
        "click",
        () => {

            const tag =
                newTag.value.trim();

            if (!tag) {
                return;
            }

            modal.classList.add(
                "hidden"
            );

            postAction("add", [tag]);
        }
    );


newTag.addEventListener(
    "keydown",
    event => {

        if (event.key === "Enter") {

            document
                .getElementById("confirmAdd")
                .click();
        }

        if (event.key === "Escape") {

            document
                .getElementById("cancel")
                .click();
        }
    }
);


if (langSelect) {

    langSelect.addEventListener(
        "change",
        () => {

            if (!(window.chrome && window.chrome.webview))
                return;

            chrome.webview.postMessage({
                action: "setLang",
                lang: langSelect.value
            });
        }
    );
}


(function applyLangFromQuery() {

    const params =
        new URLSearchParams(location.search);

    const pref = params.get("pref");
    const lang = params.get("lang");

    if (pref && langSelect)
        langSelect.value = pref;

    if (lang)
        applyI18n(lang, false);
})();


function applyHostMessage(data) {

    if (!data)
        return;

    if (typeof data.progress === "number") {

        showBusy(
            data.progress,
            data.total || 0,
            "update"
        );

        return;
    }

    if (!Array.isArray(data.tags))
        return;

    hideBusy();

    commonTags = data.tags;

    otherTags = data.other || [];

    fileCountValue = data.files || 0;

    showStatus(data.failed || 0);

    if (langSelect && data.pref)
        langSelect.value = data.pref;

    applyI18n(data.lang);
}


if (window.chrome && window.chrome.webview) {
    window.chrome.webview.addEventListener(
        "message",
        event => applyHostMessage(event.data)
    );
}
