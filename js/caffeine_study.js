const caffeineCsvUrl = "https://docs.google.com/spreadsheets/d/e/2PACX-1vTgrDQkB5m1RcpNN90XCUvQruYOJ66Cj6YGTelbLFGnH6iFecsvqMJTCHsVePQC3NKzp-nDvymWFakM/pub?gid=1029732592&single=true&output=csv";
const caffeineLogCsvUrl = "https://docs.google.com/spreadsheets/d/e/2PACX-1vTgrDQkB5m1RcpNN90XCUvQruYOJ66Cj6YGTelbLFGnH6iFecsvqMJTCHsVePQC3NKzp-nDvymWFakM/pub?gid=77153223&single=true&output=csv";


document.addEventListener("DOMContentLoaded", function () {
    loadCaffeineData();
    loadCaffeineLog();
});

function loadCaffeineData() {
    fetch(caffeineCsvUrl)
        .then(function (response) {
            return response.text();
        })
        .then(function (csvText) {
            const tableData = cleanTableData(parseCsv(csvText));

            buildCaffeineCards(tableData);
            buildCaffeineTable(tableData);
        })
        .catch(function () {
            document.getElementById("caffeine-card-container").innerHTML =
                "<p>Could not load caffeine summary cards.</p>";

            document.getElementById("caffeine-table-container").innerHTML =
                "<p>Could not load caffeine table.</p>";
        });
}

function buildCaffeineTable(tableData) {
    const tableContainer = document.getElementById("caffeine-table-container");

    let tableHTML = "<table class='caffeine-table'>";

    tableData.forEach(function (row, rowIndex) {
        tableHTML += "<tr>";

        row.forEach(function (cell) {
            if (rowIndex === 0) {
                tableHTML += "<th>" + cell + "</th>";
            } else {
                tableHTML += "<td>" + cell + "</td>";
            }
        });

        tableHTML += "</tr>";
    });

    tableHTML += "</table>";

    tableContainer.innerHTML = tableHTML;
}

function buildCaffeineCards(tableData) {
    const cardContainer = document.getElementById("caffeine-card-container");

    // if the card container is missing or there is no data, stop
    if (!cardContainer || tableData.length === 0) {
        return;
    }

    // first row contains the names
    const headerRow = tableData[0];

    // get names from the header row, skipping the first label column
    // also remove blank names so empty cards are not created
    const people = headerRow.slice(1).filter(function (person) {
        return person.trim() !== "";
    });

    // find the important rows by their first-column label
    const totalCaffeineRow = findRow(tableData, "Total Caffeine Consumed [g]");
    const efficiencyRow = findRow(tableData, "Caffeine Efficiency [g/L]");

    // if the rows could not be found, show an error instead of crashing
    if (!totalCaffeineRow || !efficiencyRow) {
        cardContainer.innerHTML =
            "<p>Could not find the caffeine summary rows.</p>";
        return;
    }

    let cardsHTML = "";

    // build one card per person
    people.forEach(function (person, index) {
        const caffeineValue = totalCaffeineRow[index + 1] || "";
        const efficiencyValue = efficiencyRow[index + 1] || "";

        cardsHTML += `
            <article class="caffeine-card">
                <h3>${person}</h3>
                <p><strong>Total Caffeine:</strong> ${caffeineValue} g</p>
                <p><strong>Efficiency:</strong> ${efficiencyValue} g/L</p>
            </article>
        `;
    });

    cardContainer.innerHTML = cardsHTML;
}

// global data storage for caffeine log
// can rebuild with re-fetching
let caffeineLogData = [];

// loads caffeine log from CSV
function loadCaffeineLog() {

    // fetch CSV
    fetch(caffeineLogCsvUrl)
        // convert response into text
        .then(function (response) {
            return response.text();
        })

        // parse and build text once loaded
        .then(function (csvText){
            // convert into 2D array
            // tableData[row][column]
            caffeineLogData = cleanTableData(parseCsv(csvText));

            // build dropdown filter
            buildPersonFilter(caffeineLogData);
            // build the entire log table
            buildCaffeineLogTable(caffeineLogData);
        })

        // if something goes wrong, show error
        .catch(function () {
            document.getElementById("caffeine-log-container").innerHTML = "<p>Could not load caffine log, gg</p>";
        })
}

// builds person filter dropdown
function buildPersonFilter(tableData) {
    
    // find dropdown
    const filter = document.getElementById("personFilter");

    // if not empty
    if(!filter || tableData.length < 2){
        return;
    }

    // first row contains column names
    const headerRow = tableData[0];
    const nameColumnIndex = headerRow.indexOf("Name");

    if (nameColumnIndex === -1){
        return;
    }

    // array for names
    const people = [];

    // start at 1 because row 0 is header
    for (let i = 1; i <tableData.length; i++) {
        // grab persons name from current row
        const person = tableData[i][nameColumnIndex];

        // if not exist, add it
        if (person && !people.includes(person)){
            people.push(person);
        }
    }

    // sort alphabetically
    people.sort();

    // add each person as an option in dropdown
    people.forEach(function (person) {
        // create new option element
        const option = document.createElement("option");
        option.value = person;
        option.textContent = person;
        // add option to dropdown
        filter.appendChild(option);
    });

    // rebuild table when user changes dropdown
    filter.addEventListener("change", function (){
        buildCaffeineLogTable(caffeineLogData);
    });
}

