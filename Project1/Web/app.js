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
        addToAll: "Add to all"
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
        addToAll: "Añadir a todos"
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


function applyI18n(lang) {

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

    if (fileCount)
        fileCount.textContent =
            filesLabel(fileCountValue);

    renderTags();
}


function postAction(action, tags) {

    chrome.webview.postMessage({
        action,
        tags
    });
}


function postRename(from, to) {

    chrome.webview.postMessage({
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

            postAction("add", [tag]);

            modal.classList.add(
                "hidden"
            );
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


if (window.chrome && window.chrome.webview) {
    window.chrome.webview.addEventListener(
        "message",
        event => {

            const data = event.data;

            if (!data)
                return;

            commonTags =
                data.tags || [];

            otherTags =
                data.other || [];

            fileCountValue =
                data.files || 0;

            if (langSelect && data.pref)
                langSelect.value = data.pref;

            applyI18n(data.lang);
        }
    );
}
