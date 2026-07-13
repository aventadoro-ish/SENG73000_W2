<?php

require_once __DIR__ . '/db.php';


$submitted = $_SERVER['REQUEST_METHOD'] === 'POST';

$errors = [];

// define variables
$firstname = '';
$lastname = '';
$email = '';
$birthday = '';
$fac_or_student = '';
$involvement = [];
$details = '';
$drives_car = '';
$involvementText = '';

$requestSaved = false;
$requestId = null;

if ($submitted) 
{

    // match HTML names (from form submmited values)
    $firstname = $_POST['firstName'] ?? '';
    $lastname = $_POST['lastName'] ?? '';
    $email = $_POST['email'] ?? '';
    $birthday = $_POST['birthday'] ?? '';
    $fac_or_student = $_POST['fac_or_student'] ?? '';
    $involvement = $_POST['involvement'] ?? [];
    $details = $_POST['details'] ?? '';
    $drives_car = $_POST['drives_car'] ?? '';

    // involvement must be an array
    if (!is_array($involvement)) {
        $involvement = [$involvement];
    }

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

    if (strlen($details) > 180) { {
        $errors[] = "Details cannot exceed 180 characters";
    }
}
    // No errors, Yeppie!
    if (empty($errors)) {
        $involvementText = implode(', ', $involvement);
        // format it into SQL using try-catch, my beloved:
        try {
            $sql = 
            "
            INSERT INTO access_requests (
                first_name,
                last_name,
                email,
                birthday,
                person_type,
                involvement,
                drives_car,
                details)
        
            VALUES (
                :first_name,
                :last_name,
                :email,
                :birthday,
                :person_type,
                :involvement,
                :drives_car,
                :details
            ) 
             ";

            $statement = $pdo->prepare($sql);

            $statement->execute([
                ':first_name' => $firstname,
                ':last_name' => $lastname,
                ':email' => $email,
                ':birthday' => $birthday,
                ':person_type' => $fac_or_student,
                ':involvement' => $involvementText,
                ':drives_car' => $drives_car,
                ':details' => $details
            ]);

            $requestId = $pdo->lastInsertId();
            $requestSaved = true;    
        } 
        catch (PDOException $e) 
        {
            $errors[] = "The access request could not be saved/failed to load";
            error_log($e->getMessage());
        }
    }
}



// Sanitizing...
$safeFirstName = htmlspecialchars($firstname, ENT_QUOTES, 'UTF-8');
$safeLastName = htmlspecialchars($lastname, ENT_QUOTES, 'UTF-8');
$safeEmail = htmlspecialchars($email, ENT_QUOTES, 'UTF-8');
$safeBirthday = htmlspecialchars($birthday, ENT_QUOTES, 'UTF-8');
$safeOccupation = htmlspecialchars($fac_or_student, ENT_QUOTES, 'UTF-8');
$safeInvolvement = htmlspecialchars(implode(', ', $involvement), ENT_QUOTES, 'UTF-8');
$safeDrivesCar = htmlspecialchars($drives_car, ENT_QUOTES, 'UTF-8');
$safeDetails = htmlspecialchars($details, ENT_QUOTES, 'UTF-8');

?>

<!DOCTYPE html>
<html lang="en">

<head>
    <title>LiF Team - Access Request Result</title>
    <meta charset="UTF-8">
    <link rel="icon" href="../media/icons/LiF_icon.ico" type="image/x-icon">

    <meta name="author" content="Nick Kapuka">
    <meta name="description" content="Access request result page for the LiF Team website.">
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
                <p class="section-label">Access Request Status</p>

                <?php if (!$submitted): ?>
                <h1>No Request Submitted</h1>
                <p class="intro-description">
                    This page displays the result after the request access form is submitted.
                </p>
                <?php elseif (!empty($errors)): ?>
                <h1>Request Needs Attention</h1>
                <p class="intro-description">
                    The access request was received, but some fields need to be corrected before it can be accepted.
                </p>
                <?php else: ?>
                <h1>Request Submitted</h1>
                <p class="intro-description">
                    Your access request was submitted successfully. The information below summarizes the request.
                </p>
                <?php endif; ?>

                <div class="button-row">
                    <a href="../html/request_access.html" class="button secondary-button">Back to Request Form</a>
                    <a href="../index.html" class="button primary-button">Return Home</a>
                </div>
            </div>

            <aside class="summary-card">
                <h2>Request Info</h2>
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
            <?php if (!$submitted): ?>
            <p class="section-label">No Form Data</p>
            <h2>Please submit the request form</h2>
            <p>
                Use the button above to return to the request access page.
            </p>
            <?php elseif (!empty($errors)): ?>
            <p class="section-label">Validation Errors</p>
            <h2>Please fix the following</h2>

            <ul class="auth-message-list">
                <?php foreach ($errors as $error): ?>
                <li><?php echo htmlspecialchars($error, ENT_QUOTES, 'UTF-8'); ?></li>
                <?php endforeach; ?>
            </ul>
            <?php else: ?>
            <p class="section-label">Submitted Values</p>
            <h2>Access Request Details</h2>

            <div class="auth-detail-grid">
                <div class="auth-detail-card">
                    <h3>First Name</h3>
                    <p><?php echo $safeFirstName; ?></p>
                </div>

                <div class="auth-detail-card">
                    <h3>Last Name</h3>
                    <p><?php echo $safeLastName; ?></p>
                </div>

                <div class="auth-detail-card">
                    <h3>Email</h3>
                    <p><?php echo $safeEmail; ?></p>
                </div>

                <div class="auth-detail-card">
                    <h3>Birthday</h3>
                    <p><?php echo $safeBirthday; ?></p>
                </div>

                <div class="auth-detail-card">
                    <h3>Faculty/Student</h3>
                    <p><?php echo $safeOccupation; ?></p>
                </div>

                <div class="auth-detail-card">
                    <h3>Involvement</h3>
                    <p><?php echo $safeInvolvement; ?></p>
                </div>

                <div class="auth-detail-card">
                    <h3>Drives a Car</h3>
                    <p><?php echo $safeDrivesCar; ?></p>
                </div>

                <div class="auth-detail-card wide-card">
                    <h3>Other Details</h3>
                    <p><?php echo $safeDetails === '' ? 'No additional details provided.' : $safeDetails; ?></p>
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