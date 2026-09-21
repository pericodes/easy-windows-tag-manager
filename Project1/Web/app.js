let commonTags = [];

const tagsContainer =
    document.getElementById("tags");

const fileCount =
    document.getElementById("fileCount");

const modal =
    document.getElementById("modal");

const newTag =
    document.getElementById("newTag");


function renderTags() {

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

            chrome.webview.postMessage({
                action: "delete",
                tags
            });
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

            chrome.webview.postMessage({
                action: "add",
                tags: [tag]
            });

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

            fileCount.textContent =
                `${data.files} archivo(s) seleccionado(s)`;

            renderTags();
        }
    );
