<?php
session_start();

require_once __DIR__ . '/db.php';


$submitted = $_SERVER['REQUEST_METHOD'] === 'POST';
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

	//Validating username with at least 4 characters long and password is at least 6...
	if (empty($username)) {
		$errors[] = "Username is required.";
	} elseif (strlen($username) < 4) {
		$errors[] = "Username must be at least 4 characters."; // for professionalism
	}

	if (empty($password)) {
		$errors[] = "Required Password.";
	} elseif (strlen($password) < 6) {
		$errors[] = "Password must be at least 6 characters.";
	}

	
	if (empty($errors)) {
		try {
			// query the DB for one user matching submitted username
			$query = 
			"
				SELECT
					user_id,
					username,
					password_hash,
					user_role,
					account_status
				FROM users
				WHERE username = :username
				LIMIT 1
			";

			// prepare the query before adding dynamic user input
			$statement = $pdo->prepare($query);

			// replace :username with submitted username
			$statement->bindValue('username', $username);

			// execute the query and retriving the matching user
			$result = $statement->execute();
			$user = $statement->fetch();
			
			// compare submitted password against the stored hash
			$passwordCorrect = $user && password_verify($password, $user['password_hash']);

			// only accounts thar are "approved" may log in (not pending!)
			$accountApproved = $user && $user['account_status'] === 'approved';

			if($passwordCorrect && $accountApproved)
			{

				// record successful login time into DB
				$query = '
   					UPDATE users
    				SET last_login_at = CURRENT_TIMESTAMP
    				WHERE user_id = :user_id
				';

				$statement = $pdo->prepare($query);

				$statement->bindValue('user_id', $user['user_id']);

				$result = $statement->execute();

				
				// store information needed by protected PHP pages
				$_SESSION['logged_in'] = true;
				$_SESSION['user_id'] = $user['user_id'];
				$_SESSION['username'] = $user['username'];
				$_SESSION['user_role'] = $user['user_role'];

				// send the authenticated user to the member area
				header("Location: members.php");
				exit;
			}

			// use one general message so we do not reveal whether the username exists
			$errors[] = "incorrect username or password";

		} catch (PDOException $e) {
			$errors[] = "Login system unavailable";
			error_log($e->getMessage());
		}
	}
}

// Sanitize once, so the template file doesn't mix special characters
$safeUsername = htmlspecialchars($username, ENT_QUOTES, 'UTF-8');

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

        <section
            class="auth-result-section <?php echo !empty($errors) ? 'auth-error-section' : 'auth-success-section'; ?>">
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
                    <p class="login-result-value">
                        <?php echo $safeUsername === '' ? 'No username saved.' : $safeUsername; ?></p>
                </div>

                <div class="auth-detail-card">
                    <h3>Password</h3>

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
</body>

</html>