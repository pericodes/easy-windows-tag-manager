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

        const label =
            document.createElement("label");

        label.className = "tag";

        const checkbox =
            document.createElement("input");

        checkbox.type = "checkbox";

        checkbox.value = tag;

        const text =
            document.createElement("span");

        text.textContent = tag;

        label.appendChild(
            checkbox
        );

        label.appendChild(
            text
        );

        tagsContainer.appendChild(
            label
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

        const row =
            document.createElement("div");

        row.className = "other-row";

        const name =
            document.createElement("span");

        name.className = "other-row-name";
        name.textContent = tag;
        name.title = tag;

        const actions =
            document.createElement("div");

        actions.className =
            "other-row-actions";

        const remove =
            document.createElement("button");

        remove.className =
            "danger small";

        remove.textContent =
            "Eliminar";

        remove.addEventListener(
            "click",
            () => {
                postAction(
                    "delete",
                    [tag]
                );
            }
        );

        const addToAll =
            document.createElement("button");

        addToAll.className =
            "primary small";

        addToAll.textContent =
            "Añadir a todos";

        addToAll.addEventListener(
            "click",
            () => {
                postAction(
                    "add",
                    [tag]
                );
            }
        );

        actions.appendChild(remove);
        actions.appendChild(addToAll);

        row.appendChild(name);
        row.appendChild(actions);

        otherTagsContainer.appendChild(
            row
        );
    }
}


function renderTags() {

    renderCommonTags();
    renderOtherTags();
}


function selectedTags() {

    return Array.from(
        tagsContainer
            .querySelectorAll(
                "input[type=checkbox]:checked"
            )
    ).map(
        checkbox => checkbox.value
    );
}


document
    .getElementById("delete")
    .addEventListener(
        "click",
        () => {

            const tags =
                selectedTags();

            if (tags.length === 0) {
                return;
            }

            postAction("delete", tags);
        }
    );


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
