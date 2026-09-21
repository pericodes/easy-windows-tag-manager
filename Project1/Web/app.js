let commonTags = [];
let otherTags = [];

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

        empty.textContent =
            "No hay tags comunes.";

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
                    "Eliminar",
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
                    "Eliminar",
                    "danger small",
                    () => postAction("delete", [tag])
                ),
                createActionButton(
                    "Añadir a todos",
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


window.chrome.webview
    .addEventListener(
        "message",
        event => {

            const data = event.data;

            if (!data)
                return;

            commonTags =
                data.tags || [];

            otherTags =
                data.other || [];

            fileCount.textContent =
                `${data.files} archivo(s) seleccionado(s)`;

            renderTags();
        }
    );
