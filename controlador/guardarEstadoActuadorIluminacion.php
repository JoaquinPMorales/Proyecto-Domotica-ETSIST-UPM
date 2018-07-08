<?php
include 'direccionLogs.php';
if (isset($_POST["estadoIluminacion"]) && isset($_POST["nombreDispositivo"]) ){
	echo "Guardando el estado de iluminacion<br><br>";
	$fichero = $direccionLogs . $_POST["nombreDispositivo"] . "_estadoActuador.txt";
	if(file_exists($fichero)){
		$file = fopen($fichero, "a");
		fwrite($file, $_POST["estadoIluminacion"] . PHP_EOL);
		fclose($file);
	}
	else{
		$file = fopen($direccionLogs . "controlador_log.txt", "a");
		fwrite($file, date('Y-m-d H:i:s') . ": guardarEstadoActuadorIluminacion.php: el fichero ". $fichero . " no existe" . PHP_EOL);
		fclose($file);
	}
}
else
		$file = fopen($direccionLogs . "controlador_log.txt", "a");
		fwrite($file, date('Y-m-d H:i:s') . ": guardarEstadoActuadorIluminacion.php: Han llegado parametros erroneos  " . PHP_EOL);
		fclose($file);
?>