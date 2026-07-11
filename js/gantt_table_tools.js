/*
 ---------- Gantt table script----------
this script keeps the source HTML manageable while improving the visual table grid by replacing colspan with td

Source HTML can use:
    <td colspan="8"></td>

After the page loads, this script converts that blank filler cell into:
    <td></td><td></td><td></td>...
*/

document.addEventListener("DOMContentLoaded", function () {
    expandBlankGanttCells();
    checkGanttRows();
});


// expands blank colspan cells into real empty table cells
function expandBlankGanttCells() {
    // find the Gantt table
    const ganttTable = document.getElementById("Gantt");

    // stop if the table is not on this page
    if (!ganttTable) {
        return;
    }

    // stop if this script already ran
    if (ganttTable.classList.contains("gantt-expanded")) {
        return;
    }

    // mark the table as expanded
    ganttTable.classList.add("gantt-expanded");

    // find all rows in the Gantt table body
    const rows = ganttTable.querySelectorAll("tbody tr");

    rows.forEach(function (row, rowIndex) {
        // skip the phase rows (3 at the top)
        // N/A | Phase 1 | Phase 2 | Phase 3
        if (rowIndex === 0) {
            return;
        }

        // skip major section rows
        // 1. Project plan, 2. Raspberry Pi, etc.
        if (row.classList.contains("gantt-section-row")) {
            return;
        }

        // convert row.children into a real array
        const cells = Array.from(row.children);

        // first cell is the task name, so timeline cells start after it
        const timelineCells = cells.slice(1);

        timelineCells.forEach(function (cell) {
            // only work on normal <td> cells
            if (cell.tagName !== "TD") {
                return;
            }

            // get the colspan number; if none exists, default to 1
            const span = Number(cell.getAttribute("colspan") || "1");

            // if the cell has no useful colspan, clean it up and stop
            if (span <= 1) {
                cell.removeAttribute("colspan");
                return;
            }

            // only expand blank filler cells.
            // do not expand coloured task/status cells.
            if (!isBlankFillerCell(cell)) {
                return;
            }

            // create one real blank <td> for each week this blank cell was spanning
            for (let i = 0; i < span; i++) {
                const newCell = document.createElement("td");
                newCell.classList.add("gantt-empty-cell");

                // insert the new cell before the old blank colspan cell
                row.insertBefore(newCell, cell);
            }

            // remove the old large blank colspan cell
            cell.remove();
        });
    });
}


// checks if a cell is just blank filler space
function isBlankFillerCell(cell) {
    // if the cell contains text, it is not blank filler
    if (cell.textContent.trim() !== "") {
        return false;
    }

    // these classes (also colour the chart) mean the cell is a real task/status bar
    const statusClasses = ["completed", "ongoing", "behind", "planned", "omitted"];
    // check if classes present (not empty; false)
    for (let i = 0; i < statusClasses.length; i++) {
        if (cell.classList.contains(statusClasses[i])) {
            return false;
        }
    }

    // no text and no status class means it is blank filler
    return true;
}


// option debug helper
// it warns in the browser console if a task row does not add up to 14 weeks

/*
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
} */