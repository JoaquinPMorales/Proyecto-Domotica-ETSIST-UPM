<?php
include 'direccionLogs.php';
if (isset($_POST["dispositivo"]) && isset($_POST["temperatura"])){
	$ficheroTemperatura = $direccionLogs . $_POST["dispositivo"] . "_temperatura" . ".txt";
	if(file_exists($ficheroTemperatura)){
		$file = fopen($ficheroTemperatura, "a");
		fwrite($file, $_POST["temperatura"] . "C" . PHP_EOL);
		fclose($file);
	}
	else{
		$file = fopen($direccionLogs . "controlador_log.txt", "a");
		fwrite($file, date('Y-m-d H:i:s') . ": guardarDatosSensor.php: el fichero ". $ficheroTemperatura . " no existe" . PHP_EOL);
		fclose($file);
	}
	
}
else if(isset($_POST["dispositivo"]) && isset($_POST["humedad"])){
	$ficheroHumedad = $direccionLogs . $_POST["dispositivo"] . "_humedad" . ".txt";
	if(file_exists($ficheroHumedad)){
		$file = fopen($ficheroHumedad, "a");
		fwrite($file, $_POST["humedad"] . "%" . PHP_EOL);
		fclose($file);
	}
	else{
		$file = fopen($direccionLogs . "controlador_log.txt", "a");
		fwrite($file, date('Y-m-d H:i:s') . ": guardarDatosSensor.php: el fichero ". $ficheroHumedad . " no existe" . PHP_EOL);
		fclose($file);
	}

}
else
	$file = fopen($direccionLogs . "controlador_log.txt", "a");
	fwrite($file, date('Y-m-d H:i:s') . ": guardarDatosSensor.php: Han llegado parametros erroneos" . PHP_EOL);
	fclose($file);

?>