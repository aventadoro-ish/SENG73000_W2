<?php
    // Elevator Diagnostics Page
    // 1. Allow only admins to access the page
    // 2. Read elevator_requests from the DB
    // 3. Decode each request into its CAN protocol info (via lookup array)
    // 4. Render as a searchable/paginated HTML table, take a peek at diagnostics.js

    session_start();

    require_once __DIR__ . '/db.php'; 

    if (!isset($_SESSION['logged_in']) || $_SESSION['logged_in'] !== true) {
        header("Location: ../html/login.html");
        exit("Please log in.");
    }

    if (($_SESSION['user_role'] ?? '') !== 'admin') {
        http_response_code(403);
        exit("Access denied; admin permissions required for this page.");
    }

    //   "en_floor"   -> bit 2 = enable bit, bits 1-0 = floor number (0-3)
    //   "single_bit" -> bit 0 = request bit, no enable bit
    $canMessages = [
        "web_floor_station" => [
            "can_id"      => "0x100",
            "transmitter" => "SC",
            "recipient"   => "EC",
            "message"     => "Supervisor Floor Request",
            "bit_layout"  => "en_floor",
            "en_label"    => "SC_EN",
            "field_label" => "SC_FloorReq",
        ],
        "web_car_controller" => [
            "can_id"      => "0x200",
            "transmitter" => "CC",
            "recipient"   => "SC",
            "message"     => "Car Controller Floor Request",
            "bit_layout"  => "en_floor",
            "en_label"    => "CC_EN",
            "field_label" => "CC_FloorReq",
        ],
        "elevator_controller" => [
            "can_id"      => "0x101",
            "transmitter" => "EC",
            "recipient"   => "ALL",
            "message"     => "Elevator Position Broadcast",
            "bit_layout"  => "en_floor",
            "en_label"    => "EC_EN",
            "field_label" => "EC_Position",
        ],
        "floor_1" => [
            "can_id"      => "0x201",
            "transmitter" => "F1",
            "recipient"   => "SC",
            "message"     => "F1 Cart Request",
            "bit_layout"  => "single_bit",
            "en_label"    => null,
            "field_label" => "F1_Req",
        ],
        "floor_2" => [
            "can_id"      => "0x202",
            "transmitter" => "F2",
            "recipient"   => "SC",
            "message"     => "F2 Cart Request",
            "bit_layout"  => "single_bit",
            "en_label"    => null,
            "field_label" => "F2_Req",
        ],
        "floor_3" => [
            "can_id"      => "0x203",
            "transmitter" => "F3",
            "recipient"   => "SC",
            "message"     => "F3 Cart Request",
            "bit_layout"  => "single_bit",
            "en_label"    => null,
            "field_label" => "F3_Req",
        ],
        "floor_4" => [
            "can_id"      => "0x204",
            "transmitter" => "F4",
            "recipient"   => "SC",
            "message"     => "F4 Cart Request",
            "bit_layout"  => "single_bit",
            "en_label"    => null,
            "field_label" => "F4_Req",
        ],
        "floor_5" => [
            "can_id"      => "0x205",
            "transmitter" => "F5",
            "recipient"   => "SC",
            "message"     => "F5 Cart Request",
            "bit_layout"  => "single_bit",
            "en_label"    => null,
            "field_label" => "F5_Req",
        ],
        "floor_6" => [
            "can_id"      => "0x206",
            "transmitter" => "F6",
            "recipient"   => "SC",
            "message"     => "F6 Cart Request",
            "bit_layout"  => "single_bit",
            "en_label"    => null,
            "field_label" => "F6_Req",
        ],
    ];

    // Fallback used when a request's source_controller has no matching entry above,
    // so a bad/unexpected value doesn't throw a PHP warning or blank row.
    $unknownMessage = [
        "can_id"      => "?",
        "transmitter" => "?",
        "recipient"   => "?",
        "message"     => "Unknown source_controller",
        "bit_layout"  => null,
        "en_label"    => null,
        "field_label" => null,
    ];

    
     //Decoded messages
    function encodeCanByte(array $decoded, $requestedFloor): array {
        if ($decoded["bit_layout"] === "en_floor") {
            $floor = (int) $requestedFloor & 0b11; // 2 bits => 0-3
            $byte = (1 << 2) | $floor;              // EN at bit 2, floor at bits 1-0
            $bits = sprintf(
                "%s=1, %s=%02s (%d)",
                $decoded["en_label"],
                $decoded["field_label"],
                decbin($floor),
                $floor
            );
            return ["hex" => sprintf("0x%02X", $byte), "bits" => $bits];
        }

        if ($decoded["bit_layout"] === "single_bit") {
            $byte = 1; // bit 0 set = request active
            $bits = sprintf("%s=1", $decoded["field_label"]);
            return ["hex" => sprintf("0x%02X", $byte), "bits" => $bits];
        }

        return ["hex" => "?", "bits" => "n/a"];
    }

    
    // isTrue = if DB value is non-null 
    function yesNo(bool $isTrue): string {
        return $isTrue ? "Yes" : "No";
    }

    $elevatorRequests = [];
    $loadError = '';

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


    $requestDetails = [];
    $detailsLoadError = '';

    try {
        $detailsQuery = "SELECT * FROM elevator_requests ORDER BY requested_at DESC";

        $detailsStatement = $pdo->prepare($detailsQuery);
        $detailsStatement->execute();

        $requestDetails = $detailsStatement->fetchAll(PDO::FETCH_ASSOC);

    } catch (PDOException $e) {
        error_log($e->getMessage());
        $detailsLoadError = "Could not load request details right now. Please try again shortly.";
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
    <link rel="stylesheet" href="../css/CANprotocol.css">
    <link rel="stylesheet" href="../css/diagnostics-additions.css">
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
                    <a href="../html/CAN_protocol.html">View the CAN Protocol Reference &rarr;</a>
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
                            <th>Raw Data (Byte 0)</th>
                            <th>Decoded Bits</th>
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
                                $canByte = encodeCanByte($decoded, $request["requested_floor"]);
                            ?>
                            <tr>
                                <td><?= htmlspecialchars($request["elevator_request_id"]) ?></td>
                                <td><?= htmlspecialchars($request["requested_floor"]) ?></td>
                                <td><?= htmlspecialchars($decoded["can_id"]) ?></td>
                                <td><?= htmlspecialchars($canByte["hex"]) ?></td>
                                <td><?= htmlspecialchars($canByte["bits"]) ?></td>
                                <td><?= htmlspecialchars($decoded["message"]) ?></td>
                                <td><?= htmlspecialchars($decoded["transmitter"]) ?></td>
                                <td><?= htmlspecialchars($decoded["recipient"]) ?></td>
                                <td><?= htmlspecialchars($request["request_status"]) ?></td>
                                <td><?= htmlspecialchars($request["requested_at"]) ?></td>
                            </tr>
                        <?php endforeach; ?>

                        <?php if (!$elevatorRequests && !$loadError): ?>
                            <tr>
                                <td colspan="10">No elevator requests found.</td>
                            </tr>
                        <?php endif; ?>
                    </tbody>
                </table>
            </div>

            <div id="diagnosticsPagination" class="CANprotocol-pagination"></div>

            <h4 class="mt-2">Request Details</h4>
            <hr>
            <p class="intro-description">
                Request type, requesting account, source, and approval/completion status
                for each request — separate from the CAN translation table above.
            </p>

            <?php if ($detailsLoadError): ?>
                <p class="CANprotocol-error"><?= htmlspecialchars($detailsLoadError) ?></p>
            <?php endif; ?>

            <input type="text" id="requestDetailsSearch" placeholder="Search request details...">

            <div class="CANprotocol-table-wrapper">
                <table class="CANprotocol-table" id="requestDetailsTable">
                    <thead>
                        <tr>
                            <th>Request ID</th>
                            <th>Request Type</th>
                            <th>Requested Floor</th>
                            <th>Requested By (User ID)</th>
                            <th>Source Controller</th>
                            <th>Status</th>
                            <th>Requested At</th>
                            <th>Accepted At</th>
                            <th>Approved</th>
                            <th>Completed At</th>
                            <th>Succeeded</th>
                            <th>Failure Reason</th>
                            <th>Failed</th>
                        </tr>
                    </thead>
                    <tbody>
                        <?php foreach ($requestDetails as $detail): ?>
                            <tr>
                                <td><?= htmlspecialchars($detail["elevator_request_id"]) ?></td>
                                <td><?= htmlspecialchars($detail["request_type"]) ?></td>
                                <td><?= htmlspecialchars($detail["requested_floor"]) ?></td>
                                <td><?= htmlspecialchars($detail["requested_by_user_id"] ?? "—") ?></td>
                                <td><?= htmlspecialchars($detail["source_controller"]) ?></td>
                                <td><?= htmlspecialchars($detail["request_status"]) ?></td>
                                <td><?= htmlspecialchars($detail["requested_at"]) ?></td>
                                <td><?= htmlspecialchars($detail["accepted_at"] ?? "—") ?></td>
                                <td><?= htmlspecialchars(yesNo($detail["accepted_at"] !== null)) ?></td>
                                <td><?= htmlspecialchars($detail["completed_at"] ?? "—") ?></td>
                                <td><?= htmlspecialchars(yesNo($detail["completed_at"] !== null)) ?></td>
                                <td><?= htmlspecialchars($detail["failure_reason"] ?? "—") ?></td>
                                <td><?= htmlspecialchars(yesNo($detail["failure_reason"] !== null)) ?></td>
                            </tr>
                        <?php endforeach; ?>

                        <?php if (!$requestDetails && !$detailsLoadError): ?>
                            <tr>
                                <td colspan="13">No request details found.</td>
                            </tr>
                        <?php endif; ?>
                    </tbody>
                </table>
            </div>

            <div id="requestDetailsPagination" class="CANprotocol-pagination"></div>

            <div class="button-row">
                <a href="members.php" class="button secondary-button">Back to Members</a>
                <a href="logout.php" class="button primary-button">Log Out</a>
            </div>
        </div>
    </main>

    <script src="../js/diagnostics.js"></script>
</body>
</html>