document.addEventListener("DOMContentLoaded", function () {

    // find the empty placeholder div where the navbar will be inserted
    const placeholder = document.getElementById("navbar-placeholder");

    if (!placeholder) {
        return;
    }

    // get the root path from the data-root attribute in the HTML
    const root = placeholder.getAttribute("data-root") || "";

    // load the shared navbar HTML file
    fetch(root + "html/navbar.html")

        // convert the  file into plain text
        .then(function (response) {
            return response.text();
        })

        // insert the navbar into the placeholder div
        .then(function (navbarHTML) {

            // replace all {{root}} markers in navbar.html with the correct path
            navbarHTML = navbarHTML.replaceAll("{{root}}", root);

            placeholder.innerHTML = navbarHTML;
        })

        .catch(function (error) {
            console.error("Navbar failed to load:", error);
        });
});