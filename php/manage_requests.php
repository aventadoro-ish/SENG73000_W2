<?php

session_start();

// load the DB connection
require_once __DIR__ . '/db.php';

    // login.php sets $_SESSION to be logged in to true, so we're gonna check that
    // if someone is NOT logged in, send them back to the login page
    if (!isset($_SESSION['logged_in']) || $_SESSION['logged_in'] !== true) {
        header("Location: ../html/login.html");
        exit("Please log in.");
    }

    // check if the user logged in is the LiF Admin account:
    // use user-role or an empty string
    if(($_SESSION['user_role'] ?? '') !== 'admin') {
        // unauthorized access
        http_response_code(403);
        exit("Access denied; Admin perms required for entry to this page. Please return and login.");
    }
    
    // next, we want to query the DB and grab all requests that are "pending"
    // query the DB to grab important information to use using a string
    $query = "
        SELECT
            request_id,
            first_name,
            last_name,
            email,
            person_type,
            involvement,
            submitted_at
        FROM access_requests
        WHERE request_status = 'pending'
        ORDER BY submitted_at ASC
    ";
    // this block will query the SQL to grab these items FROM access_request DB, specifically entries where request_status is pending
    // and it will be ordered by submission time

    // execute the query:
    $statement = $pdo->query($query);

    // retreive the data
    $requests = $statement->fetchAll();
    
    // an array of errors/faults
    $errors = [];
    $formMessage = '';

    // part of the page refresh logic
    if (isset($_GET['approved']) && $_GET['approved'] === '1') {
        $formMessage = "The user account has been created and the request was approved!";
    }
    
    $requestID = null;
    $newUsername = '';
    $newPassword = '';

    // process approval after it has been submitted
    if($_SERVER['REQUEST_METHOD'] === 'POST') {
        // grab variables from the submitted form
        $submittedRequestID = $_POST['request_id'] ?? '';
        $newUsername = $_POST['username'] ?? '';
        $newPassword = $_POST['password'] ?? '';

        // convert submitted requestr ID into an integer
        $requestID = filter_var($submittedRequestID, FILTER_VALIDATE_INT);

        // request ID must be positive
        if ($requestID === false || $requestID < 1) {
            $errors[] = "Invalid access request ID";
        }

        // match username limits of the actual DB entry
        if(empty($newUsername)) {
            $errors[] = "a username is required";
        } else if (strlen($newUsername) < 4) {
            $errors[] = "username must be more than four characters";
        } else if (strlen($newUsername) > 50) {
            $errors[] = "username must be under 50 characters";
        }

        // match password limits of the actual DB entry
        if(empty($newPassword)) {
            $errors[] = "a password is required";
        } else if (strlen($newPassword) < 6) {
            $errors[] = "password must be more than six characters";
        }

        if(empty($errors)) {
            // if there are no errors, approve the request by checking the DB that everything matches up
            $query = "
                SELECT
                    request_id,
                    email,
                    request_status
                FROM access_requests
                WHERE request_id = :request_id
                LIMIT 1
            ";
            

            // create a prepare statement for the query
            $statement = $pdo->prepare($query);

            // replace :request_id in the query with the submitted integer
            $statement->bindValue('request_id', $requestID, PDO::PARAM_INT);

            // execute the query
            $result = $statement->execute();

            // retrieve the matching request
            $selectedRequest = $statement->fetch();

            // if nothing returned 
            if(!$selectedRequest) {
                $errors[] = "the selected request does not exist";

            // if the request is NOT pending (denied, approved)
            } else if ($selectedRequest['request_status'] !== 'pending') {
                $errors[] = "the selected request has already been reviewed";
            
            // if it exists and IS pending (since it's not any other form), it is ready
            // if it is ready, insert it into the DB
            } else {
                // convert the password that the admin entered into a hash
                // use PHP's hash function to hash the new password using the default hasher
                $passwordHash = password_hash($newPassword, PASSWORD_DEFAULT);

                try 
                {
                    // define the account that should be inserted into DB
                    $query = "
                        INSERT INTO users (
                            access_request_id,
                            username,
                            password_hash,
                            email,
                            user_role,
                            account_status
                        )

                        VALUES (
                            :access_request_id,
                            :username,
                            :password_hash,
                            :email,
                            :user_role,
                            :account_status
                        )
                    ";

                    // prepare the insertion
                    $statement = $pdo->prepare($query);

                    // map each SQL placeholder to its real PHP value
                    // $selectedRequest is the data that was fetched from the DB
                    $params = [
                        'access_request_id' => $selectedRequest['request_id'],
                        'username' => $newUsername,
                        'password_hash' => $passwordHash,
                        'email' => $selectedRequest['email'],
                        'user_role' => 'user',
                        'account_status' => 'approved'
                    ];

                    // create the website account by executing
                    $result = $statement->execute($params);

                    // check if it worked
                    if ($result) {
                        // make a query string for access_requests. Only basic stuff has to be changed.
                        $query = "
                            UPDATE access_requests 
                            SET request_status = 'approved', 
                            reviewed_at = CURRENT_TIMESTAMP WHERE request_id = :request_id;
                        ";

                        // prepare the insertion
                        $statement = $pdo->prepare($query);

                        // map each SQL placeholder to its real PHP value
                        $params = [
                            'request_id' =>  $selectedRequest['request_id']
                        ];

                        // execute the command
                        $result = $statement->execute($params);

                        // check if it worked
                        if ($result && $statement->rowCount() === 1) {
                            // return to the page using a GET request instead of POST
                            // prevents a refresh from sending another INSERT INTO request
                            header("Location: manage_requests.php?approved=1");
                            exit;
                        } else {
                            $errors[] = "access requests could not be updated";
                        }
                    } else {
                        $errors[] = "the user account was created, but access requests could not be updated";
                    }
                }
                catch (PDOException $e)
                {
                    $errors[] = "Database error: " . $e->getMessage();
                    error_log($e->getMessage());
                }

            }
        }
    }
