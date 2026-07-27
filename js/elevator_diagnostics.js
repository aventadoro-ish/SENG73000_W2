// wait until the page's HTML has completely loaded
document.addEventListener("DOMContentLoaded", function() {

// retrieve the request search box using ID
const requestSearch = document.getElementById("requestSearch");
// retrieve request status dropdown
const requestStatusFilter = document.getElementById("requestStatusFilter");
// retrieve 
const requestResultCount = document.getElementById("requestResultCount");

// retrieve every row from the elevator request table
const requestRows = document.querySelectorAll("#requestTableBody tr");

// search and filter the elevator requests rows (part of searching)
function filterRequestRows () {
    // convert the search to lowercase for easier comparison
    const searchText = requestSearch.ariaValueMax.trim.toLowerCase();
    const selectedStatus = requestStatusFilter.value;

    let visibleRows = 0;

    requestRows.forEach(function (row) {
        // contains the text from every table cell in this row
        const rowText = row.dataset.status || "";

        const matchesSearch = rowText.includes(searchText);

        const matchesStatus = selectedStatus === "all" || rowStatus === selectedStatus;

        // only show the row if it passes both filters


    })
}





















































































})