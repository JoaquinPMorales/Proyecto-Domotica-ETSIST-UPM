<?php
session_start();
include 'direccionLogs.php';
if(isset($_GET["nombreDispositivo"])){
	$nombreDispositivo = $_GET["nombreDispositivo"];
	$dirFichero = $direccionLogs . $nombreDispositivo . "_temperatura.txt";
	if(file_exists($dirFichero)){
		$ficheroTemperatura = file($dirFichero);
		$ultimaLineaTemperatura = count($ficheroTemperatura) - 1; 
		echo "<div id='" . $nombreDispositivo . "'>" . $ficheroTemperatura[$ultimaLineaTemperatura] . "</div>";
		unset($_SESSION["errorFichero".$nombreDispositivo]);
	}
	else{
		echo "<div id='" . $nombreDispositivo  . "'>Error</div>";
		 if (!isset($_SESSION["errorFichero".$nombreDispositivo])){
			 $file = fopen($direccionLogs . "controlador_log.txt", "a");
			fwrite($file, date('Y-m-d H:i:s') . ": obtenerTemperatura.php: el fichero ". $dirFichero . " no existe" . PHP_EOL);
			fclose($file);
			$_SESSION['errorFichero'.$nombreDispositivo] = "si";
		 }
		
	}
}
else{
	echo "<div id='" . $nombreDispositivo  . "'>Error</div>";
	$file = fopen($direccionLogs . "controlador_log.txt", "a");
	fwrite($file, date('Y-m-d H:i:s') . ": obtenerTemperatura.php: Han llegado parametros erroneos" . PHP_EOL);
	fclose($file);
}
?>
