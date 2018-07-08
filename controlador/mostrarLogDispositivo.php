<?php
include 'direccionLogs.php';
if (isset($_GET["nombreDispositivo"])){
	$nombreDispositivo = $_GET["nombreDispositivo"];
	$ficheroLog = $direccionLogs . $nombreDispositivo . "_log.txt";
	if(file_exists($ficheroLog)){
		$file = fopen($ficheroLog, "r");
		while(!feof($file)) {
		echo fgets($file). "<br />";
		}
		fclose($file);
	}
	else{
		echo "<h2> Error. El fichero " . $ficheroLog . " no existe</h2>";
	}
}
else
	echo "<h2>Parámetros erroneos recibidos</h2>";
?>
<form action="./">
    <input type="submit" value="Vuelta a la pagina principal" />
</form>