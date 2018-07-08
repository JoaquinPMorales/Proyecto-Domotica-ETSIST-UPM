<?php 
session_start();
include 'direccionLogs.php';
if(isset($_GET["nombreActuadorEnchufe"])){
	$nombreDispositivo = $_GET["nombreActuadorEnchufe"];
	$dirFichero = $direccionLogs . $nombreDispositivo  . "_estadoActuador.txt";
	if(file_exists($dirFichero)){
		$ficheroEstadoEnchufe = file($dirFichero);
		$ultimaLinea = count($ficheroEstadoEnchufe) - 1; 
		echo "<div id='" . $nombreDispositivo . "'>" . $ficheroEstadoEnchufe[$ultimaLinea] . "</div>";
		unset($_SESSION["errorFichero".$nombreDispositivo]);
	}
	else{
		echo "<div id='" . $nombreDispositivo  . "'>Error</div>";
		if (!isset($_SESSION["errorFichero".$nombreDispositivo])){
			$file = fopen($direccionLogs . "controlador_log.txt", "a");
			fwrite($file, date('Y-m-d H:i:s') . ": obtenerEstadoEnchufe.php: el fichero ". $dirFichero . " no existe" . PHP_EOL);
			fclose($file);
			$_SESSION['errorFichero'.$nombreDispositivo] = "si";
		}
	}
}
else{
	echo "<div id='" . $nombreDispositivo  . "'>Error</div>";
	$file = fopen($direccionLogs . "controlador_log.txt", "a");
	fwrite($file, date('Y-m-d H:i:s') . ": obtenerEstadoEnchufe.php: Han llegado parametros erroneos" . PHP_EOL);
	fclose($file);
}

?>