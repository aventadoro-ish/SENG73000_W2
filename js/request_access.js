const details = document.getElementById("details");
const characterCount = document.getElementById("character_count");
const characterError = document.getElementById("character_error");
const accessForm = document.querySelector(".access-form");

const maximumCharacters = 180;

function updateCharacterCount() {
    const charactersUsed = details.value.length;
    const charactersRemaining = maximumCharacters - charactersUsed;

    if (charactersRemaining >= 0) {
        characterCount.textContent =
            charactersRemaining + " characters remaining";

        characterError.hidden = true;
    } else {
        characterCount.textContent =
            Math.abs(charactersRemaining) + " characters over the limit";

        characterError.hidden = false;
    }
}

details.addEventListener("input", updateCharacterCount);

accessForm.addEventListener("submit", function (event) {
    if (details.value.length > maximumCharacters) {
        event.preventDefault();
        characterError.hidden = false;
        details.focus();
    }
});

updateCharacterCount();