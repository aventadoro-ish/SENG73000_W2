:<?php
    $host = 'localhost';
    $dbname = 'elevatorDB';
    $dbuser = 'elevator_user';
    $dbpass = 'ChooseAPassword123';
    try {
        $pdo = new PDO("mysql:host=$host;dbname=$dbname;charset=utf8", $dbuser, $dbpass);
        $pdo->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
    } catch (PDOException $e) {
        die("Database connection failed: " . $e->getMessage());
    }
    ?>