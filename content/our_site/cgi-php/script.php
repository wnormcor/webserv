#! /usr/bin/php
<?php
echo "Status: 200 OK\r\n";
echo "Content-Type: text/html\r\n\r\n";
?>


<html>
<body>
<?php


parse_str($_ENV["QUERY_STRING"], $result);
echo "<h2>Username: " . $result['login']. "</h2></br>";
echo "<h2>Password: " . $result['pass'] . "</h2></br>";
?>

</body>
</html>