// ---------- Gantt Table Helper ----------
// This script keeps the source HTML manageable while improving the visual table grid.
//
// Source HTML can use:
//     <td colspan="8"></td>
//
// After the page loads, this script converts that blank filler cell into:
//     <td></td><td></td><td></td>...
//
// Coloured task bars like:
//     <td class="planned" colspan="2">NK, MR</td>
//
// are left alone so they still visually span multiple weeks.

document.addEventListener("DOMContentLoaded", function () {
    expandBlankGanttCells();
    checkGanttRows();
});


// Expands blank colspan cells into real empty table cells
function expandBlankGanttCells() {
    // Find the Gantt table
    const ganttTable = document.getElementById("Gantt");

    // Stop if the table is not on this page
    if (!ganttTable) {
        return;
    }

    // Stop if this script already ran
    if (ganttTable.classList.contains("gantt-expanded")) {
        return;
    }

    // Mark the table as expanded
    ganttTable.classList.add("gantt-expanded");

    // Find all rows in the Gantt table body
    const rows = ganttTable.querySelectorAll("tbody tr");

    rows.forEach(function (row, rowIndex) {
        // Skip the phase row:
        // N/A | Phase 1 | Phase 2 | Phase 3
        if (rowIndex === 0) {
            return;
        }

        // Skip major section rows:
        // 1. Project plan, 2. Raspberry Pi, etc.
        if (row.classList.contains("gantt-section-row")) {
            return;
        }

        // Convert row.children into a real array
        const cells = Array.from(row.children);

        // First cell is the task name, so timeline cells start after it
        const timelineCells = cells.slice(1);

        timelineCells.forEach(function (cell) {
            // Only work on normal <td> cells
            if (cell.tagName !== "TD") {
                return;
            }

            // Get the colspan number; if none exists, default to 1
            const span = Number(cell.getAttribute("colspan") || "1");

            // If the cell has no useful colspan, clean it up and stop
            if (span <= 1) {
                cell.removeAttribute("colspan");
                return;
            }

            // Only expand blank filler cells.
            // Do not expand coloured task/status cells.
            if (!isBlankFillerCell(cell)) {
                return;
            }

            // Create one real blank <td> for each week this blank cell was spanning
            for (let i = 0; i < span; i++) {
                const newCell = document.createElement("td");
                newCell.classList.add("gantt-empty-cell");

                // Insert the new cell before the old blank colspan cell
                row.insertBefore(newCell, cell);
            }

            // Remove the old large blank colspan cell
            cell.remove();
        });
    });
}


// Checks if a cell is just blank filler space
function isBlankFillerCell(cell) {
    // If the cell contains text, it is not blank filler
    if (cell.textContent.trim() !== "") {
        return false;
    }

    // These classes mean the cell is a real task/status bar
    const statusClasses = ["completed", "ongoing", "behind", "planned", "omitted"];

    for (let i = 0; i < statusClasses.length; i++) {
        if (cell.classList.contains(statusClasses[i])) {
            return false;
        }
    }

    // No text and no status class means it is blank filler
    return true;
}


// Optional debug helper.
// It warns in the browser console if a task row does not add up to 14 weeks.
function checkGanttRows() {
    const ganttTable = document.getElementById("Gantt");

    if (!ganttTable) {
        return;
    }

    const rows = ganttTable.querySelectorAll("tbody tr");

    rows.forEach(function (row, rowIndex) {
        // Skip phase row
        if (rowIndex === 0) {
            return;
        }

        // Skip section rows
        if (row.classList.contains("gantt-section-row")) {
            return;
        }

        const cells = Array.from(row.children);

        // Skip rows that do not have a task label + timeline cells
        if (cells.length < 2) {
            return;
        }

        // Skip first cell because it is the task label
        const timelineCells = cells.slice(1);

        let weekCount = 0;

        timelineCells.forEach(function (cell) {
            const span = Number(cell.getAttribute("colspan") || "1");
            weekCount += span;
        });

        if (weekCount !== 14) {
            const taskName = cells[0].textContent.trim();

            console.warn(
                "Gantt row does not add up to 14 weeks:",
                taskName,
                "Current total:",
                weekCount
            );
        }
    });
}