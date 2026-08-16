<?php 

require_once __DIR__ . '/classes/elevatorCar.php';
require_once __DIR__ . '/classes/floorNode.php';
require_once __DIR__ . '/classes/distanceSensor.php';

// create one new elevator car object
$elevatorCar = new ElevatorCar("EC", "0x101", 1);

// create one object for each floor
$floor1 = new FloorNode("F1", "0x201", 1);
$floor2 = new FloorNode("F2", "0x202", 2);
$floor3 = new FloorNode("F3", "0x203", 3);

// make a distance sensor object
$distanceSensor = new DistanceSensor("DS", 0.0);

// simulate a floor 2 request and elevator response
$floor2->requestElevator();
$elevatorCar->canMove();
$elevatorCar->openDoors();
$distanceSensor->setDistanceMm(840.0);

$objects = [
    [
        "type" => $elevatorCar->getNodeType(),
        "name" => $elevatorCar->getNodeName(),
        "can_id" => $elevatorCar->getCANID(),
        "state" => "Floor" . $elevatorCar->getCurrentFloor() . ", doors " . ($elevatorCar->areDoorsOpen() ? "open" : "closed")
    ],

    [
        "type" => $floor1->getNodeType(),
        "name" => $floor1->getNodeName(),
        "can_id" => $floor1->getCANID(),
        "state" => $floor1->hasActiveRequest() ? "request active" : "no request"
    ],

    [
        "type" => $floor2->getNodeType(),
        "name" => $floor2->getNodeName(),
        "can_id" => $floor2->getCANID(),
        "state" => $floor2->hasActiveRequest() ? "request active" : "no request"
    ],

    [
        "type" => $floor3->getNodeType(),
        "name" => $floor3->getNodeName(),
        "can_id" => $floor3->getCANID(),
        "state" => $floor3->hasActiveRequest() ? "request active" : "no request"
    ],

    [
        "type" => $distanceSensor->getNodeType(),
        "name" => $distanceSensor->getNodeName(),
        "can_id" => "connected to EC",
        "state" => $distanceSensor->getDistanceMm() . " mm"
    ]

];
?>


<!DOCTYPE html>
<html lang="en">

<head>
    <meta charset="UTF-8">
    <title>Elevator OOP Test</title>
</head>

<body>
    <h1>Elevator OOP Test</h1>

    <p>
        <strong>Total node objects:</strong>
        <?php echo Node::getNodeCount(); ?>
    </p>

    <table border="1" cellpadding="8">
        <thead>
            <tr>
                <th>Node Type</th>
                <th>Node</th>
                <th>CAN ID</th>
                <th>Current State</th>
            </tr>
        </thead>

        <tbody>
            <?php foreach($objects as $object): ?>
            <tr>
                <td><?php echo htmlspecialchars($object["type"], ENT_QUOTES, "UTF-8"); ?></td>
                <td><?php echo htmlspecialchars($object["name"], ENT_QUOTES, "UTF-8"); ?></td>
                <td><?php echo htmlspecialchars($object["can_id"], ENT_QUOTES, "UTF-8"); ?></td>
                <td><?php echo htmlspecialchars($object["state"], ENT_QUOTES, "UTF-8"); ?></td>
            </tr>
            <?php endforeach; ?>
        </tbody>
    </table>
</body>

</html>