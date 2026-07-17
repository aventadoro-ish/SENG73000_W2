<?php
session_start();

// load the DB connection
require_once __DIR__ . '/db.php';

if (!isset($_SESSION['logged_in']) || $_SESSION['logged_in'] !== true) {
    header("Location: ../html/login.html");
    exit;
}

$safeUsername = htmlspecialchars($_SESSION['username'] ?? 'User', ENT_QUOTES, 'UTF-8');
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
        <section class="intro-section member-area-intro" id="page_top">
            <div class="intro-text">
                <p class="section-label">Member Area</p>

                <h1>Welcome, <?php echo $safeUsername; ?></h1>

                <p class="intro-description">
                    This page is only visible after logging in. Just like your mother said, you're special :3
                </p>

                <div class="button-row">
                    <a href="../php/logout.php" class="button secondary-button">Log Out</a>
                    <a href="../index.html" class="button primary-button">Return Home</a>
                </div>
            </div>

            <aside class="summary-card">
                <h2>Access Status</h2>
                <p><strong>Status:</strong> Logged in</p>
                <p><strong>User:</strong> <?php echo $safeUsername; ?></p>
                <p><strong>Area:</strong> Protected content</p>
                <p><strong>Account Type:</strong> <?php echo $_SESSION['user_role']; ?>
                <p>
            </aside>
        </section>

        <section class="member-content-section">
            <p class="section-label">Member Dashboard</p>
            <h2>Available Member Tools</h2>

            <p>
                Select one of the options below. These pages are only meant to be accessed
                after logging in.
            </p>


            <div class="member-button-grid">

                <a href="elevator_control.php" class="member-action-card elevator-card">
                    <h3>Elevator Control</h3>
                    <p>Control the elevator from a website!</p>
                </a>

                <a href="super_secret.php" class="member-action-card secret-card">
                    <h3>Source Code</h3>
                    <p>Click here to see ALL of our source code and copy our ENTIRE project!!! Source: trust me bro</p>
                </a>

                <a href="logout.php" class="member-action-card logout-card">
                    <h3>Log Out</h3>
                    <p>End the current login session.</p>
                </a>
                <?php if ($_SESSION['user_role'] === 'admin'): ?>
                <a href="manage_requests.php" class="member-action-card manage-req">
                    <h3>Manage Incoming Requests</h3>
                    <p>Accept/deny website access.</p>
                </a>
                <?php endif ?>
            </div>
        </section>

        <p class="top-link">
            <a href="#page_top">Go to the top of this page</a>
        </p>
    </main>

    <footer class="site-footer">
        <p>Copyright &copy; 2026 Nick Kapuka</p>
    </footer>
</body>

</html>