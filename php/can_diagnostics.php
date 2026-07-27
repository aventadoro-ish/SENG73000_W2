<?php
    // Elevator Diagnostics Page
    // 1. Allow only admins to access the page
    // 2. Read elevator_requests from the DB
    // 3. Decode each request into its CAN protocol info (via lookup array)
    // 4. Render as a searchable/paginated HTML table (JS handles the UI part)

    session_start();

    require_once __DIR__ . '/db.php'; // gives $pdo

    // User login
    if (!isset($_SESSION['logged_in']) || $_SESSION['logged_in'] !== true) {
        header("Location: ../html/login.html");
        exit("Please log in.");
    }

    // Check if user is an admin
    if (($_SESSION['user_role'] ?? '') !== 'admin') {
        http_response_code(403);
        exit("Access denied; admin permissions required for this page.");
    }

    // Lookup table: source_controller -> CAN message info
    // Add a new entry here any time a new controller/message type is added.
    $canMessages = [
        "web_floor_station" => [
            "can_id"      => "0x100",
            "transmitter" => "SC",
            "recipient"   => "EC",
            "message"     => "Supervisor Floor Request",
        ],
        "web_car_controller" => [
            "can_id"      => "0x200",
            "transmitter" => "CC",
            "recipient"   => "SC",
            "message"     => "Car Controller Floor Request",
        ],
        "elevator_controller" => [
            "can_id"      => "0x101",
            "transmitter" => "EC",
            "recipient"   => "ALL",
            "message"     => "Elevator Position Broadcast",
        ],
        "floor_1" => [
            "can_id"      => "0x201",
            "transmitter" => "F1",
            "recipient"   => "SC",
            "message"     => "F1 Cart Request",
        ],
        "floor_2" => [
            "can_id"      => "0x202",
            "transmitter" => "F2",
            "recipient"   => "SC",
            "message"     => "F2 Cart Request",
        ],
        "floor_3" => [
            "can_id"      => "0x203",
            "transmitter" => "F3",
            "recipient"   => "SC",
            "message"     => "F3 Cart Request",
        ],
        "floor_4" => [
            "can_id"      => "0x204",
            "transmitter" => "F4",
            "recipient"   => "SC",
            "message"     => "F4 Cart Request",
        ],
        "floor_5" => [
            "can_id"      => "0x205",
            "transmitter" => "F5",
            "recipient"   => "SC",
            "message"     => "F5 Cart Request",
        ],
        "floor_6" => [
            "can_id"      => "0x206",
            "transmitter" => "F6",
            "recipient"   => "SC",
            "message"     => "F6 Cart Request",
        ],
    ];

    // When the request's source_controller has no matching entry from the options above,
    // so a bad/unexpected value doesn't throw a PHP warning or blank row.
    $unknownMessage = [
        "can_id"      => "?",
        "transmitter" => "?",
        "recipient"   => "?",
        "message"     => "Unknown source_controller",
    ];

    $elevatorRequests = [];
    $loadError = '';

    // Organize table by time/date
    try {
        $query = "
            SELECT
                elevator_request_id,
                requested_floor,
                source_controller,
                request_status,
                requested_at
            FROM elevator_requests
            ORDER BY requested_at DESC 
        ";

        $statement = $pdo->prepare($query);
        $statement->execute();

        $elevatorRequests = $statement->fetchAll(PDO::FETCH_ASSOC);

    } catch (PDOException $e) {
        error_log($e->getMessage());
        $loadError = "Could not load elevator requests right now. Please try again shortly.";
    }
?>
<!DOCTYPE html>
<html lang="en">

<head>
    <title>LiF Team - Diagnostics</title>
    <meta charset="UTF-8">
    <link rel="icon" href="../media/icons/LiF_icon.ico" type="image/x-icon">

    <meta name="author" content="Rita Yevtushenko">
    <meta name="description" content="Secured diagnostics table.">
    <meta name="robots" content="noindex, nofollow">
    <meta http-equiv="Pragma" content="no-cache">

    <link rel="stylesheet" href="../css/base.css">
    <link rel="stylesheet" href="../css/members.css">
    <link rel="stylesheet" href="../css/can.css">
    <link rel="stylesheet" href="../css/diagnostics.css">
</head>

<body>
    <div id="navbar-placeholder" data-root="../"></div>
    <script src="../js/navbar.js"></script>

    <main class="page-wrapper">
        <section class="intro-section CANprotocol-intro">
            <div class="intro-text">
                <p class="section-label">CAN Communication</p>
                <h1>CAN Diagnostics</h1>
                <p>
                    Welcome to the diagnostics page. Here you can see the elevator
                    requests recorded in the database, decoded into their CAN
                    protocol message equivalents.
                </p>
                <p class="intro-description">
                    Need help understanding CAN IDs and bit layouts?
                    <a href="can_protocol_reference.php">View the CAN Protocol Reference &rarr;</a>
                </p>
            </div>
        </section>

        <div class="container">
            <h4 class="mt-2">Elevator Request Log</h4>
            <hr>

            <?php if ($loadError): ?>
            <p class="CANprotocol-error"><?= htmlspecialchars($loadError) ?></p>
            <?php endif; ?>

            <input type="text" id="diagnosticsSearch" placeholder="Search requests...">

            <div class="CANprotocol-table-wrapper">
                <table class="CANprotocol-table" id="diagnosticsTable">
                    <thead>
                        <tr>
                            <th>Request ID</th>
                            <th>Requested Floor</th>
                            <th>CAN ID</th>
                            <th>Message</th>
                            <th>Transmitter</th>
                            <th>Recipient</th>
                            <th>Status</th>
                            <th>Requested At</th>
                        </tr>
                    </thead>
                    <tbody>
                        <?php foreach ($elevatorRequests as $request): ?>
                        <?php
                                $decoded = $canMessages[$request["source_controller"]] ?? $unknownMessage;
                            ?>
                        <tr>
                            <td><?= htmlspecialchars($request["elevator_request_id"]) ?></td>
                            <td><?= htmlspecialchars($request["requested_floor"]) ?></td>
                            <td><?= htmlspecialchars($decoded["can_id"]) ?></td>
                            <td><?= htmlspecialchars($decoded["message"]) ?></td>
                            <td><?= htmlspecialchars($decoded["transmitter"]) ?></td>
                            <td><?= htmlspecialchars($decoded["recipient"]) ?></td>
                            <td><?= htmlspecialchars($request["request_status"]) ?></td>
                            <td><?= htmlspecialchars($request["requested_at"]) ?></td>
                        </tr>
                        <?php endforeach; ?>

                        <?php if (!$elevatorRequests && !$loadError): ?>
                        <tr>
                            <td colspan="8">No elevator requests found.</td>
                        </tr>
                        <?php endif; ?>
                    </tbody>
                </table>
            </div>

            <div id="diagnosticsPagination" class="CANprotocol-pagination"></div>
        </div>
    </main>

    <script src="../js/diagnostics.js"></script>
</body>

</html>