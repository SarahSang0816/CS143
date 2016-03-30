<html>

<h1>Database Query</h1>

<style>
body {
    background-color: #d0e4fe;
}

h1 {
    color: Green;
    text-align: center;
}

h2 {
    color: Orange;
    font-family: "verdana";
}

table, th, td {
    border: 1.5px solid black;
    border-collapse: collapse;
    font-family: "Arial";
}
tr:nth-child(odd)		{ background-color:"pink"; }
tr:nth-child(even)		{ background-color:#fff; }

</style>

<body>
<p>
<font size="4" color="Orange" face="verdana"><b>Type a SQL query in the following box:</b></font>
<br>
<font size="3" color="Grey">Example: SELECT * FROM Actor WHERE id=10;</font>
<br>
<font size="2" color="purple">(Ver 1.0 15/10/2014 by Yu Zhang, Qi Sang)</font>
<br>
</p>

<form action="<?php echo $_SERVER['PHP_SELF'];?>" method="get">
<textarea name="query" cols="60" rows="10"></textarea>
<br>
<input type="submit" value="Submit">
</form>
<br>

</body>
</html>

<?php
/* 	establishing a coonection with MYSQL
	connect to the MySQL database system on the machine "localhost"
	and specify a particular database CS143 we'll be accessing
*/
function connect() {
	$db_connection = mysql_connect("localhost", "cs143", "");
	mysql_select_db("CS143", $db_connection);
	return $db_connection;
}

// issuing queries, and queries are passed from PHP 
function query($query, $db_connection) {
	if (!$db_connection) {
		$errmsg = mysql_error($db_connection);
		echo "Connection failed:" .  $errmsg . "<br/>";
		exit(1);
	}
	else {
		$rs = mysql_query($query, $db_connection);
		return $rs;
	}
}

/*
Tables are defined with the <table> tag.
Tables are divided into table rows with the <tr> tag.
Table rows are divided into table data with the <td> tag.
A table row can also be divided into table headings with the <th> tag.
*/
function printResult($rs) {
	if (!$rs) {
		echo 'Could not run query: ' . mysql_error();
    	exit;
	}
	// get the #collum of the result table
	$fields_num = mysql_num_fields($rs);
	// get the #row of the result table
	$rows_num = mysql_num_rows ($rs);
	// if no tuple is qualified, echo message
	if ($rows_num == 0) {
		echo "No qualified tuple found";
		return;
	}

	// print out table
	echo '<table border="1" style="width:50%" bgcolor="Pink"';
	// print out attribute names
	echo '<tr>';
    for($j = 0; $j < $fields_num; $j++) {
        $field = mysql_fetch_field($rs);
        echo "<td>{$field->name}</td>";
    }
    echo '</tr>';

	// print out each tuple
    while($row = mysql_fetch_row($rs)) {
        echo "<tr>";
        $size = sizeof($row);
		for ($i = 0; $i < $size; $i++) {
			if (is_null($row[$i])) {
				echo "<td>N/A</td>";
			}
			else {
				echo '<td>' . $row[$i]. '</td>';
			}	
		}
		echo '</tr>';
    }
	echo '</table>';
}


// close connection once you are done
function disconnect($db_connection) {
	mysql_close($db_connection);
}
?>

<?php
if($_GET["query"]) {
	echo "<h2>Result</h2>"; 
	$db_connection = connect();
	$rs = query($_GET["query"], $db_connection);
    printResult($rs);
	disconnect($db_connection);
}
?>

