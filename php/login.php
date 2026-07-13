<?php
session_start();
$submitted = !empty($_POST);
$errors = [];
$username = '';
$password = '';

if ($submitted) {

	//NOT implementing a trim function. Group decision for the user to be smart 
	// and choose a good username and password :)
	//Trimming whitespace for more inputs and assigning username and password... ie. " admin " and "admin"= VALID
	//$username = trim($_POST['username']?? '');
	//$password = trim($_POST['password']?? '');

	//Assigning username and password
	$username = $_POST['username'] ?? '';
	$password = $_POST['password'] ?? '';

	//Validating username with at least 3 characters long and password is at least 6...
	if (empty($username)) {
		$errors[] = "Username is required.";
	} elseif (strlen($username) < 3) {
		$errors[] = "Username must be at least 3 characters."; // for professionalism
	}

	if (empty($password)) {
		$errors[] = "Required Password.";
	} elseif (strlen($password) < 6) {
		$errors[] = "Password must be at least 6 characters.";
	}

	// No errors, Yeppie!
	if (empty($errors)) {
		setcookie('username', $username);
		setcookie('password', $password);

		$_SESSION['logged_in'] = true;
		$_SESSION['username'] = $username;

		header("Location: ../php/members.php");
		exit;
	}
} else {
	$username = $_COOKIE['username'] ?? '';
	$password = $_COOKIE['password'] ?? '';
}

// Sanitize once, so the template file doesn't mix special characters
$safeUsername = htmlspecialchars($username, ENT_QUOTES, 'UTF-8');
$safePassword = htmlspecialchars($password, ENT_QUOTES, 'UTF-8');

if (!empty($errors)) {
	echo "<ul>";
	foreach ($errors as $error) {
		echo "<li>" . htmlspecialchars($error) . "</li>";
	}
	echo "</ul>";
}
?>
<!DOCTYPE html>
<html lang="en">

<head>
	<title>LiF Team - Login Result</title>
	<meta charset="UTF-8">
	<link rel="icon" href="../media/icons/LiF_icon.ico" type="image/x-icon">

	<meta name="author" content="Nick Kapuka">
	<meta name="description" content="Login result page for the LiF Team website.">
	<meta name="robots" content="index, follow">
	<meta http-equiv="Pragma" content="no-cache">

	<link rel="stylesheet" href="../css/base.css">
	<link rel="stylesheet" href="../css/auth_result.css">
</head>

<body>
	<!-- nav bar -->
	<div id="navbar-placeholder" data-root="../"></div>
	<script src="../js/navbar.js"></script>

	<main class="page-wrapper">
		<section class="intro-section auth-result-intro" id="page_top">
			<div class="intro-text">
				<p class="section-label">Login Status</p>

				<?php if (!$submitted): ?>
					<h1>Saved Login Information</h1>
					<p class="intro-description">
						No new login form was submitted. Any saved cookie values are shown below.
					</p>
				<?php elseif (!empty($errors)): ?>
					<h1>Login Needs Attention</h1>
					<p class="intro-description">
						The login form was received, but some fields need to be fixed before continuing.
					</p>
				<?php else: ?>
					<h1>Login Submitted</h1>
					<p class="intro-description">
						The login form was submitted successfully and the entered information was saved for this browser.
					</p>
				<?php endif; ?>

				<div class="button-row">
					<a href="../html/login.html" class="button secondary-button">Back to Login</a>
					<a href="../index.html" class="button primary-button">Return Home</a>
				</div>
			</div>

			<aside class="summary-card">
				<h2>Result Info</h2>
				<p><strong>Project:</strong> Engineering Project VI</p>
				<p><strong>Team:</strong> Lithium Firefly</p>
				<p><strong>Status:</strong>
					<?php
					if (!$submitted) {
						echo "No form submitted";
					} elseif (!empty($errors)) {
						echo "Action required";
					} else {
						echo "Submitted";
					}
					?>
				</p>
			</aside>
		</section>

		<section class="auth-result-section <?php echo !empty($errors) ? 'auth-error-section' : 'auth-success-section'; ?>">
			<?php if (!empty($errors)): ?>
				<p class="section-label">Validation Errors</p>
				<h2>Please fix the following</h2>

				<ul class="auth-message-list">
					<?php foreach ($errors as $error): ?>
						<li><?php echo htmlspecialchars($error, ENT_QUOTES, 'UTF-8'); ?></li>
					<?php endforeach; ?>
				</ul>
			<?php else: ?>
				<p class="section-label">Submitted Values</p>
				<h2>Login Details</h2>

				<div class="auth-detail-grid login-detail-grid">
					<div class="auth-detail-card">
						<h3>Username</h3>
						<p class="login-result-value"><?php echo $safeUsername === '' ? 'No username saved.' : $safeUsername; ?></p>
					</div>

					<div class="auth-detail-card">
						<h3>Password</h3>

						<div class="password-reveal-row">
							<span id="passwordDisplay" class="login-result-value" data-password="<?php echo $safePassword; ?>">••••••••</span>
							<button type="button" id="togglePasswordBtn" class="password-eye-button">Show</button>
						</div>
					</div>
				</div>
			<?php endif; ?>
		</section>

		<p class="top-link">
			<a href="#page_top">Go to the top of this page</a>
		</p>
	</main>

	<footer class="site-footer">
		<p>Copyright &copy; 2026 Nick Kapuka</p>
	</footer>

	<script>
		// script to display password when clicked (otherwise hidden)
		const passwordDisplay = document.getElementById("passwordDisplay");
		const togglePasswordBtn = document.getElementById("togglePasswordBtn");

		// shpw real password when button clicked
		if (passwordDisplay && togglePasswordBtn) {
			togglePasswordBtn.addEventListener("click", function() {
				const realPassword = passwordDisplay.dataset.password;

				if (togglePasswordBtn.textContent === "Show") {
					passwordDisplay.textContent = realPassword;
					togglePasswordBtn.textContent = "Hide";
				} else {
					passwordDisplay.textContent = "••••••••";
					togglePasswordBtn.textContent = "Show";
				}
			});
		}
	</script>
</body>

</html>