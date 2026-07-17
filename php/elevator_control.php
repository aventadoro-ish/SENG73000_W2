<?php
session_start();

// DB connection
require_once __DIR__ . '/db.php';

if (!isset($_SESSION['logged_in']) || $_SESSION['logged_in'] !== true) {
    header("Location: ../html/login.html");
    exit;
}

// elevator_control JS sends a POST request when an elevator button is pressed
// read it here
if($_SERVER['REQUEST_METHOD'] === 'POST') {
    // tell JS that PHP will return a JSON file instead of HTML:
    header('Content-Type: application/json');

    // receive JS values
    $submittedFloor = $_POST['requested_floor'] ?? '';
    $sourceController = $_POST['source_controller'] ?? '';

    // temporary test response
    echo json_encode([
        'success' => true,
        'requested_floor' => $submittedFloor,
        'source_controller' => $sourceController
    ]);

    // don't render the rest of elevator_control.php
    exit;
}















$safeUsername = htmlspecialchars($_SESSION['username'] ?? 'Member', ENT_QUOTES, 'UTF-8');
?>

<!DOCTYPE html>
<html lang="en">

<head>
    <title>LiF Team - Elevator Control</title>
    <meta charset="UTF-8">
    <link rel="icon" href="../media/icons/LiF_icon.ico" type="image/x-icon">

    <meta name="author" content="Nick Kapuka">
    <meta name="description" content="Protected elevator control interface for the LiF Team website.">
    <meta name="robots" content="noindex, nofollow">
    <meta http-equiv="Pragma" content="no-cache">

    <link rel="stylesheet" href="../css/base.css">
    <link rel="stylesheet" href="../css/elevator_control.css">
</head>

<body>
    <!-- nav bar -->
    <div id="navbar-placeholder" data-root="../"></div>
    <script src="../js/navbar.js"></script>

    <main class="page-wrapper">
        <section class="intro-section elevator-control-intro" id="page_top">
            <div class="intro-text">
                <p class="section-label">Protected Control Panel</p>

                <h1>Elevator Control Room</h1>

                <p class="intro-description">
                    Welcome, <?php echo $safeUsername; ?>. This page allows you, yes YOU, to control the humble elevator
                    platform!
                    The floor stations can request the car to go to them, while the car controller can send the cab to
                    any floor.
                </p>

                <div class="button-row">
                    <a href="members.php" class="button secondary-button">Back to Members</a>
                    <a href="logout.php" class="button primary-button">Log Out</a>
                </div>
            </div>

            <aside class="summary-card">
                <h2>Control Status</h2>
                <p><strong>Mode:</strong> Elevator control</p>
                <p><strong>Floors:</strong> 3</p>
                <p><strong>System:</strong> Floor calls + car controller</p>
            </aside>
        </section>

        <!-- main page -->
        <section class="elevator-game-panel">
            <div class="control-hud">
                <div>
                    <p class="section-label">Simulation display</p>
                    <h2>Cab Status</h2>
                </div>

                <div class="hud-readout">
                    <span class="hud-label">Current Floor</span>
                    <span id="currentFloorDisplay" class="hud-value">1</span>
                </div>

                <div class="hud-readout">
                    <span class="hud-label">Last Command</span>
                    <span id="lastCommandDisplay" class="hud-value">System idle</span>
                </div>
            </div>

            <div class="elevator-layout">
                <section class="building-tower">
                    <div class="floor-row" data-floor="3">
                        <div class="floor-label">
                            <span>Floor 3</span>
                            <small>Station F3</small>
                        </div>

                        <div class="shaft">
                            <div class="floor-line"></div>
                            <div class="elevator-car" id="elevatorCar">
                                <span class="car-screen">1</span>
                                <span class="car-door"></span>
                            </div>
                        </div>

                        <button class="floor-request-button" data-floor="3">
                            Request Car
                        </button>
                    </div>

                    <div class="floor-row" data-floor="2">
                        <div class="floor-label">
                            <span>Floor 2</span>
                            <small>Station F2</small>
                        </div>

                        <div class="shaft">
                            <div class="floor-line"></div>
                        </div>

                        <button class="floor-request-button" data-floor="2">
                            Request Car
                        </button>
                    </div>

                    <div class="floor-row active-floor" data-floor="1">
                        <div class="floor-label">
                            <span>Floor 1</span>
                            <small>Station F1</small>
                        </div>

                        <div class="shaft">
                            <div class="floor-line"></div>
                        </div>

                        <button class="floor-request-button" data-floor="1">
                            Request Car
                        </button>
                    </div>
                </section>

                <aside class="car-controller">
                    <p class="section-label">Car Controller</p>
                    <h2>Cab Destination</h2>

                    <p>
                        These buttons will behave as the buttons inside the elevator cab. Select a destination
                        floor to move the car in the visual demo.
                    </p>

                    <div class="car-button-stack">
                        <button class="car-floor-button" data-floor="3">Go to Floor 3</button>
                        <button class="car-floor-button" data-floor="2">Go to Floor 2</button>
                        <button class="car-floor-button active-car-button" data-floor="1">Go to Floor 1</button>
                    </div>

                    <div class="controller-note">
                        <strong>Note:</strong> This does not control hardware yet. It only updates the webpage visuals.
                    </div>
                </aside>
            </div>
        </section>

        <p class="top-link">
            <a href="#page_top">Go to the top of this page</a>
        </p>
    </main>

    <footer class="site-footer">
        <p>Copyright &copy; 2026 Nick Kapuka</p>
    </footer>

    <script src="../js/elevator_control.js"></script>
</body>

</html>