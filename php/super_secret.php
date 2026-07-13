<?php
session_start();

if (!isset($_SESSION['logged_in']) || $_SESSION['logged_in'] !== true) {
    header("Location: ../html/login.html");
    exit;
}

$safeUsername = htmlspecialchars($_SESSION['username'] ?? 'User', ENT_QUOTES, 'UTF-8');
?>

<!DOCTYPE html>
<html lang="en">

<head>
    <title>LiF Team - Super Secret</title>
    <meta charset="UTF-8">
    <link rel="icon" href="../media/icons/LiF_icon.ico" type="image/x-icon">

    <meta name="author" content="Nick Kapuka">
    <meta name="description" content="Extremely classified LiF Team member page.">
    <meta name="robots" content="noindex, nofollow">
    <meta http-equiv="Pragma" content="no-cache">

    <link rel="stylesheet" href="../css/base.css">
    <link rel="stylesheet" href="../css/super_secret.css">
</head>

<body>
    <!-- nav bar -->
    <div id="navbar-placeholder" data-root="../"></div>
    <script src="../js/navbar.js"></script>

    <main class="page-wrapper">
        <section class="intro-section super-secret-intro" id="page_top">
            <div class="intro-text">
                <p class="section-label">Classified Member Area</p>

                <h1>Super Secret Page</h1>

                <p class="intro-description">
                    Welcome, <?php echo $safeUsername; ?>. You have successfully reached the most protected
                    and definitely-not-suspicious page on the Lithium Firefly website.
                </p>

                <div class="button-row">
                    <a href="members.php" class="button secondary-button">Back to Members</a>
                    <a href="logout.php" class="button primary-button">Log Out</a>

                </div>

                <aside class="summary-card">
                    <h2>Security Status</h2>
                    <p><strong>Clearance:</strong> Extremely serious</p>
                    <p><strong>Threat Level:</strong> Musical</p>
                    <p><strong>Outcome:</strong> Inevitable</p>
                </aside>
            </div>
        </section>

        <section class="secret-video-section">
            <div class="secret-heading">
                <p class="section-label">Source code:</p>
                <h2>Confidential source code:</h2>
                <p>
                    The following classified material, our entire code, is only available to those who logged in.
                    Viewer discretion is advised.
                </p>
            </div>

            <div class="rickroll-frame-wrapper">
                <iframe
                    src="https://www.youtube.com/embed/dQw4w9WgXcQ?autoplay=1&rel=0"
                    title="Super secret source code"
                    allow="accelerometer; autoplay; clipboard-write; encrypted-media; gyroscope; picture-in-picture; web-share"
                    allowfullscreen>
                </iframe>
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