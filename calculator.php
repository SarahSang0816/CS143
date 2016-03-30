<html>
<body>

<h1>Calculator</h1>

<p>
(Ver 1.0 3/10/2014 by Yu Zhang, Qi Sang)<br>
Type an expression in the following box (e.g., 10.5+20*3/25).
</p>

<form action="<?php echo $_SERVER['PHP_SELF'];?>" method="GET">
<input type="text" name="expression"/>
<input type="submit" value="Calculate"/>
</form>

<ul>
	<li>Only numbers and +,-,* and / operators are allowed in the expression.</li>
	<li>The evaluation follows the standard operator precedence.</li>
	<li>The calculator does not support parentheses.</li>
	<li>The calculator handles invalid input "gracefully". It does not output PHP error messages.</li>
</ul>
<p>Here are some(but not limit to) reasonable test cases:</p>
<ol>
	<li>A basic arithmetic operation: 3+4*5=23</li>
	<li>An expression with floating point or negative sign : -3.2+2*4-1/3 = 4.46666666667, 3*-2.1*2 = -12.6</li>
	<li>Some typos inside operation (e.g. alphabetic letter): Invalid input expression 2d4+1</li>
</ol>

<?php
	if($_GET["expression"]){
	$test = $_GET["expression"];
	$test = preg_replace('/\s+/', '', $test);

	//'((\-)?[0-9]+(\.[0-9]+)?)';  pattern for numbers
	//'(\+|\-|\*|\/)';  pattern for operators

	echo "<h2>Result</h2>"; 

	if(preg_match('/^((\-)?[0-9]+(\.[0-9]+)?)((\+|\-|\*|\/)(\-)?[0-9]+(\.[0-9]+)?)*$/', $test)){
		if(preg_match('(/0)', $test)){ 
			echo "Division by zero error! <br>";
		}
		else{
			eval("\$ans=$test;");
			echo $test." = ".$ans;
		}
	}

	else{
		echo "Invalid Expression!";
	}
}
?>  

</body>
</html>