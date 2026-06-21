<?php
$submitted = !empty($_POST);
$errors = [];
$username = '';
$password = '';

if ($submitted)	{

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
	
	if (empty($password)){
		$errors[] = "Required Password.";
	} elseif(strlen($password) < 6) {
		$errors[] = "Password must be at least 6 characters.";
	}

	// No errors, Yeppie!
	if (empty($errors)){
		setcookie('username', $username);
		setcookie('password', $password);
	}

} else {
    $username = $_COOKIE['username'] ?? '';
    $password = $_COOKIE['password'] ?? '';
}

// Sanitize once, so the template file doesn't mix special characters
$safeUsername = htmlspecialchars($username, ENT_QUOTES, 'UTF-8');
$safePassword = htmlspecialchars($password, ENT_QUOTES, 'UTF-8');

echo "<p>Username: $safeUsername</p>";
echo "<p>Password: $safePassword</p>";

if (!empty($errors)) {
    echo "<ul>";
    foreach ($errors as $error) {
        echo "<li>" . htmlspecialchars($error) . "</li>";
    }
    echo "</ul>";
}
?>

