// Involves search field and pagination for the diagnostics table

document.addEventListener("DOMContentLoaded", function () {
    const searchInput = document.getElementById("diagnosticsSearch");
    const table = document.getElementById("diagnosticsTable");
    const paginationContainer = document.getElementById("diagnosticsPagination");

    if (!table) return;

    const tbody = table.querySelector("tbody");
    const headers = table.querySelectorAll("thead th");

    const ROWS_PER_PAGE = 10;
    let currentPage = 1;
    let sortColumn = null;
    let sortAscending = true;

    // Cache the original row elements once, so search/sort/pagination
    // all work off the same in-memory list instead of re-reading the DOM.
    let allRows = Array.from(tbody.querySelectorAll("tr"));

    function getFilteredRows() {
        const filter = (searchInput?.value || "").toUpperCase().trim();
        if (!filter) return allRows;

        return allRows.filter((row) => {
            const text = row.textContent.toUpperCase();
            return text.indexOf(filter) > -1;
        });
    }

    function sortRows(rows) {
        if (sortColumn === null) return rows;

        const sorted = [...rows].sort((a, b) => {
            const aText = a.children[sortColumn].textContent.trim();
            const bText = b.children[sortColumn].textContent.trim();

            const aNum = parseFloat(aText);
            const bNum = parseFloat(bText);
            const bothNumeric = !isNaN(aNum) && !isNaN(bNum);

            let result;
            if (bothNumeric) {
                result = aNum - bNum;
            } else {
                result = aText.localeCompare(bText);
            }

            return sortAscending ? result : -result;
        });

        return sorted;
    }

    function renderPage() {
        const filtered = sortRows(getFilteredRows());
        const totalPages = Math.max(1, Math.ceil(filtered.length / ROWS_PER_PAGE));
        currentPage = Math.min(currentPage, totalPages);

        const start = (currentPage - 1) * ROWS_PER_PAGE;
        const pageRows = filtered.slice(start, start + ROWS_PER_PAGE);

        tbody.innerHTML = "";
        if (pageRows.length === 0) {
            const emptyRow = document.createElement("tr");
            const cell = document.createElement("td");
            cell.colSpan = headers.length;
            cell.textContent = "No matching requests.";
            emptyRow.appendChild(cell);
            tbody.appendChild(emptyRow);
        } else {
            pageRows.forEach((row) => tbody.appendChild(row));
        }

        renderPagination(totalPages);
    }

    function renderPagination(totalPages) {
        if (!paginationContainer) return;
        paginationContainer.innerHTML = "";

        if (totalPages <= 1) return;

        const prevBtn = document.createElement("button");
        prevBtn.textContent = "Prev";
        prevBtn.disabled = currentPage === 1;
        prevBtn.addEventListener("click", () => {
            currentPage -= 1;
            renderPage();
        });
        paginationContainer.appendChild(prevBtn);

        const pageLabel = document.createElement("span");
        pageLabel.className = "CANprotocol-page-label";
        pageLabel.textContent = ` Page ${currentPage} of ${totalPages} `;
        paginationContainer.appendChild(pageLabel);

        const nextBtn = document.createElement("button");
        nextBtn.textContent = "Next";
        nextBtn.disabled = currentPage === totalPages;
        nextBtn.addEventListener("click", () => {
            currentPage += 1;
            renderPage();
        });
        paginationContainer.appendChild(nextBtn);
    }

    // Search box: re-render on every keystroke, reset to page 1
    if (searchInput) {
        searchInput.addEventListener("input", () => {
            currentPage = 1;
            renderPage();
        });
    }

    // Click a header to sort by that column; click again to reverse
    headers.forEach((header, index) => {
        header.style.cursor = "pointer";
        header.addEventListener("click", () => {
            if (sortColumn === index) {
                sortAscending = !sortAscending;
            } else {
                sortColumn = index;
                sortAscending = true;
            }
            renderPage();
        });
    });

    renderPage();
});