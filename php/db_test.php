<?php

// load db.php but only once
// __DIR__ is start from folder containing PHP files
require_once __DIR__ . '/db.php';

echo "database connection successful!";