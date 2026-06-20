<?php

$submitted = $_SERVER['REQUEST_METHOD'] === 'POST';

$errors = [];

$firstname = '';
$lastname = '';
$email = '';
$birthday = '';
$fac_or_student = '';
$involvement = [];
$details = '';
$drives_car = '';

if ($submitted) {

    // Assign elements (matching your HTML's actual field names)
    $firstname = $_POST['firstName'] ?? '';
    $lastname = $_POST['lastName'] ?? '';
    $email = $_POST['email'] ?? '';
    $birthday = $_POST['birthday'] ?? '';
    $fac_or_student = $_POST['fac_or_student'] ?? '';
    $involvement = $_POST['involvement'] ?? [];
    $details = $_POST['details'] ?? '';

    if (!is_array($involvement)) {
        $involvement = [$involvement];
    }

    $drives_car = $_POST['drives_car'] ?? '';

    // Validating input
    // First name
    if (empty($firstname)) {
        $errors[] = "First name is required.";
    }

    // Last name
    if (empty($lastname)) {
        $errors[] = "Last name is required.";
    }

    // Email
    if (empty($email)) {
        $errors[] = "Please provide an email.";
    } elseif (!filter_var($email, FILTER_VALIDATE_EMAIL)) {
        $errors[] = "Please provide a valid email address.";
    }

    // Birthday
    if (empty($birthday)) {
        $errors[] = "Please provide your birth date.";
    }

    // Faculty or student selection
    if (empty($fac_or_student)) {
        $errors[] = "Please select Faculty or Student.";
    }

    // Involvement (checkbox array)
    if (empty($involvement)) {
        $errors[] = "Please check at least one box for involvement.";
    }

    // No errors, Yeppie!
    if (empty($errors)) {
        setcookie('firstname', $firstname);
        setcookie('lastname', $lastname);
        setcookie('email', $email);
        setcookie('birthday', $birthday);
        setcookie('fac_or_student', $fac_or_student);
        setcookie('involvement', implode(',', $involvement));
    }
} 

// Sanitizing...
$safeFirstName = htmlspecialchars($firstname, ENT_QUOTES, 'UTF-8');
$safeLastName = htmlspecialchars($lastname, ENT_QUOTES, 'UTF-8');
$safeEmail = htmlspecialchars($email, ENT_QUOTES, 'UTF-8');
$safeBirthday = htmlspecialchars($birthday, ENT_QUOTES, 'UTF-8');
$safeOccupation = htmlspecialchars($fac_or_student, ENT_QUOTES, 'UTF-8');
$safeInvolvement = htmlspecialchars(implode(', ', $involvement), ENT_QUOTES, 'UTF-8');
$safeDrivesCar = htmlspecialchars(
    $drives_car,
    ENT_QUOTES,
    'UTF-8'
);

$safeDetails = htmlspecialchars(
    $details,
    ENT_QUOTES,
    'UTF-8'
);

// Output
if (!$submitted) {
    echo "<p>Please submit the request form.</p>";
} elseif (!empty($errors)) {
    echo "<h2>Please fix the following:</h2>";
    echo "<ul>";
    foreach ($errors as $error) {
        echo "<li>" . htmlspecialchars($error) . "</li>";
    }
    echo "</ul>";
} else {
    echo "<h2>Request Submitted</h2>";
    echo "<p>First Name: $safeFirstName</p>";
    echo "<p>Last Name: $safeLastName</p>";
    echo "<p>Email: $safeEmail</p>";
    echo "<p>Birthday: $safeBirthday</p>";
    echo "<p>Faculty/Student: $safeOccupation</p>";
    echo "<p>Involvement: $safeInvolvement</p>";
    echo "<p>Drives a car: $safeDrivesCar</p>";
    echo "<p>Other details: $safeDetails</p>";
}
?>