<?php
include 'direccionLogs.php';
if (isset($_POST["dispositivo"]) && isset($_POST["info"])){

// Primero hacer log
	$fichero = $direccionLogs . $_POST["dispositivo"] . "_log" . ".txt";
	if(file_exists($fichero)){
		$file = fopen($fichero, "a");
		fwrite($file, date('Y-m-d H:i:s') . ": " . $_POST["info"] . PHP_EOL);
		fclose($file);
	}
	else{
		$file = fopen($direccionLogs . "controlador_log.txt", "a");
		fwrite($file, date('Y-m-d H:i:s') . ": grabarLog.php: el fichero ". $fichero . " no existe" . PHP_EOL);
		fclose($file);
	}
	
}
else
	$file = fopen($direccionLogs . "controlador_log.txt", "a");
	fwrite($file, date('Y-m-d H:i:s') . ": grabarLog.php: Han llegado parametros erroneos en la peticion ". PHP_EOL);
	fclose($file);

?>
