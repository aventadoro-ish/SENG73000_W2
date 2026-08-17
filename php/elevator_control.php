<?php
session_start();

// DB connection
require_once __DIR__ . '/db.php';

// OOP class files as per A2
require_once __DIR__ . '/classes/elevatorCar.php';
require_once __DIR__ . '/classes/floorNode.php';
require_once __DIR__ . '/classes/distanceSensor.php';

// create objects that represent the hardware used by the control page
$elevatorCar = new ElevatorCar("CC", "0x200", 1);

$floorNodes = [
    1 => new FloorNode("F1", "0x201", 1),
    2 => new FLoorNode("F2", "0x202", 2),
    3 => new FloorNode("F3", "0x203", 3)
];

// distance sensor (UNUSED)
$distanceSensor = new DistanceSensor("DS");

// test the CANDevice interface and shared CAN ID trait between nodes
$canDevices = [
    $elevatorCar,
    $floorNodes[1],
    $floorNodes[2],
    $floorNodes[3]
];

// check if all implement CAN ID traits
$canDeviceTestPassed = true;
$canDeviceDetails = [];

foreach($canDevices as $canDevice) {
    if(!$canDevice instanceof CANDevice) {
        $canDeviceTestPassed = false;
        continue;
    }

    $canDeviceDetails[] = $canDevice->getNodeName() . " (" . $canDevice->getCANID() . ")";
}

if (!isset($_SESSION['logged_in']) || $_SESSION['logged_in'] !== true) {
    header("Location: ../html/login.html");
    exit;
}



// return the latest EC-confirmed floor when the webpage loads
if($_SERVER['REQUEST_METHOD'] === 'GET' && ($_GET['request_action'] ?? '') === 'status') {
    header('Content-Type: application/json');
    header('Cache-Control: no-store');
    try {
        // find the newest valid position report received from the EC
        $query = "
            SELECT log_id, raw_byte
            FROM can_message_log
            WHERE can_id = '0x101'
                AND direction = 'rx'
                AND source_controller = 'EC'
                AND raw_byte IN (5, 6, 7)
            ORDER BY log_id DESC
            LIMIT 1
        ";

        $statement = $pdo->query($query);
        $latestPosition = $statement->fetch();

        // return a successful response even if no position has been logged yet
        if(!$latestPosition) {
            echo json_encode([
                'success' => true,
                'position_available' => false,
                'current_floor' => null,
                'log_id' => null
            ]);

            exit;
        }

        // convert each EC position byte into its corresponding floor
        $floorByRawByte = [
            5 => 1,
            6 => 2,
            7 => 3
        ];

        // type-cast it into an int
        $rawByte = (int) $latestPosition['raw_byte'];
        $currentFloor = $floorByRawByte[$rawByte];

        echo json_encode([
            'success' => true,
            'position_available' => true,
            'current_floor' => $currentFloor,
            'log_id' => (int) $latestPosition['log_id']
        ]);

        exit;
        
    } catch(PDOException $error) {
        error_log($error->getMessage());

        echo json_encode([
            'success' => false,
            'message' => 'elevator status could not be loaded'
        ]);

        exit;
    }
}

