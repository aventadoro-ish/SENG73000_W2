// wait for the HTML page to finish loading
document.addEventListener("DOMContentLoaded", function () {

    // number of table rows displayed on each page
    const ROWS_PER_PAGE = 15;

    // ELEVATOR REQUEST TABLE
    // retrieve the request search box
    const requestSearch = document.getElementById("requestSearch");

    // retrieve the request-status dropdown
    const requestStatusFilter = document.getElementById("requestStatusFilter");

    // retrieve the paragraph used to show the result count
    const requestResultCount = document.getElementById("requestResultCount");

    const requestPreviousPage = document.getElementById("requestPreviousPage");
    const requestNextPage = document.getElementById("requestNextPage");

    // retrieve the text that displays the current page
    const requestPageDisplay = document.getElementById("requestPageDisplay");

    // retrieve every data row from the request table
    // [data-status] prevents an empty-table message from being included
    const requestRows = document.querySelectorAll("#requestTableBody tr[data-status]");

    // begin on the first request page
    let requestCurrentPage = 1;


    // update the request table using the search, filter, and page
    function updateRequestTable() {

        // retrieve what the user typed
        const searchText = requestSearch.value.trim().toLowerCase();

        // retrieve the selected request status
        const selectedStatus = requestStatusFilter.value;

        // this array will hold every row that passes the filters
        const matchingRows = [];


        // check every request row
        requestRows.forEach(function (row) {

            // retrieve all visible text from the current row
            const rowText = row.textContent.toLowerCase();

            // retrieve the row's data-status value
            // use an empty string if the attribute is missing
            const rowStatus = row.dataset.status || "";

            // true when the row contains the search text
            const matchesSearch = rowText.includes(searchText);

            // true when "all" is selected or the statuses match
            const matchesStatus = selectedStatus === "all" || rowStatus === selectedStatus;

            // add the row to the array if both conditions are true
            if (matchesSearch && matchesStatus) 
            {
                matchingRows.push(row);
            }

            // temporarily hide every row as the correct page will be shown afterward
            row.hidden = true;
        });


        // calculate how many pages are required
        let totalPages = Math.ceil(matchingRows.length / ROWS_PER_PAGE);

        // keep one page displayed even when there are no matches
        if (totalPages === 0) {
            totalPages = 1;
        }

        // move back to the final valid page if necessary
        if (requestCurrentPage > totalPages) {
            requestCurrentPage = totalPages;
        }

        // calculate the first array position for the current page
        const startIndex = (requestCurrentPage - 1) * ROWS_PER_PAGE;

        // calculate the position where the page should stop
        const endIndex = startIndex + ROWS_PER_PAGE;


        // check every matching row and its array position
        matchingRows.forEach(function (row, index) {

            // show rows whose positions belong to this page
            if (index >= startIndex && index < endIndex) {
                row.hidden = false;
            }
        });


        // show how many total rows passed the search and filter
        requestResultCount.textContent = matchingRows.length + " of " + requestRows.length + " requests match";

        // display the current page and total number of pages
        requestPageDisplay.textContent = "Page " + requestCurrentPage + " of " + totalPages;

        // disable Previous while already on the first page
        requestPreviousPage.disabled = requestCurrentPage === 1;

        // disable Next while already on the final page
        requestNextPage.disabled = requestCurrentPage === totalPages;
    }


    // make sure all request-table controls exist
    if (requestSearch && requestStatusFilter && requestResultCount && requestPreviousPage && requestNextPage && requestPageDisplay) {

        // run whenever the user types in the search box
        requestSearch.addEventListener("input", function () {

            // return to page one for the new search
            requestCurrentPage = 1;

            // update the displayed rows
            updateRequestTable();
        });


        // run whenever the selected status changes
        requestStatusFilter.addEventListener("change", function () {

            // return to page one for the new filter
            requestCurrentPage = 1;

            // update the displayed rows
            updateRequestTable();
        });


        // move backward one page
        requestPreviousPage.addEventListener("click", function () {

            // only subtract if another page exists
            if (requestCurrentPage > 1) {
                requestCurrentPage--;
                updateRequestTable();
            }
        });


        // move forward one page
        requestNextPage.addEventListener(
            "click",
            function () {

                // the button becomes disabled on the final page,
                // so it is safe to add one here
                requestCurrentPage++;
                updateRequestTable();
            }
        );


        // display the first page when the website loads
        updateRequestTable();
    }

    // CAN MESSAGE TABLE
    // retrieve the CAN-message search box
    const CANSearch = document.getElementById("canSearch");

    // retrieve the CAN-direction dropdown
    const CANDirectionFilter = document.getElementById("canDirectionFilter");

    // retrieve the CAN result-count paragraph
    const CANResultCount = document.getElementById("canResultCount");

    // retrieve the CAN Previous button
    const CANPreviousPage = document.getElementById("CANPreviousPage");

    // retrieve the CAN Next button
    const CANNextPage = document.getElementById("CANNextPage");

    // retrieve the CAN page-number display
    const CANPageDisplay = document.getElementById("CANPageDisplay");

    // retrieve every data row from the CAN table
    const CANRows =
        document.querySelectorAll("#canTableBody tr[data-direction]");

    // begin on the first CAN page
    let CANCurrentPage = 1;


    // update the CAN table using the search, filter, and page
    function updateCANTable() {

        // retrieve and prepare the user's search text
        const searchText = CANSearch.value.trim().toLowerCase();

        // retrieve the selected CAN direction
        const selectedDirection = CANDirectionFilter.value;

        // this array will hold CAN rows that pass both filters
        const matchingRows = [];


        // check every CAN row
        CANRows.forEach(function (row) {

            // retrieve all visible text from the row
            const rowText = row.textContent.toLowerCase();

            // retrieve the row's data-direction value
            const rowDirection =
                row.dataset.direction || "";

            // true when the row contains the search text
            const matchesSearch = rowText.includes(searchText);

            // true when "all" is selected or directions match
            const matchesDirection = selectedDirection === "all" || rowDirection === selectedDirection;

            // save the row if it passes both filters
            if (matchesSearch && matchesDirection) {
                matchingRows.push(row);
            }

            // temporarily hide every CAN row
            row.hidden = true;
        });


        // calculate how many CAN pages are required
        let totalPages = Math.ceil(matchingRows.length / ROWS_PER_PAGE);

        // display one page even if there are no matching rows
        if (totalPages === 0) {
            totalPages = 1;
        }

        // prevent the current page from exceeding the last page
        if (CANCurrentPage > totalPages) {
            CANCurrentPage = totalPages;
        }


        // calculate the beginning position of the current page
        const startIndex = (CANCurrentPage - 1) * ROWS_PER_PAGE;

        // calculate the ending position of the current page
        const endIndex = startIndex + ROWS_PER_PAGE;


        // show only the rows belonging to the current page
        matchingRows.forEach(function (row, index) {

            if (index >= startIndex && index < endIndex) {
                row.hidden = false;
            }
        });


        // display the number of CAN rows that matched
        CANResultCount.textContent = matchingRows.length + " of " + CANRows.length + " CAN messages match";

        // display the current CAN page
        CANPageDisplay.textContent = "Page " + CANCurrentPage + " of " + totalPages;

        // disable Previous on the first page
        CANPreviousPage.disabled = CANCurrentPage === 1;

        // disable Next on the final page
        CANNextPage.disabled = CANCurrentPage === totalPages;
    }


    // make sure all CAN-table controls exist
    if (CANSearch && CANDirectionFilter && CANResultCount && CANPreviousPage && CANNextPage && CANPageDisplay) {

        // run whenever the CAN search text changes
        CANSearch.addEventListener("input", function () {

            // start the new search on page one
            CANCurrentPage = 1;

            // update the displayed CAN rows
            updateCANTable();
        });


        // run whenever the selected direction changes
        CANDirectionFilter.addEventListener("change", function () {
            // start the new filter on page one
            CANCurrentPage = 1;

            // update the displayed CAN rows
            updateCANTable();
        });


        // move backward one CAN page
        CANPreviousPage.addEventListener("click", function () {
            if (CANCurrentPage > 1) {
                CANCurrentPage--;
                updateCANTable();
            }
        });


        // move forward one CAN page
        CANNextPage.addEventListener("click", function () {
            CANCurrentPage++;
            updateCANTable();
        });


        // display the first CAN page when the website loads
        updateCANTable();
    }
});