?>

<!DOCTYPE html>
<html lang="en">

<head>
    <title>Manage Access Requests</title>
    <meta charset="UTF-8">
    <link rel="icon" href="../media/icons/LiF_icon.ico" type="image/x-icon">

    <meta name="author" content="Nick Kapuka">
    <meta name="description" content="confirm/deny access requests">
    <meta name="robots" content="noindex, nofollow">
    <meta http-equiv="Pragma" content="no-cache">

    <link rel="stylesheet" href="../css/base.css">
    <link rel="stylesheet" href="../css/manage_requests.css">
</head>

<body>

    <!-- nav bar -->
    <div id="navbar-placeholder" data-root="../"></div>
    <script src="../js/navbar.js"></script>

    <main class="page-wrapper">
        <section class="intro-section" id="page_top">
            <div>
                <p class="section-label">Admin Page</p>

                <h1>Hello, <?php echo $_SESSION['username']; ?></h1>

                <p class="intro-description">
                    Here, you will see pending access requests. You're free to accept or deny them.

                    Additionally, you grant them a permanent username and temporary password which they can use to log
                    in.
                </p>

                <div class="button-row">
                    <a href="logout.php" class="button secondary-button">Log out</a>
                    <a href="members.php" class="button primary-button">Return to members page</a>
                </div>

            </div>
            <aside class="summary-card">
                <h2>Access Status</h2>
                <p><strong>Status:</strong> Logged in</p>
                <p><strong>User:</strong> <?php echo $_SESSION['username']; ?></p>
                <p><strong>Area:</strong> Protected content</p>
                <p><strong>Account Type:</strong>
                    <?php echo htmlspecialchars($_SESSION['user_role'], ENT_QUOTES, 'UTF-8');?>
                <p>
            </aside>
        </section>

        <section class="admin-content-section">
            <h2>Status Updates:</h2>

            <!-- print any errors -->
            <?php if(!empty($errors)): ?>
            <h2>Approval errors!</h2>
            <ul>
                <!-- Loop through contents to dispaly errors in array -->
                <?php foreach($errors as $error): ?>
                <li><?php echo htmlspecialchars($error, ENT_QUOTES, 'UTF-8');?></li>
                <?php endforeach ?>
            </ul>

            <!-- print if successful -->
            <?php elseif (!empty($formMessage)): ?>
            <p>
                <?php echo htmlspecialchars($formMessage, ENT_QUOTES, 'UTF-8'); ?>
            </p>
            <?php endif ?>

            <?php if(empty($requests)): ?>
            <p>There are currently no pending requests</p>

            <?php else: ?>
            <p>There are currently <b><?php echo count($requests); ?></b> pending requests for review.</p>
            <?php endif ?>
        </section>

        <?php if (!empty($requests)): ?>
        <section class="requests-table-section">
            <div class="requests-heading">
                <p class="section-label">Request Table</p>
                <h2>Pending Requests:</h2>
                <p>View all the pending requests below:</p>

            </div>

            <div class="requests-table-wrapper">
                <!-- make a table of requests -->
                <table class="requests-table">
                    <thead>
                        <tr>
                            <th>Request ID</th>
                            <th>First and Last Name</th>
                            <th>Email</th>
                            <th>Occupation</th>
                            <th>Involvement</th>
                            <th>Submitted at</th>
                            <th>Create Account</th>
                        </tr>
                    </thead>

                    <tbody>
                        <?php foreach ($requests as $request): ?>

                        <tr>
                            <td>
                                <?php echo $request['request_id']; ?>
                            </td>

                            <td>
                                <?php echo htmlspecialchars($request['first_name'] . ' ' . $request['last_name'], ENT_QUOTES, 'UTF-8'); ?>
                            </td>

                            <td>
                                <?php echo htmlspecialchars($request['email'], ENT_QUOTES, 'UTF-8'); ?>
                            </td>

                            <td>
                                <?php echo htmlspecialchars($request['person_type'], ENT_QUOTES, 'UTF-8'); ?>
                            </td>

                            <td>
                                <?php echo htmlspecialchars($request['involvement'], ENT_QUOTES, 'UTF-8'); ?>
                            </td>

                            <td>
                                <?php echo htmlspecialchars($request['submitted_at'], ENT_QUOTES, 'UTF-8'); ?>
                            </td>

                            <td>
                                <form action="manage_requests.php" method="POST">
                                    <!-- -->
                                    <input type="hidden" name="request_id"
                                        value="<?php echo $request['request_id']; ?>">

                                    <label>Username:
                                        <input type="text" name="username" minlength="7" required>
                                    </label>
                                    <br>
                                    <label>Temporary Password:
                                        <input type="text" name="password" minlength="7" required>
                                    </label>
                                    <br>
                                    <button type="submit">Approve</button>
                                </form>
                            </td>
                        </tr>
                        <?php endforeach; ?>
                    </tbody>
                </table>



        </section>
        <?php endif; ?>
    </main>
    </div>
    <p>
        <a href="members.php">Return to member area</a>
    </p>
    </div>
</body>

</html>