// elevator_control JS sends a POST request when an elevator button is pressed
// read it here
if($_SERVER['REQUEST_METHOD'] === 'POST') {
    // tell JS that PHP will return a JSON file instead of HTML:
    header('Content-Type: application/json');

    // determine what type of AJAX request was sent (move elevator or toggle doors button)
    $requestAction = $_POST['request_action'] ?? 'move';

    // handle a door-state update separately from a move elevator request
    if($requestAction === 'door') {
        // JS will send open/closed so check for that
        $submittedDoorState = $_POST['door_state'] ?? '';

        // add only two states for doors
        $allowedDoorStates = ['open', 'closed'];

        // reject other states
        if(!in_array($submittedDoorState, $allowedDoorStates, true)) {
            echo json_encode([
                'success' => false,
                'message' => 'invalid door state'
            ]);
            
            exit;
        } 

        try {
            // Lock the shared state row while permission is checked and the
            // door state is updated. This prevents Maintenance from changing
            // halfway through a non-admin door-open request.
            $pdo->beginTransaction();

            $query = "
                SELECT doors_open, operation_mode
                FROM elevator_state
                WHERE state_id = 1
                LIMIT 1
                FOR UPDATE
            ";

            $statement = $pdo->prepare($query);
            $result = $statement->execute();
            $elevatorState = $statement->fetch();

            if(!$result || !$elevatorState) {
                throw new RuntimeException('Elevator state could not be received');
            }

            $userIsAdmin = ($_SESSION['user_role'] ?? '') === 'admin';

            // During Maintenance, only administrators may operate the doors.
            // This blocks both opening and closing while personnel may be
            // servicing the doorway, actuator, or safety sensors.
            if(
                $elevatorState['operation_mode'] === 'maintenance' &&
                !$userIsAdmin
            ) {
                $currentDoorState = ((int) $elevatorState['doors_open'] & 1) === 1
                    ? 'open'
                    : 'closed';

                $pdo->rollBack();

                echo json_encode([
                    'success' => false,
                    'message' => 'Only administrators can operate the doors while Maintenance mode is active',
                    'door_state' => $currentDoorState,
                    'operation_mode' => 'maintenance'
                ]);

                exit;
            }

            // OOP change - open/closeDoors function
            if($submittedDoorState === 'open') {
                $elevatorCar->openDoors();
            } else {
                $elevatorCar->closeDoors();
            }

            // convert the object state into the DB boolean
            $doorsOpen = $elevatorCar->areDoorsOpen() ? 1 : 0;

            // update the row that represents door state
            $query = "
                UPDATE elevator_state
                SET doors_open = :doors_open
                WHERE state_id = 1
            ";

            $statement = $pdo->prepare($query);

            $params  = ['doors_open' => $doorsOpen];

            $result = $statement->execute($params);

            if(!$result) {
                throw new RuntimeException('Door state could not be updated');
            }

            $pdo->commit();

            echo json_encode([
                'success' => true,
                'message' => 'door state updated',
                'door_state' => $submittedDoorState,
                'operation_mode' => $elevatorState['operation_mode']
            ]);
        } catch (Throwable $e) {
            if($pdo->inTransaction()) {
                $pdo->rollBack();
            }

            error_log($e->getMessage());

            echo json_encode([
                'success' => false,
                'message' => 'A database error prevented the door update'
            ]);
        }

        // door request complete, do not continue

        exit;
    }

    if($requestAction === 'sabbath') {
        // JS sends either enabled or disabled
        $submittedSabbathState = $_POST['sabbath_state'] ?? '';

        $allowedSabbathStates = ['enabled', 'disabled'];

        if(!in_array($submittedSabbathState, $allowedSabbathStates, true)) {
            echo json_encode([
                'success' => false,
                'message' => 'invalid sabbath state'
            ]);
            exit;
        }

        try {
            // Lock the shared state row so Maintenance cannot change halfway
            // through the Sabbath-mode validation and update.
            $pdo->beginTransaction();

            $query = "
                SELECT operation_mode
                FROM elevator_state
                WHERE state_id = 1
                LIMIT 1
                FOR UPDATE
            ";

            $statement = $pdo->prepare($query);
            $result = $statement->execute();
            $elevatorState = $statement->fetch();

            if(!$result || !$elevatorState) {
                throw new RuntimeException('Elevator operation mode could not be received');
            }

            // Maintenance owns the shared operation mode. A Sabbath request
            // must never enable Sabbath or accidentally turn Maintenance off.
            if($elevatorState['operation_mode'] === 'maintenance') {
                $pdo->rollBack();

                echo json_encode([
                    'success' => false,
                    'message' => 'Sabbath mode is unavailable while Maintenance mode is active',
                    'sabbath_state' => 'disabled',
                    'operation_mode' => 'maintenance'
                ]);

                exit;
            }

            $operationMode = $submittedSabbathState === 'enabled'
                ? 'sabbath'
                : 'normal';

            $query = "
                UPDATE elevator_state
                SET operation_mode = :operation_mode
                WHERE state_id = 1
            ";

            $statement = $pdo->prepare($query);
            $result = $statement->execute([
                'operation_mode' => $operationMode
            ]);

            if(!$result) {
                throw new RuntimeException('Sabbath mode could not be updated');
            }

            $pdo->commit();

            echo json_encode([
                'success' => true,
                'message' => 'sabbath mode updated',
                'sabbath_state' => $submittedSabbathState,
                'operation_mode' => $operationMode
            ]);
        } catch (Throwable $e) {
            if($pdo->inTransaction()) {
                $pdo->rollBack();
            }

            error_log($e->getMessage());

            echo json_encode([
                'success' => false,
                'message' => 'A database error prevented the Sabbath toggle'
            ]);
        }

        exit;
    }

    // handle the maintenance-mode toggle separately from elevator movement requests
    if($requestAction === 'maintenance') {
        // the button is admin-only in the HTML, so enforce the same rule in PHP
        if(($_SESSION['user_role'] ?? '') !== 'admin') {
            echo json_encode([
                'success' => false,
                'message' => 'Administrator access is required to change Maintenance mode'
            ]);
            exit;
        }

        // JS sends either enabled or disabled
        $submittedMaintenanceState = $_POST['maintenance_state'] ?? '';

        // only accept the two states used by the maintenance button
        $allowedMaintenanceStates = ['enabled', 'disabled'];

        // reject any other submitted value
        if(!in_array($submittedMaintenanceState, $allowedMaintenanceStates, true)) {
            echo json_encode([
                'success' => false,
                'message' => 'invalid maintenance state'
            ]);
            exit;
        }

        // disabling maintenance returns the elevator to normal operation
        $operationMode = 'normal';

        // enabling maintenance changes the shared operation_mode column
        if($submittedMaintenanceState === 'enabled') {
            $operationMode = 'maintenance';
        }

        try {
            // update the single elevator-state row
            $query = "
                UPDATE elevator_state
                SET operation_mode = :operation_mode
                WHERE state_id = 1
            ";

            $statement = $pdo->prepare($query);

            $params = ['operation_mode' => $operationMode];

            $result = $statement->execute($params);

            if($result) {
                // return both the button state and final DB operation mode to JS
                echo json_encode([
                    'success' => true,
                    'message' => 'maintenance mode updated',
                    'maintenance_state' => $submittedMaintenanceState,
                    'operation_mode' => $operationMode
                ]);
            } else {
                echo json_encode([
                    'success' => false,
                    'message' => 'maintenance mode could not be updated'
                ]);
            }
        } catch (PDOException $e) {
            error_log($e->getMessage());

            echo json_encode([
                'success' => false,
                'message' => 'a database error prevented the maintenance toggle'
            ]);
        }

        // maintenance request complete, do not continue into movement handling
        exit;
    }

    // save or remove one per-floor maintenance lockout
    if($requestAction === 'floor_lockout') {
        // lockout controls are available only to administrators
        if(($_SESSION['user_role'] ?? '') !== 'admin') {
            echo json_encode([
                'success' => false,
                'message' => 'Administrator access is required to change floor lockouts'
            ]);
            exit;
        }

        $submittedLockoutFloor = $_POST['floor_number'] ?? '';
        $submittedLockoutState = $_POST['lockout_state'] ?? '';

        $lockoutFloor = filter_var($submittedLockoutFloor, FILTER_VALIDATE_INT);
        $allowedLockoutFloors = [1, 2, 3, 4, 5, 6];
        $allowedLockoutStates = ['locked', 'unlocked'];

        if($lockoutFloor === false || !in_array($lockoutFloor, $allowedLockoutFloors, true)) {
            echo json_encode([
                'success' => false,
                'message' => 'Lockout floor must be between 1 and 6'
            ]);
            exit;
        }

        if(!in_array($submittedLockoutState, $allowedLockoutStates, true)) {
            echo json_encode([
                'success' => false,
                'message' => 'Invalid floor lockout state'
            ]);
            exit;
        }

        $updatedByUserID = $_SESSION['user_id'] ?? null;

        if($updatedByUserID === null) {
            echo json_encode([
                'success' => false,
                'message' => 'The logged in user could not be identified'
            ]);
            exit;
        }

        try {
            // lockout changes are allowed only while the elevator is in maintenance mode
            $query = "
                SELECT operation_mode
                FROM elevator_state
                WHERE state_id = 1
                LIMIT 1
            ";

            $statement = $pdo->prepare($query);
            $result = $statement->execute();
            $elevatorState = $statement->fetch();

            if(!$result || !$elevatorState) {
                throw new CommunicationException("Elevator operation mode could not be received");
            }

            if($elevatorState['operation_mode'] !== 'maintenance') {
                echo json_encode([
                    'success' => false,
                    'message' => 'Floor lockouts can only be changed while Maintenance mode is enabled',
                    'operation_mode' => $elevatorState['operation_mode']
                ]);
                exit;
            }

            $floorIsLocked = $submittedLockoutState === 'locked';
            $lockoutReason = $floorIsLocked ? 'Locked from elevator control webpage' : null;

            // INSERT creates a missing floor row; the duplicate-key branch updates an existing row
            $query = "
                INSERT INTO elevator_floor_lockouts (
                    floor_number,
                    is_locked,
                    lockout_reason,
                    updated_by_user_id
                )
                VALUES (
                    :floor_number,
                    :is_locked,
                    :lockout_reason,
                    :updated_by_user_id
                )
                ON DUPLICATE KEY UPDATE
                    is_locked = VALUES(is_locked),
                    lockout_reason = VALUES(lockout_reason),
                    updated_by_user_id = VALUES(updated_by_user_id)
            ";

            $statement = $pdo->prepare($query);
            $params = [
                'floor_number' => $lockoutFloor,
                'is_locked' => $floorIsLocked ? 1 : 0,
                'lockout_reason' => $lockoutReason,
                'updated_by_user_id' => $updatedByUserID
            ];

            $result = $statement->execute($params);

            if(!$result) {
                throw new CommunicationException("Floor lockout could not be updated");
            }

            echo json_encode([
                'success' => true,
                'message' => $floorIsLocked ? 'Floor locked successfully' : 'Floor restored successfully',
                'floor_number' => $lockoutFloor,
                'is_locked' => $floorIsLocked,
                'lockout_reason' => $lockoutReason,
                'updated_by_user_id' => (int) $updatedByUserID,
                'operation_mode' => 'maintenance'
            ]);
        } catch (CommunicationException $e) {
            error_log($e->getMessage());

            echo json_encode([
                'success' => false,
                'message' => $e->getMessage()
            ]);
        } catch (PDOException $e) {
            error_log($e->getMessage());

            echo json_encode([
                'success' => false,
                'message' => 'A database error prevented the floor lockout update'
            ]);
        }

        exit;
    }

    try {
        // receive JS values to move the elevator
        $submittedFloor = $_POST['requested_floor'] ?? '';
        $sourceController = $_POST['source_controller'] ?? '';

        // convert submitted floor into a valid integer
        $requestedFloor = filter_var($submittedFloor, FILTER_VALIDATE_INT);

        // create a 3-element array (since we have 3 floors... for now)
        $allowedFloors = [1, 2, 3];

        // reject any other values
        if($requestedFloor === false || !in_array($requestedFloor, $allowedFloors, true)) {
            throw new NodeInputException("Requested floor must be between 1 and 3");
        }

 

        // once again, only use valid sources
        $allowedSources = ['web_floor_station', 'web_car_controller'];

        // and once again, handle valid source controllers (from the web)
        // reject unknown controller names
        if(!in_array($sourceController, $allowedSources, true)) {
            throw new NodeInputException("Source controller is invalid");
        }

        // grab the user ID
        // login.php stored it in this session
        $requestedByUserID = $_SESSION['user_id'] ?? null;

        // if empty, get yo ahh outta here
        if($requestedByUserID === null){
            throw new NodeInputException("The logged in user could not be identified");
        }

        // OOP change
        // select the requested physical elevator floor node and mark its active request
        $selectedFloorNode = $floorNodes[$requestedFloor];
        

        // load the current door state, operation mode, and selected-floor lockout
        $query = "
            SELECT
                es.doors_open,
                es.operation_mode,
                COALESCE(efl.is_locked, 0) AS floor_locked
            FROM elevator_state AS es
            LEFT JOIN elevator_floor_lockouts AS efl
                ON efl.floor_number = :lockout_floor
            WHERE es.state_id = 1
            LIMIT 1
        ";

        // prep and execute
        $statement = $pdo->prepare($query);
        $result = $statement->execute([
            'lockout_floor' => $requestedFloor
        ]);

        // fetch from DB
        $elevatorState = $statement->fetch();

        // handle invalid state
        if(!$result || !$elevatorState){
            throw new CommunicationException("Elevator state could not be received");
        }

        // maintenance mode must not add anything to the elevator request queue
        if($elevatorState['operation_mode'] === 'maintenance') {
            echo json_encode([
                'success' => false,
                'message' => 'Floor requests are disabled while Maintenance mode is enabled',
                'operation_mode' => 'maintenance',
                'requested_floor' => $requestedFloor,
                'source_controller' => $sourceController
            ]);

            exit;
        }

        // a saved floor lockout remains active after maintenance mode is disabled
        if((int) $elevatorState['floor_locked'] === 1) {
            echo json_encode([
                'success' => false,
                'message' => 'Floor ' . $requestedFloor . ' is locked out of service',
                'operation_mode' => $elevatorState['operation_mode'],
                'requested_floor' => $requestedFloor,
                'source_controller' => $sourceController,
                'floor_locked' => true
            ]);

            exit;
        }

        // only contact and update the node objects after both DB checks pass
        $elevatorCar->verifyConnection();
        $selectedFloorNode->verifyConnection();

        $selectedFloorNode->requestElevator();

        // movement is unsafe if the saved door state cannot be read
        $doorsOpen = true;
        $elevatorCar->openDoors();

        if($result && $elevatorState) {
            // OOP change
            // store current door state
            $doorsState = (int) $elevatorState['doors_open'];
            $doorsOpen = ($doorsState & 1) === 1;

            // load the DB door state into the elevatorCar object
            if ($doorsOpen) {
                $elevatorCar->openDoors();
            } else {
                $elevatorCar->closeDoors();
            }
        } 

        // OOP change - check door state and determine movement
        $doorsOpen = $elevatorCar->areDoorsOpen();
        $movementAllowed = $elevatorCar->canMove();


        // requests with open doors can still be queued; maintenance and locked floors cannot

        
        // insert into DB
        $query = "
            INSERT INTO elevator_requests (
                request_type,
                requested_floor,
                requested_by_user_id,
                source_controller
            )
        
            SELECT
                :request_type,
                :requested_floor,
                :requested_by_user_id,
                :source_controller

            FROM elevator_state AS es
            LEFT JOIN elevator_floor_lockouts AS efl
                ON efl.floor_number = :lockout_floor
            WHERE es.state_id = 1
            AND es.operation_mode <> 'maintenance'
            AND COALESCE(efl.is_locked, 0) = 0
        ";

        $statement = $pdo->prepare($query);

        // OOP change - getFloorNumber
        $params = [
            'request_type' => 'remote',
            'requested_floor' => $selectedFloorNode->getFloorNumber(),
            'requested_by_user_id' => $requestedByUserID,
            'source_controller' => $sourceController,
            'lockout_floor' => $selectedFloorNode->getFloorNumber()
        ];

        $result = $statement->execute($params);

        // rowCount is zero if maintenance was enabled after the earlier state check
        if($result && $statement->rowCount() === 1) {
            // retrieve the auto-generated elevator_request_id
            // type-cast to int since we need it to be int for sending it back to JS
            $elevatorRequestID = (int) $pdo->lastInsertId();

            // send the successful query back to JS
            echo json_encode([
                'success' => true,
                'message' => 'elevator request logged successfully!',
                'elevator_request_id' => $elevatorRequestID,
                'requested_floor' => $selectedFloorNode->getFloorNumber(),
                'source_controller' => $sourceController,

                // if logged successfully, tell JS what PHP found in elevator_state DB (for doors)
                'doors_open' => $doorsOpen,
                'movement_allowed' => $movementAllowed
            ]);
        // the query ran but maintenance or a floor lockout prevented the INSERT
        } elseif($result) {
            echo json_encode([
                'success' => false,
                'message' => 'Request blocked because Maintenance mode or a floor lockout became active',
                'requested_floor' => $selectedFloorNode->getFloorNumber(),
                'source_controller' => $sourceController
            ]);

        // query failed/request not logged
        } else {
            throw new CommunicationException("the elevator request could not be sent");
        }
    }
    catch (NodeInputException $e) {
        error_log($e->getMessage());

        // print the error message
        echo json_encode([
            'success' => false,
            'exception_type' => get_class($e),
            'message' => $e->getMessage()
        ]);
    }
    catch (CommunicationException $e) {
        error_log($e->getMessage());

        // print the error message
        echo json_encode([
            'success' => false,
            'exception_type' => get_class($e),
            'message' => $e->getMessage()
        ]);
    }

    catch (PDOException $e) 
    {
        error_log($e->getMessage());

        // report message back to JS too
        echo json_encode([
            'success' => false,
            'exception_type' => get_class($e),
            'message' => 'database communication failure (PDO catch)'
        ]);
    }
    exit;
}


