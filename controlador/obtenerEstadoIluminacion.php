<?php 
session_start();
include 'direccionLogs.php';
if(isset($_GET["nombreActuadorIluminacion"])){
	$nombreDispositivo = $_GET["nombreActuadorIluminacion"];
	$dirFichero = $direccionLogs . $nombreDispositivo  . "_estadoActuador.txt";
	if(file_exists($dirFichero)){
		$ficheroEstadoIluminacion = file($dirFichero);
		$ultimaLinea = count($ficheroEstadoIluminacion) - 1; 
		echo "<div id='" . $nombreDispositivo . "'>" . $ficheroEstadoIluminacion[$ultimaLinea] . "</div>";
		unset($_SESSION["errorFichero".$nombreDispositivo]);
	}
	else{
		echo "<div id='" . $nombreDispositivo  . "'>Error</div>";
		if (!isset($_SESSION["errorFichero".$nombreDispositivo])){
			$file = fopen($direccionLogs . "controlador_log.txt", "a");
			fwrite($file, date('Y-m-d H:i:s') . ": obtenerEstadoIluminacion.php: el fichero ". $dirFichero . " no existe" . PHP_EOL);
			fclose($file);
			$_SESSION['errorFichero'.$nombreDispositivo] = "si";
		}
	}
}
else{
	echo "<div id='" . $nombreDispositivo  . "'>Error</div>";
	$file = fopen($direccionLogs . "controlador_log.txt", "a");
	fwrite($file, date('Y-m-d H:i:s') . ": obtenerEstadoIluminacion.php: Han llegado parametros erroneos" . PHP_EOL);
	fclose($file);
}

?>
