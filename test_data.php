<?php

$hostname = "localhost";
$username = "root";
$password = "";
$database = "hydrosense";

$conn = mysqli_connect($hostname, $username, $password, $database);

if (!$conn) {
    die("Connection failed: " . mysqli_connect_error());
}

echo "Database connection is OK ";

// Check if any of the required parameters are present
if (isset($_REQUEST["Time"]) && isset($_REQUEST["Liters"]) && isset($_REQUEST["PersonType"]) && isset($_REQUEST["Temperature"])) {

    $T = $_REQUEST["Time"];
    $L = $_REQUEST["Liters"];
    $P = $_REQUEST["PersonType"];
    $t = $_REQUEST["Temperature"];

    $sql = "INSERT INTO data (Time, Liters, PersonType, Temperature) VALUES ('$T', '$L', '$P', '$t')";

    if (mysqli_query($conn, $sql)) {
        echo "New record created successfully";
    } else {
        echo "Error: " . $sql . "<br>" . mysqli_error($conn);
    }
} else {
    echo "One or more parameters are missing.";
}

?>