// create a dynamic default display if there are no elevator requests - pull from DB to keep it updated:
$initialFloor = 1;
$initialRequestID = null;
$initialSource = 'floor';
$initialStatus = 'idle';

try {
    // query has no user input so a prep statement doesn't have to be used:
    $query = "
        SELECT
            elevator_request_id,
            requested_floor,
            source_controller,
            request_status
        FROM elevator_requests
        ORDER BY elevator_request_id DESC
        LIMIT 1    
    ";

    // run the query
    $statement = $pdo->query($query);

    // retireve the newest row or false if empty
    $latestRequest = $statement->fetch();

    if($latestRequest) {
        $initialFloor = (int) $latestRequest['requested_floor'];
        $initialRequestID = (int) $latestRequest['elevator_request_id'];
        $initialStatus = $latestRequest['request_status'];

        if($latestRequest['source_controller'] === 'web_car_controller') {
            $initialSource = 'car';
        }

    }
} catch (PDOException $e) {
    // Keep the default Floor 1/System idle display if loading fails.
    error_log($e->getMessage());
}

// default vused if the DB state cannot be loaded:
$initialDoorsOpen = false;
// additionally, default operation mode to normal until DB is queried:
$initialOperationMode = 'normal';

try {
    // read from DB to load page with the data
    $query = "
        SELECT doors_open, operation_mode
        FROM elevator_state
        WHERE state_id = 1
        LIMIT 1
    ";

    $statement = $pdo->prepare($query);
    $result = $statement->execute();

    $elevatorState = $statement->fetch();

    // maria DB returns 0 or 1 for door state, convert it into a PHP bool
    // set the operation mode too
    if($elevatorState) {
        $doorsState = (int) $elevatorState['doors_open'];
        $initialDoorsOpen = ($doorsState & 1) === 1;
        $initialOperationMode = $elevatorState['operation_mode'];
    }
} catch (PDOException $e) {
    error_log($e->getMessage());
}

