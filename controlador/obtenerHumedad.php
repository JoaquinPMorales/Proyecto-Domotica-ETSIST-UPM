<?php
session_start();
include 'direccionLogs.php';
if(isset($_GET["nombreDispositivo"])){
	$nombreDispositivo = $_GET["nombreDispositivo"];
	$dirFichero = $direccionLogs . $nombreDispositivo . "_humedad.txt";
	if(file_exists($dirFichero)){
		$ficheroHumedad = file($dirFichero);
		$ultimaLineaHumedad = count($ficheroHumedad) - 1; 
		echo "<div id='" . $nombreDispositivo . "'>" . $ficheroHumedad[$ultimaLineaHumedad] . "</div>";
		unset($_SESSION["errorFichero".$nombreDispositivo]);
	}
	else{
		echo "<div id='" . $nombreDispositivo  . "'>Error</div>";
		if (!isset($_SESSION["errorFichero".$nombreDispositivo])){
			$file = fopen($direccionLogs . "controlador_log.txt", "a");
			fwrite($file, date('Y-m-d H:i:s') . ": obtenerHumedad.php: el fichero ". $dirFichero . " no existe" . PHP_EOL);
			fclose($file);
			$_SESSION['errorFichero'.$nombreDispositivo] = "si";
		}
		
	}
}
else{
	echo "<div id='" . $nombreDispositivo  . "'>Error</div>";
	$file = fopen($direccionLogs . "controlador_log.txt", "a");
	fwrite($file, date('Y-m-d H:i:s') . ": obtenerHumedad.php: Han llegado parametros erroneos" . PHP_EOL);
	fclose($file);
}
?>
