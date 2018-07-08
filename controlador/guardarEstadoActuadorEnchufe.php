<?php
include 'direccionLogs.php';
if (isset($_POST["estadoEnchufe"]) && isset($_POST["nombreDispositivo"]) ){
	$fichero = $direccionLogs . $_POST["nombreDispositivo"] . "_estadoActuador.txt";
	if(file_exists($fichero)){
		$file = fopen($fichero, "a");
		fwrite($file, $_POST["estadoEnchufe"] . PHP_EOL);
		fclose($file);
	}
	else{
		$file = fopen($direccionLogs . "controlador_log.txt", "a");
		fwrite($file, date('Y-m-d H:i:s') . ": guardarEstadoActuadorEnchufe.php: el fichero ". $fichero . " no existe" . PHP_EOL);
		fclose($file);
	}
	
}
else
	$file = fopen($direccionLogs . "controlador_log.txt", "a");
	fwrite($file, date('Y-m-d H:i:s') . ": guardarEstadoActuadorEnchufe.php: Han llegado parametros erróneos" . PHP_EOL);
	fclose($file);
?>