// load every saved per-floor lockout for the initial webpage display
$initialFloorLockouts = [];

for($floorNumber = 1; $floorNumber <= 6; $floorNumber++) {
    $initialFloorLockouts[$floorNumber] = false;
}

try {
    $query = "
        SELECT floor_number, is_locked
        FROM elevator_floor_lockouts
        ORDER BY floor_number
    ";

    $statement = $pdo->query($query);

    while($floorLockout = $statement->fetch()) {
        $floorNumber = (int) $floorLockout['floor_number'];

        if($floorNumber >= 1 && $floorNumber <= 6) {
            $initialFloorLockouts[$floorNumber] = ((int) $floorLockout['is_locked']) === 1;
        }
    }
} catch (PDOException $e) {
    // keep the unlocked defaults if the lockout table cannot be loaded
    error_log($e->getMessage());
}

$safeUsername = htmlspecialchars($_SESSION['username'] ?? 'Member', ENT_QUOTES, 'UTF-8');
?>

<!DOCTYPE html>
<html lang="en">

<head>
    <title>LiF Team - Elevator Control</title>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <link rel="icon" href="../media/icons/LiF_icon.ico" type="image/x-icon">

    <meta name="author" content="Nick Kapuka">
    <meta name="description" content="Protected elevator control interface for the LiF Team website.">
    <meta name="robots" content="noindex, nofollow">
    <meta http-equiv="Pragma" content="no-cache">

    <link rel="stylesheet" href="../css/base.css">
    <link rel="stylesheet" href="../css/elevator_control.css">
