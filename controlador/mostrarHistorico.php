<?php
include 'direccionLogs.php';
if (isset($_GET["historico"]) && isset($_GET["nombreDispositivo"])){
	$tipoSensor = $_GET["historico"];
	$nombreDispositivo = $_GET["nombreDispositivo"];
	if($tipoSensor == "temperatura"){
		$ficheroTemperatura = $direccionLogs . $nombreDispositivo . "_temperatura.txt";
		if(file_exists($ficheroTemperatura)){
			$file = fopen($ficheroTemperatura, "r");
			while(!feof($file)) {
			echo fgets($file). "<br />";
			}
			fclose($file);
		}
		else{
			echo "<h2> Error. El fichero" . $ficheroTemperatura . " no existe</h2>";
		}

	}
	else if($tipoSensor == "humedad"){
		$ficheroHumedad = $direccionLogs . $nombreDispositivo . "_humedad.txt";
		if(file_exists($ficheroHumedad)){
				$file = fopen($ficheroHumedad, "r");
				while(!feof($file)) {
				echo fgets($file). "<br />";
			}
			fclose($file);
		}
		else{
			echo "<h2> Error. El fichero" . $ficheroHumedad . " no existe</h2>";
		}
		
	}
}
else
	echo "<h2> Parámetros erroneos recibidos</h2>";
?>
<form action="./">
    <input type="submit" value="Vuelta a la pagina principal" />
</form>