<?php
session_start();
if (!isset($_SESSION['user_id'])) {
    header("Location: login.php");
    exit();
}
?>

<!DOCTYPE html>
<html>

<head>
    <title>Elevator Dashboard</title>
</head>

<body>
    <h1>Welcome, <?= htmlspecialchars($_SESSION['username']) ?>!</h1>
    <p>Elevator Control System — Phase 2</p>
    <ul>
        <li><a href="#">Diagnostics</a></li>
        <li><a href="#">Send Command</a></li>
        <li><a href="#">Event Log</a></li>
        <li><a href="#">Sabbath Mode</a></li>
    </ul>
    <a href="logout.php">Log out</a>
</body>

</html>