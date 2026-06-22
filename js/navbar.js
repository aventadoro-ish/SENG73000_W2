// builds the shared navbar for all pages
document.addEventListener("DOMContentLoaded", function () {
    const navbarPlaceholder = document.getElementById("navbar-placeholder");

    if (!navbarPlaceholder) {
        return;
    }

    const root = navbarPlaceholder.dataset.root || "";

    navbarPlaceholder.innerHTML = `
        <header class="site-header" id="page_top">
            <nav class="navbar">
                <div class="nav-brand">LiF Team</div>

                <ul class="nav-links">
                    <li><a href="${root}index.html" data-page="home">Homepage</a></li>
                    <li><a href="${root}html/about.html" data-page="about">About</a></li>
                    <li><a href="${root}html/proj_details_revamp.html" data-page="details">Project Details</a></li>
                    <li><a href="${root}html/proj_plan.html" data-page="plan">Project Plan</a></li>
                    <li><a href="${root}html/logbook_main.html" data-page="logbooks">Logbooks</a></li>
                    <li><a href="${root}html/documentation.html" data-page="documentation">Documentation</a></li>
                </ul>

                <ul class="nav-links nav-right">
                    <li><a href="${root}index.html#login-section">Login</a></li>
                    <li><a href="${root}request_access.html">Request Access</a></li>
                </ul>
            </nav>
        </header>
    `;

    highlightCurrentPage();
});

// Highlights the current navbar link
function highlightCurrentPage() {
    const currentFile = window.location.pathname.split("/").pop();
    const navLinks = document.querySelectorAll(".nav-links a");

    navLinks.forEach(function (link) {
        const linkFile = link.getAttribute("href").split("/").pop();

        if (currentFile === linkFile) {
            link.classList.add("active");
        }
    });
}