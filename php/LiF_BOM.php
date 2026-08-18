<?php 

session_start();

require_once __DIR__ . '/db.php';

if (!isset($_SESSION['logged_in']) || $_SESSION['logged_in'] !== true) {
    header("Location: ../html/login.html");
    exit;
}

$statusCsvURL = 'https://docs.google.com/spreadsheets/d/e/2PACX-1vTgrDQkB5m1RcpNN90XCUvQruYOJ66Cj6YGTelbLFGnH6iFecsvqMJTCHsVePQC3NKzp-nDvymWFakM/pub?gid=1788837120&single=true&output=csv';

$username = htmlspecialchars((string) ($_SESSION['username'] ?? 'LiF_Member'), ENT_QUOTES, 'UTF-8');

$userRole = htmlspecialchars((string) ($_SESSION['user-role'] ?? 'user'), ENT_QUOTES, 'UTF-8');

?>


<!DOCTYPE html>
<html lang="en">

<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>LiF Team PCB Population Status</title>

    <link rel="icon" href="../media/icons/LiF_icon.ico" type="image/x-icon">

    <meta name="author" content="Nick Kapuka">
    <meta name="description" content="PCB BoM and board population status">
    <meta name="robots" contents="noindex, nofollow">
    <meta http-equiv="Pragma" content="no-cache">

    <link rel="stylesheet" href="../css/base.css">
    <link rel="stylesheet" href="../css/LiF_BOM.css">
</head>

<body>
    <!-- load navbar -->
    <div id="navbar-placeholder" data-root="../"></div>
    <script src="../js/navbar.js"></script>

    <main class="page-wrapper bom-page" id="bomPage"
        data-csv-url="<?php echo htmlspecialchars($statusCsvURL, ENT_QUOTES, 'UTF-8'); ?>">

        <section class="intro-section bom-intro" id="page-top">
            <div class="intro-text">
                <p class="section-label">PCB population tracker</p>
                <h1>Bill of Materials</h1>

                <p class="intro-description">
                    Welcome, <?php echo $username; ?>. This page loads the LiF PCB BoM from a CSV file
                    and dispalys the various contents related to the assembly of all three custom boards.
                </p>

                <div class="button-row">
                    <button class="button primary-button bom-reload-button" id="reloadBomButton" type="button">
                        Reload CSV data
                    </button>

                    <a href="members.php" class="button secondary-button">Return to members page</a>
                </div>
            </div>

            <aside class="summary-card bom-access-card">
                <h2>Data status</h2>
                <p><strong>Source:</strong> Google Sheets CSV Export</p>
                <p><strong>Board count:</strong> 3</p>
                <p><strong>User:</strong> <?php echo $username; ?></p>
                <p><strong>Account Type:</strong> <?php echo $userRole; ?></p>
                <p id="bomUpdatedAt"><strong>Loaded:</strong> Waiting for data</p>
            </aside>
        </section>

        <p class="bom-load-status" id="bomLoadStatus" role="status" aria-live="polite">
            Loading PCB population data...
        </p>

        <section class="bom-summary-section" aria-labelledby="bomSummaryHeading">
            <div class="bom-heading">
                <p class="section-label">Project Summary</p>
                <h2 id="bomSummaryHeading">Population Overview</h2>
                <p>
                    These values are calculated from the component rows in the published sheet.
                </p>
            </div>

            <div class="bom-summary-grid" id="bomSummaryCards">
                <article class="bom-summary-card is-loading">
                    <span>Loading</span>
                    <strong>---</strong>
                </article>
            </div>
        </section>

        <section class="board-status-section" aria-labelledby="boardStatusHeading">
            <div class="bom-heading">
                <p class="section-label">Board Progress</p>
                <h2 id="boardStatusHeading">PCB Population Status</h2>
                <p>
                    Each board card shows the completion value and timing information stored at the bottom of the CSV.
                </p>
            </div>

            <div class="board-status-grid" id="boardStatusCards">
                <p>Loading board status...</p>
            </div>
        </section>

        <section class="bom-table-section" aria-labelledby="bomTableHeading">
            <div class="bom-heading">
                <p class="section-label">Component Details</p>
                <h2 id="bomTableHeading">PCB Population Table</h2>
                <p>
                    Search by reference designator, DigiKey part number, bag number, or assigned team member.
                </p>
            </div>

            <div class="bom-controls">
                <label class="bom-filter bom-search-filter">
                    <span>Search Components</span>
                    <input type="search" id="bomSearch" placeholder="Example: R21, ESP32, bag 12..." autocomplete="off">
                </label>

                <label class="bom-filter">
                    <span>Board</span>
                    <select id="bomBoardFilter">
                        <option value="all">All Boards</option>
                        <option value="0">Board 1</option>
                        <option value="1">Board 2</option>
                        <option value="2">Board 3</option>
                    </select>
                </label>

                <label class="bom-filter">
                    <span>Population</span>
                    <select id="bomStatusFilter">
                        <option value="all">All Statuses</option>
                        <option value="populated">Populated</option>
                        <option value="incomplete">Incomplete</option>
                        <option value="dnp">Do Not Populate</option>
                    </select>
                </label>

                <label class="bom-filter">
                    <span>Assigned Person</span>
                    <select id="bomPersonFilter">
                        <option value="all">All People</option>
                    </select>
                </label>

                <p class="bom-result-count" id="bomResultCount" aria-live="polite"></p>
            </div>

            <div class="bom-table-wrapper" id="bomTableContainer">
                <p>Loading bill of materials...</p>
            </div>
        </section>

        <p class="top-link">
            <a href="#page_top">Go to the top of this page</a>
        </p>
    </main>

    <footer class="site-footer">
        <p>Copyright &copy; 2026 Nick Kapuka</p>
    </footer>

    <script src="../js/LiF_BOM.js"></script>
</body>

</html>