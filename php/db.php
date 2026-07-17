<?php
    $host = 'localhost';
    $dbname = 'lif_elevator';
    $dbuser = 'LiF_Admin';
    $dbpass = 'LiF_ESE';
    try {
        $pdo = new PDO("mysql:host=$host;dbname=$dbname;charset=utf8mb4", $dbuser, $dbpass);

        // return rows as arrays using database field names as keys
        // michael calls this 'db' but I already had it set to pdo so Im not changing like 10 files
        $pdo->setAttribute(PDO::ATTR_DEFAULT_FETCH_MODE, PDO::FETCH_ASSOC);

        $pdo->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);

    } catch (PDOException $e) {
        die("Database connection failed: " . $e->getMessage());
    }