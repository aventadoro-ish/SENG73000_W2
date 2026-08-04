// diagnostics.js
// Handles search, sorting, and pagination for the diagnostics tables.
//
// initDiagnosticsTable() sets up one table's search/sort/pagination in
// isolation, so the CAN translation table and the request-details table
// below it behave independently

function initDiagnosticsTable({ tableId, searchInputId, paginationId, rowsPerPage = 10 }) {
    const table = document.getElementById(tableId);
    if (!table) return;

    const searchInput = document.getElementById(searchInputId);
    const paginationContainer = document.getElementById(paginationId);
    const tbody = table.querySelector("tbody");
    const headers = table.querySelectorAll("thead th");

    let currentPage = 1;
    let sortColumn = null;
    let sortAscending = true;

    // Cache the original row elements once, so search/sort/pagination
    // all work off the same in-memory list instead of re-reading the DOM.
    const allRows = Array.from(tbody.querySelectorAll("tr"));

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

        return [...rows].sort((a, b) => {
            const aText = a.children[sortColumn].textContent.trim();
            const bText = b.children[sortColumn].textContent.trim();

            const aNum = parseFloat(aText);
            const bNum = parseFloat(bText);
            const bothNumeric = !isNaN(aNum) && !isNaN(bNum);

            const result = bothNumeric ? aNum - bNum : aText.localeCompare(bText);
            return sortAscending ? result : -result;
        });
    }

    function renderPage() {
        const filtered = sortRows(getFilteredRows());
        const totalPages = Math.max(1, Math.ceil(filtered.length / rowsPerPage));
        currentPage = Math.min(currentPage, totalPages);

        const start = (currentPage - 1) * rowsPerPage;
        const pageRows = filtered.slice(start, start + rowsPerPage);

        tbody.innerHTML = "";
        if (pageRows.length === 0) {
            const emptyRow = document.createElement("tr");
            const cell = document.createElement("td");
            cell.colSpan = headers.length;
            cell.textContent = "No matching rows.";
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

    if (searchInput) {
        searchInput.addEventListener("input", () => {
            currentPage = 1;
            renderPage();
        });
    }

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
}

document.addEventListener("DOMContentLoaded", function () {
    initDiagnosticsTable({
        tableId: "diagnosticsTable",
        searchInputId: "diagnosticsSearch",
        paginationId: "diagnosticsPagination",
    });

    initDiagnosticsTable({
        tableId: "requestDetailsTable",
        searchInputId: "requestDetailsSearch",
        paginationId: "requestDetailsPagination",
    });
});