// builds a large table with log entries
function buildCaffeineLogTable(tableData) {

    // find where the table will be placed
    const tableContainer = document.getElementById("caffeine-log-container");

    // find dropdown filter (for filtering by person)
    const filter = document.getElementById("personFilter");

    // if the container does not exist, stop
    if(!tableContainer || tableData.length === 0) {
        return;
    }

    // if no dropdown, default to showing all
    const selectedPerson = filter ? filter.value : "all";

    // first row
    const headerRow = tableData[0];

    // find which column contains the person's name
    const nameColumnIndex = headerRow.indexOf("Name");

    // if there is no "Name" column, show an error and stop
    if (nameColumnIndex === -1) {
        tableContainer.innerHTML = "<p>Could not find the Name column in the caffeine log.</p>";
        return;
    }


    // start building the actual table
    let tableHTML = "<table class='caffeine-table caffeine-log-table'>";

    // table header
    tableHTML += "<thead><tr>";
    
    headerRow.forEach(function (header) {
        tableHTML += "<th>" + header + "</th>";
    });

    tableHTML += "</tr></thead>";

    // build table body
    tableHTML += "<tbody>";

    // loop through the table, start at 1 because 0 is header row
    for (let i = 1; i < tableData.length; i++) {
        const row = tableData[i];

        // skip empty rows
        if (!row || row.length === 0) {
            continue;
        }

        // if a person is selected (not all), skip the rows that do not match that person
        if(selectedPerson !== "all" && row[nameColumnIndex] !== selectedPerson){
            continue;
        }

        // start one table row
        tableHTML += "<tr>";

        // add every cell in this row
        row.forEach(function (cell) {
            tableHTML += "<td>" + cell + "</td>";
        });
        // end row
        tableHTML += "</tr>";
    }

    // close body and table itself
    tableHTML += "</tbody>";
    tableHTML += "</table>";

    // put table back
    tableContainer.innerHTML = tableHTML;

}

// note: functions below are made with ChatGPT as I'm not making my own CSV parser
// parses CSV text into a 2D array
// this version handles oddities - commas, quotes, and line breaks inside quoted cells.
function parseCsv(csvText) {
    const rows = [];
    let currentRow = [];
    let currentCell = "";
    let insideQuotes = false;

    for (let i = 0; i < csvText.length; i++) {
        const character = csvText[i];
        const nextCharacter = csvText[i + 1];

        if (character === '"' && insideQuotes && nextCharacter === '"') {
            currentCell += '"';
            i++;
        } else if (character === '"') {
            insideQuotes = !insideQuotes;
        } else if (character === "," && !insideQuotes) {
            currentRow.push(currentCell);
            currentCell = "";
        } else if ((character === "\n" || character === "\r") && !insideQuotes) {
            if (character === "\r" && nextCharacter === "\n") {
                i++;
            }

            currentRow.push(currentCell);

            if (currentRow.some(function (cell) {
                return cell.trim() !== "";
            })) {
                rows.push(currentRow);
            }

            currentRow = [];
            currentCell = "";
        } else {
            currentCell += character;
        }
    }

    if (currentCell !== "" || currentRow.length > 0) {
        currentRow.push(currentCell);

        if (currentRow.some(function (cell) {
            return cell.trim() !== "";
        })) {
            rows.push(currentRow);
        }
    }

    return rows;
}

// Another ChatGPT-made function
// cleans Google Sheets CSV data.
// Google Sheets sometimes exports extra blank rows/columns from unused cells.
function cleanTableData(tableData) {
    if (!tableData || tableData.length === 0) {
        return [];
    }

    const headerRow = tableData[0];

    // only keep columns where the header cell is not blank
    const columnsToKeep = [];

    headerRow.forEach(function (header, index) {
        if (header.trim() !== "") {
            columnsToKeep.push(index);
        }
    });

    if (columnsToKeep.length === 0) {
        return tableData;
    }

    // rebuild each row using only useful columns
    const cleanedRows = tableData.map(function (row) {
        return columnsToKeep.map(function (columnIndex) {
            return row[columnIndex] ? row[columnIndex].trim() : "";
        });
    });

    // remove fully empty rows
    return cleanedRows.filter(function (row) {
        return row.some(function (cell) {
            return cell.trim() !== "";
        });
    });
}

// cleans up labels before comparing them.
// this helps if Google Sheets exports weird spacing or line breaks.
function normalizeLabel(text) {
    return String(text).replace(/\s+/g, " ").trim();
}

// finds a row by checking the first cell in each row.
// example: findRow(tableData, "Total Caffeine Consumed [g]")
function findRow(tableData, rowName) {
    return tableData.find(function (row) {
        return normalizeLabel(row[0]) === normalizeLabel(rowName);
    });
}