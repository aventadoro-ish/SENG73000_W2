<?php
    $host = 'localhost';
    $dbname = 'lif_elevator';
    $dbuser = 'LiF_Admin';
    $dbpass = 'LiF_ESE';
    try {
        $pdo = new PDO("mysql:host=$host;dbname=$dbname;charset=utf8mb4", $dbuser, $dbpass);

        $pdo->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
    } catch (PDOException $e) {
        die("Database connection failed: " . $e->getMessage());
    }