</head>

<body>
    <!-- nav bar -->
    <div id="navbar-placeholder" data-root="../"></div>
    <script src="../js/navbar.js"></script>

    <main class="page-wrapper <?php echo $initialOperationMode === 'maintenance' ? 'maintenance-active' : ''; ?>"
        id="elevatorControlPage" data-initial-floor="<?php echo $initialFloor; ?>"
        data-initial-request-id="<?php echo $initialRequestID; ?>" data-initial-source="<?php echo $initialSource; ?>"
        data-operation-mode="<?php echo htmlspecialchars($initialOperationMode, ENT_QUOTES, 'UTF-8'); ?>"
        data-is-admin="<?php echo (($_SESSION['user_role'] ?? '') === 'admin') ? 'true' : 'false'; ?>"
        data-floor-lockouts="<?php echo htmlspecialchars(json_encode($initialFloorLockouts), ENT_QUOTES, 'UTF-8'); ?>">
        <section class="intro-section elevator-control-intro" id="page_top">

            <div class="intro-text">
                <p class="section-label">Protected Control Panel</p>

                <h1>Elevator Control Room</h1>

                <p class="intro-description">
                    Welcome, <?php echo $safeUsername; ?>. This page allows you, yes YOU, to control the humble elevator
                    platform!
                    The floor stations can request the car to go to them, while the car controller can send the cab to
                    any floor.
                </p>

                <div class="button-row">
                    <a href="members.php" class="button secondary-button">Back to Members</a>
                    <a href="logout.php" class="button primary-button">Log Out</a>
                </div>
            </div>



            <aside class="summary-card">
                <h2>Control Status</h2>
                <p><strong>Mode:</strong> Elevator control</p>
                <p><strong>Floors:</strong> 3 active / 6 displayed</p>
                <p><strong>System:</strong> Floor calls + car controller</p>
                <p><strong>PHP Model:</strong> <?php echo Node::getNodeCount(); ?> node objects</p>
                <p>
                    <strong>CAN Interface Test:</strong>
                    <?php echo $canDeviceTestPassed ? "PASS" : "FAIL"; ?>
                </p>

                <p>
                    <strong>CAN Devices:</strong>
                    <?php echo htmlspecialchars(implode(", ", $canDeviceDetails), ENT_QUOTES, "UTF-8"); ?>
                </p>
                <p><strong>Doors:</strong> <?php echo $initialDoorsOpen ? 'Open' : 'Closed'; ?></p>
            </aside>
        </section>

        <!-- main page -->
        <section class="elevator-panel">
            <div class="control-hud">
                <div>
                    <p class="section-label">Simulation display</p>
                    <h2>Cab Status</h2>
                </div>

                <div class="hud-readout">
                    <span class="hud-label">Current Floor</span>
                    <span id="currentFloorDisplay" class="hud-value"><?php echo $initialFloor;?></span>
                </div>

                <div class="hud-readout">
                    <span class="hud-label">Last Command</span>
                    <span id="lastCommandDisplay" class="hud-value">
                        <?php if($initialRequestID === null):?>
                        System Idle
                        <?php else: ?>
                        #Request #<?php echo $initialRequestID;?>
                        for floor <?php echo $initialFloor;?>
                        -- <?php echo htmlspecialchars($initialStatus, ENT_QUOTES, 'UTF-8');?>
                        <?php endif; ?>
                    </span>
                </div>
            </div>

            <aside class="maintenance-status-rail" id="maintenanceStatusRail" role="status" aria-live="polite"
                aria-atomic="true" <?php echo $initialOperationMode === 'maintenance' ? '' : 'hidden'; ?>>
                <span class="maintenance-status-light" aria-hidden="true"></span>

                <div class="maintenance-status-copy">
                    <span>Restricted operation</span>
                    <strong>Maintenance Mode Active</strong>
                    <small>Passenger requests are paused while authorized personnel configure floor access.</small>
                </div>
            </aside>

            <div class="elevator-layout">
                <section class="building-tower">
                    <?php for($floorNumber = 6; $floorNumber >= 1; $floorNumber--): ?>
                    <div class="floor-row <?php echo $floorNumber === $initialFloor ? 'active-floor' : ''; ?>"
                        data-floor="<?php echo $floorNumber; ?>">
                        <div class="floor-label">
                            <span>Floor <?php echo $floorNumber; ?></span>
                            <small>
                                <?php echo $floorNumber <= 3 ? 'Station F' . $floorNumber : 'Future station'; ?>
                            </small>
                        </div>

                        <div class="shaft" aria-hidden="true">
                            <div class="floor-line"></div>

                            <?php if($floorNumber === 6): ?>
                            <div class="elevator-car" id="elevatorCar">
                                <span class="car-screen"><?php echo $initialFloor; ?></span>
                                <span class="car-door"></span>
                            </div>
                            <?php endif; ?>
                        </div>

                        <button type="button"
                            class="floor-request-button <?php echo $floorNumber > 3 ? 'interface-preview' : ''; ?>"
                            data-floor="<?php echo $floorNumber; ?>"
                            data-interface-only="<?php echo $floorNumber > 3 ? 'true' : 'false'; ?>"
                            title="<?php echo $floorNumber > 3 ? 'Interface preview - floor node not configured yet' : 'Request the elevator'; ?>">
                            Request Car
                        </button>
                    </div>
                    <?php endfor; ?>
                </section>

                <aside class="car-controller">
                    <p class="section-label">Car Controller</p>
                    <h2>Cab Destination</h2>

                    <p>
                        These buttons will behave as the buttons inside the elevator cab. Select a destination
                        floor to move the car in the visual demo.
                    </p>

                    <div class="car-button-stack" aria-label="Cab floor buttons">
                        <?php for($floorNumber = 6; $floorNumber >= 1; $floorNumber--): ?>
                        <button type="button"
                            class="car-floor-button <?php echo $floorNumber === $initialFloor ? 'active-car-button' : ''; ?> <?php echo $floorNumber > 3 ? 'interface-preview' : ''; ?>"
                            data-floor="<?php echo $floorNumber; ?>"
                            data-interface-only="<?php echo $floorNumber > 3 ? 'true' : 'false'; ?>"
                            title="<?php echo $floorNumber > 3 ? 'Interface preview - floor node not configured yet' : 'Go to Floor ' . $floorNumber; ?>">
                            <?php echo $floorNumber; ?>
                        </button>
                        <?php endfor; ?>
                    </div>

                    <div class="mode-control-stack">
                        <div class="door-control-panel"
                            data-door-state="<?php echo $initialDoorsOpen ? 'open' : 'closed';?>">
                            <p class="section-label"><b>Door Status</b>
                                <span id="doorStatusDisplay">Closed</span>
                            </p>

                            <button type="button" id="doorToggleButton" class="door-toggle-button">Open Doors</button>
                        </div>

                        <div class="sabbath-toggle"
                            data-operation-mode="<?php echo htmlspecialchars($initialOperationMode, ENT_QUOTES, 'UTF-8'); ?>">
                            <p class="section-label"><b>Sabbath Toggle</b></p>

                            <button type="button" id="sabbathToggle" class="sabbath-toggle-button">
                                Sabbath Mode
                            </button>
                        </div>

                        <?php if(($_SESSION['user_role'] ?? '') === 'admin'): ?>
                        <div class="sabbath-toggle maintenance-toggle">
                            <p class="section-label"><b>Maintenance Toggle</b></p>

                            <button type="button" id="maintenanceToggle"
                                class="sabbath-toggle-button maintenance-toggle-button">
                                Maintenance Mode
                            </button>
                        </div>
                        <?php endif; ?>
                    </div>
                </aside>
            </div>

            <?php if(($_SESSION['user_role'] ?? '') === 'admin'): ?>
            <section class="floor-lockout-panel" id="floorLockoutPanel"
                <?php echo $initialOperationMode === 'maintenance' ? '' : 'hidden'; ?>>
                <div class="lockout-heading">
                    <div>
                        <p class="section-label">Maintenance Mode</p>
                        <h2>Single-Floor Lockout</h2>
                    </div>
                    <span class="preview-flag">Database Synced</span>
                </div>

                <p class="lockout-help">
                    Select individual floors to mark them out of service. Saved lockouts remain active until restored.
                </p>

                <div class="floor-lockout-grid">
                    <?php for($floorNumber = 1; $floorNumber <= 6; $floorNumber++): ?>
                    <div class="lockout-floor" data-floor="<?php echo $floorNumber; ?>">
                        <span class="lockout-number"><?php echo $floorNumber; ?></span>
                        <span class="lockout-copy">
                            <strong>Floor <?php echo $floorNumber; ?></strong>
                            <small>Available</small>
                        </span>
                        <button type="button" class="floor-lockout-button" data-floor="<?php echo $floorNumber; ?>">
                            Lock Floor
                        </button>
                    </div>
                    <?php endfor; ?>
                </div>
            </section>
            <?php endif; ?>
        </section>

        <p class="top-link">
            <a href="#page_top">Go to the top of this page</a>
        </p>
    </main>

    <footer class="site-footer">
        <p>Copyright &copy; 2026 Nick Kapuka</p>
    </footer>

    <script src="../js/elevator_control.js"></script>
</body>

</html>