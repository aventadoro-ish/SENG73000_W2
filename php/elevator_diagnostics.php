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
            $elevatorRequests = $statement->fetchAll();
        }

        // load states
        $query = "
            SELECT
                state_id,
                doors_open,
                sabbath_enabled,
                updated_at
            FROM elevator_state
            ORDER BY state_id
        "; 

        // prepare and pull the DB data into a PHP array
        $statement = $pdo->prepare($query);

        // send query to DB
        $result = $statement->execute();

        if($result) {
            // retrieve the data from the query
            $elevatorState = $statement->fetchAll();
        }

    } catch (PDOException $e) {
        error_log($e->getMessage());
    }
?>

<!DOCTYPE html>
<html lang="en">

<head>
    <title>LiF Team - Member Area</title>
    <meta charset="UTF-8">
    <link rel="icon" href="../media/icons/LiF_icon.ico" type="image/x-icon">

    <meta name="author" content="Nick Kapuka">
    <meta name="description" content="Protected member area for the LiF Team website.">
    <meta name="robots" content="noindex, nofollow">
    <meta http-equiv="Pragma" content="no-cache">

    <link rel="stylesheet" href="../css/base.css">
    <link rel="stylesheet" href="../css/members.css">
</head>

<body>
    <div id="navbar-placeholder" data-root="../"></div>
    <script src="../js/navbar.js"></script>

    <main class="page-wrapper">
    </main>
</body>