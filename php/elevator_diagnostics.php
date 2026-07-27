<?php

    session_start();

    // load DB connection
    require_once __DIR__ . '/db.php';

    if(!isset($_SESSION['logged_in']) || $_SESSION['logged_in'] != true) {  
        header("Location: ../html/login.html");
        exit("please log in");
    }

    // check if the user logged in is the LiF Admin account
    if(($_SESSION['user_role'] ?? '') !== 'admin') {
        // unauthorized access
        http_response_code(403);
        exit("Access denied; Admin perms required for entry to this page. Please return and login again.");
    }

    // empty arrays for HTML to load DB data into tables
    $elevatorRequests = [];
    $elevatorState = [];

    $loadError = '';

    try {
        // query the entire elevator_requests DB - this is for diagnostics so seeing the whole table makes sense
        // might have to ORDER BY or filter for WHERE later
        $query = "
            SELECT
                elevator_request_id,
                request_type,
                requested_floor,
                requested_by_user_id,
                source_controller,
                request_status,
                requested_at,
                accepted_at,
                completed_at,
                failure_reason
            FROM elevator_requests
            ORDER BY elevator_request_id DESC
        ";

        // prepare and pull the DB data into a PHP array
        $statement = $pdo->prepare($query);

        // send query to DB
        $result = $statement->execute();

        if($result) {
            // retrieve the data from the query
            $elevatorRequests = $statement->fetchAll(PDO::FETCH_ASSOC);
        }
        
        // query the entire CAN message log
        $query = "
            SELECT
                log_id,
                elevator_request_id,
                can_id,
                direction,
                raw_byte,
                dlc,
                source_controller,
                logged_at
            FROM can_message_log
            ORDER BY log_id DESC
        "; 

        // prepare and pull the DB data into a PHP array
        $statement = $pdo->prepare($query);

        // send query to DB
        $result = $statement->execute();

        if($result) {
            // retrieve the data from the query
            $canMessages = $statement->fetchAll(PDO::FETCH_ASSOC);
        }

    } catch (PDOException $e) {
        error_log($e->getMessage());
        $loadError = 'The diagnostic data could not be loaded from the database.';
    }

    // safely format database values before displaying them in HTML
    function displayValue($value) {
        if($value === null || $value === '') {
            return '---';
        }

        return htmlspecialchars((string) $value, ENT_QUOTES, 'UTF-8');
    }
?>

<!DOCTYPE html>
<html lang="en">

<head>
    <title>LiF Team - Elevator Diagnostic Page</title>
    <meta charset="UTF-8">
    <link rel="icon" href="../media/icons/LiF_icon.ico" type="image/x-icon">

    <meta name="author" content="Nick Kapuka">
    <meta name="description" content="Protected elevator diagnostics page for the LiF Team website.">
    <meta name="robots" content="noindex, nofollow">
    <meta http-equiv="Pragma" content="no-cache">

    <link rel="stylesheet" href="../css/base.css">
    <link rel="stylesheet" href="../css/elevator_diagnostics.css">
</head>

<body>
    <div id="navbar-placeholder" data-root="../"></div>
    <script src="../js/navbar.js"></script>

    <main class="page-wrapper diagnostics-page">
        <h1>Elevator Diagnostics</h1>

        <?php if($loadError !== '') { ?>
        <p><?php echo htmlspecialchars($loadError, ENT_QUOTES, 'UTF-8'); ?></p>
        <?php } ?>

        <section>
            <h2>Elevator Requests</h2>

            <div class="diagnostic-controls">
                <label class="diagnostics-filter">
                    <span>Search Requests</span>
                    <input type="search" id="requestSearch" placeholder="ID, floor, controller...">
                </label>

                <label class="diagnostics-filter">
                    <span>Request Status</span>

                    <select id="requestStatusFilter">
                        <option value="all">All Statuses</option>
                        <option value="pending">Pending</option>
                        <option value="accepted">Accepted</option>
                        <option value="completed">Completed</option>
                        <option value="failed">Failed</option>
                    </select>
                </label>

                <p class=diagnostic-result-count id="requestResultCount"></p>
            </div>

            <?php if(empty($elevatorRequests)) { ?>
            <p>No elevator requests were found.</p>
            <?php } else { ?>

            <div class="table-scroll">
                <table class="diagnostics-table requests-table">
                    <thead>
                        <tr>
                            <th>
                            <th>Request ID</th>
                            <th>Request Type</th>
                            <th>Requested Floor</th>
                            <th>Requested By User ID</th>
                            <th>Source Controller</th>
                            <th>Status</th>
                            <th>Requested At</th>
                            <th>Accepted At</th>
                            <th>Completed At</th>
                            <th>Failure Reason</th>
                        </tr>
                    </thead>
                    <tbody id="requestTableBody">
                        <?php foreach($elevatorRequests as $request) { ?>
                        <tr
                            data-status="<?php echo htmlspecialchars(strtolower((string) $request['request_status']), ENT_QUOTES, 'UTF-8'); ?>">
                            <td><?php echo displayValue($request['elevator_request_id']); ?></td>
                            <td><?php echo displayValue($request['request_type']); ?></td>
                            <td><?php echo displayValue($request['requested_floor']); ?></td>
                            <td><?php echo displayValue($request['requested_by_user_id']); ?></td>
                            <td><?php echo displayValue($request['source_controller']); ?></td>
                            <td>
                                <span class="diagnostic-badge"
                                    data-status="<?php echo htmlspecialchars($request['request_status'], ENT_QUOTES, 'UTF-8'); ?>">

                                    <?php echo displayValue($request['request_status']); ?>
                                </span>
                            </td>
                            <td><?php echo displayValue($request['requested_at']); ?></td>
                            <td><?php echo displayValue($request['accepted_at']); ?></td>
                            <td><?php echo displayValue($request['completed_at']); ?></td>
                            <td><?php echo displayValue($request['failure_reason']); ?></td>
                        </tr>
                        <?php } ?>
                    </tbody>
                </table>
            </div>
            <?php } ?>
        </section>

        <section>
            <h2>CAN Message Log</h2>

            <?php if(empty($canMessages)) { ?>
            <p>No CAN messages were found.</p>
            <?php } else { ?>
            <div class="table-scroll">
                <table class="diagnostics-table can-table">
                    <thead>
                        <tr>
                            <th>Log ID</th>
                            <th>Elevator Request ID</th>
                            <th>CAN ID</th>
                            <th>Direction</th>
                            <th>Raw Byte</th>
                            <th>DLC</th>
                            <th>Source Controller</th>
                            <th>Logged At</th>
                        </tr>
                    </thead>

                    <tbody>
                        <?php foreach($canMessages as $message) { ?>
                        <tr>
                            <td><?php echo displayValue($message['log_id']); ?></td>
                            <td><?php echo displayValue($message['elevator_request_id']); ?></td>
                            <td><?php echo displayValue($message['can_id']); ?></td>
                            <td>
                                <span class="diagnostic-badge"
                                    data-direction="<?php echo htmlspecialchars($message['direction'], ENT_QUOTES, 'UTF-8'); ?>">

                                    <?php echo displayValue($message['direction']); ?>
                                </span>
                            </td>
                            <td><?php echo displayValue($message['raw_byte']); ?></td>
                            <td><?php echo displayValue($message['dlc']); ?></td>
                            <td><?php echo displayValue($message['source_controller']); ?></td>
                            <td><?php echo displayValue($message['logged_at']); ?></td>
                        </tr>
                        <?php } ?>
                    </tbody>
                </table>
            </div>
            <?php } ?>
        </section>






















    </main>
</body>