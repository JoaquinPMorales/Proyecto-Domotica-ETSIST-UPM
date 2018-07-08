<?php
session_start();
session_destroy();
/* BLOQUE DE EJECUCIÓN DE OPERACIONES */
/*En este bloque mediante código php se captura la acción pedida por el usuario mediante los enlaces disponibles para cada dispositivo, y se hace la peticion HTTP al dispositivo  */
if (isset($_GET["tipoDispositivo"]) && isset($_GET["accion"]) && isset($_GET["ip"])  && isset($_GET["nombreDispositivo"])){
	$tipoDispositivo =  $_GET["tipoDispositivo"];
	$tipoAccion = $_GET["accion"];
	$ip = $_GET["ip"];
	$nombreDispositivo = $_GET["nombreDispositivo"];
	$ch = curl_init();
	//defimos la url a la que hacemos la peticion
	curl_setopt($ch, CURLOPT_URL, "http://" . $ip ."/operaciones");
	//definimos el tipo de peticion: POST
	curl_setopt($ch, CURLOPT_POST, TRUE);
	switch($tipoDispositivo){
		case "actuadorIluminacion":
			switch($tipoAccion){
				case "encender":
					//definimos los parametros de la peticion http
					curl_setopt($ch, CURLOPT_POSTFIELDS, "iluminacion=encender");
					break;
				case "apagar":
					//definimos los parametros de la peticion http
					curl_setopt($ch, CURLOPT_POSTFIELDS, "iluminacion=apagar");
					break;
				}
			break;
		case "actuadorEnchufe":
			switch($tipoAccion){
				//Peticion http al dispositivo para conectar el enchufe
				case "conectar":
					//definimos los parametros de la peticion http
					curl_setopt($ch, CURLOPT_POSTFIELDS, "enchufe=conectar");
					break;
				//Peticion http al dispositivo para desconectar el enchufe
				case "desconectar":
					curl_setopt($ch, CURLOPT_POSTFIELDS, "enchufe=desconectar");
					break;
				}
			break;
		case "actuadorPersiana":
			switch($tipoAccion){
				//Peticion http al dispositivo para subir la persiana
				case "subir":
					//definimos los parametros de la peticion http
					curl_setopt($ch, CURLOPT_POSTFIELDS, "persiana=subir");
					break;
				//Peticion http al dispositivo para bajar la persiana
				case "bajar":
					//definimos los parametros de la peticion http
					curl_setopt($ch, CURLOPT_POSTFIELDS, "persiana=bajar");
					break;
				//Peticion http al dispositivo para parar la persiana
				case "parar":
					//definimos los parametros de la peticion http
					curl_setopt($ch, CURLOPT_POSTFIELDS, "persiana=parar");
					break;
				}
				break;
	}
	curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
	$respuestaHttp = curl_exec($ch);
	curl_close($ch);
}

?>
<html>
<head>
<meta http-equiv='Content-Type' content='text/html; charset=utf-8'/>
<title>Casa Domotica</title>
<script src="https://code.jquery.com/jquery-2.1.1.min.js" type="text/javascript"></script>
<?php 
/* BLOQUE GENERACION FILAS DE LA TABLA */
/* Este bloque genera una nueva fila de la tabla, de manera que puede añadirse un nuevo dispositivo a la interfaz simplemente añadiendo la ip del dispositivo, su nombre y su tipo*/
function mostrarDispositivo($tipoDispositivo, $nombreDispositivo, $ipDispositivo){
	switch($tipoDispositivo){
		case "actuadorIluminacion":
			echo "<tr>";
			echo "<td class='tg-ov9q'>Actuador Iluminacion</td>";
			echo "<td class='tg-ov9q'><a href='./mostrarLogDispositivo.php?nombreDispositivo=" . $nombreDispositivo ."'>" . $nombreDispositivo . "</a></td>";
			echo "<td class='tg-ov9q'><div id='" . $nombreDispositivo . "'></div></td>";
			echo "<td class='tg-ov9q'> <a href='./index.php?tipoDispositivo=" . $tipoDispositivo ."&accion=encender&ip=" . $ipDispositivo ."&nombreDispositivo=" . $nombreDispositivo . "'>Encender </a> | <a href='./index.php?tipoDispositivo=". $tipoDispositivo ."&accion=apagar&ip=" . $ipDispositivo ."&nombreDispositivo=" . $nombreDispositivo . "'>Apagar </a></td>";
			echo "</tr>";
			break;
		case "actuadorEnchufe":
			echo "<tr>";
			echo "<td class='tg-us36'>Actuador Enchufe</td>";
			echo "<td class='tg-us36'><a href='./mostrarLogDispositivo.php?nombreDispositivo=" . $nombreDispositivo ."'>" . $nombreDispositivo ."</a></td>";
			echo "<td class='tg-us36'><div id='" . $nombreDispositivo . "'></div></td>";
			echo "<td class='tg-us36'><a href='./index.php?tipoDispositivo=" . $tipoDispositivo . "&accion=conectar&ip=" . $ipDispositivo ."&nombreDispositivo=" .  $nombreDispositivo . "'>Conectar </a> | <a href='./index.php?tipoDispositivo=" . $tipoDispositivo . "&accion=desconectar&ip=" . $ipDispositivo ."&nombreDispositivo=" . $nombreDispositivo . "'>Desconectar </a></td>";
			echo "</tr>";
			break;
		case "actuadorPersiana":
			echo "<tr>";
			echo "<td class='tg-ov9q'>Actuador Persiana</td>";
			echo "<td class='tg-ov9q'><a href='./mostrarLogDispositivo.php?nombreDispositivo=" . $nombreDispositivo ."'>" . $nombreDispositivo . "</a></td>";
			echo "<td class='tg-ov9q'></td>";
			echo "<td class='tg-ov9q'><a href='./index.php?tipoDispositivo=" . $tipoDispositivo . "&accion=subir&ip=" . $ipDispositivo ."&nombreDispositivo=" .  $nombreDispositivo . "'>Subir </a> | <a href='./index.php?tipoDispositivo=" . $tipoDispositivo . "&accion=bajar&ip=" . $ipDispositivo ."&nombreDispositivo=" .  $nombreDispositivo . "'>Bajar </a>  |  <a href='./index.php?tipoDispositivo=" . $tipoDispositivo . "&accion=parar&ip=" . $ipDispositivo ."&nombreDispositivo=" . $nombreDispositivo . "'>Parar </a></td>";
			echo "</tr>";
			break;
		case "sensorTemperatura":
			echo "<tr>";
			echo "<td class='tg-us36'>Sensor Temperatura</td>";
			echo "<td class='tg-us36'><a href='./mostrarLogDispositivo.php?nombreDispositivo=" . $nombreDispositivo ."'>" . $nombreDispositivo . "</a></td>";
			echo "<td class='tg-us36'><div id='" . $nombreDispositivo . "'></div></td>";
			echo "<td class='tg-us36'><a href='./mostrarHistorico.php?historico=temperatura&nombreDispositivo=" . $nombreDispositivo . "'>Histórico </a></td>";
			echo "</tr>";
			break;
		case "sensorHumedad":
			echo "<tr>";
			echo "<td class='tg-ov9q'>Sensor Humedad</td>";
			echo "<td class='tg-ov9q'><a href='./mostrarLogDispositivo.php?nombreDispositivo=" . $nombreDispositivo . "'>" . $nombreDispositivo . " </a></td>";
			echo "<td class='tg-ov9q'><div id='". $nombreDispositivo ."'></div></td>";
			echo "<td class='tg-ov9q'><a href='./mostrarHistorico.php?historico=humedad&nombreDispositivo=" . $nombreDispositivo . "'>Histórico </a></td>";
			echo "</tr>";
			break;
	}
	refrescarDispositivo($tipoDispositivo, $nombreDispositivo);
		
}
/* BLOQUE REFRESCAR DISPOSITIVO */
/* En este bloque se genera un código javascript en función del tipo de dispositivo, capaz de refrescar su estado periodicamente*/
function refrescarDispositivo($tipoDispositivo, $nombreDispositivo){
	switch($tipoDispositivo){
		case "actuadorIluminacion":
			?>
			<script>
					$(document).ready(function(){
					setInterval(function(){
					$("#<?php echo $nombreDispositivo;?>").load('obtenerEstadoIluminacion.php?nombreActuadorIluminacion=<?php echo $nombreDispositivo;?>')
					}, 2000);
					});
			</script>
			<?php
			break;
		case "actuadorEnchufe":
			?>
			<script>
				$(document).ready(function(){
				setInterval(function(){
				$("#<?php echo $nombreDispositivo;?>").load('obtenerEstadoEnchufe.php?nombreActuadorEnchufe=<?php echo $nombreDispositivo;?>')
				}, 2000);
				});
			</script>
			<?php
			break;
		case "sensorTemperatura":
			?>
			<script>
				$(document).ready(function(){
				setInterval(function(){
				$("#<?php echo $nombreDispositivo;?>").load('obtenerTemperatura.php?nombreDispositivo=<?php echo $nombreDispositivo;?>')
				}, 3000);
				});
			</script>
			<?php
			break;
		case "sensorHumedad":
			?>
			<script>
				$(document).ready(function(){
				setInterval(function(){
				$("#<?php echo $nombreDispositivo;?>").load('obtenerHumedad.php?nombreDispositivo=<?php echo $nombreDispositivo;?>')
				}, 3000);
				});
			</script>
			<?php
			break;
		
	}
}
?>
<style type="text/css">
.tg  {border-collapse:collapse;border-spacing:0;border-color:#999;}
.tg td{font-family:Arial, sans-serif;font-size:14px;padding:10px 5px;border-style:solid;border-width:0px;overflow:hidden;word-break:normal;border-top-width:1px;border-bottom-width:1px;border-color:#999;color:#444;background-color:#F7FDFA;}
.tg th{font-family:Arial, sans-serif;font-size:14px;font-weight:normal;padding:10px 5px;border-style:solid;border-width:0px;overflow:hidden;word-break:normal;border-top-width:1px;border-bottom-width:1px;border-color:#999;color:#fff;background-color:#26ADE4;}
.tg .tg-c3ow{border-color:inherit;text-align:center;vertical-align:top}
.tg .tg-us36{border-color:inherit;vertical-align:top}
.tg .tg-ov9q{background-color:#D2E4FC;border-color:inherit;vertical-align:top}
@media screen and (max-width: 767px) {.tg {width: auto !important;}.tg col {width: auto !important;}.tg-wrap {overflow-x: auto;-webkit-overflow-scrolling: touch;}}</style>
</head>
<body>
<h1>Bienvenidos al sistema domótico</h1>
<div class="tg-wrap"><table class="tg">
<!-- Primera fila -->
  <tr>
    <th class="tg-c3ow">Tipo</th>
    <th class="tg-us36">Nombre</th>
    <th class="tg-us36">Estado</th>
    <th class="tg-us36">Acciones</th>
  </tr>
  <?php
  //Añada aqui sus nuevos dispositivos
  //mostrarDispositivo(tipoDispositivo, nombreDispositivo, ipDispositivo)
	mostrarDispositivo("actuadorIluminacion", "Iluminacion_Entrada", "192.168.1.175");
	mostrarDispositivo("actuadorIluminacion", "Iluminacion_Salon", "192.168.1.176");
	mostrarDispositivo("actuadorEnchufe", "Enchufe_Cafetera", "192.168.1.177");
	mostrarDispositivo("actuadorEnchufe", "Enchufe_Television", "192.168.1.178");
	mostrarDispositivo("actuadorPersiana", "Persiana_Salon", "192.168.1.179");
	mostrarDispositivo("actuadorPersiana", "Persiana_HabitacionPrincipal", "192.168.1.180");
	mostrarDispositivo("sensorTemperatura", "Temperatura_Salon" , "192.168.1.181");
	mostrarDispositivo("sensorTemperatura", "Temperatura_HabitacionBebe" , "192.168.1.182");
	mostrarDispositivo("sensorHumedad", "Humedad_Jardin" , "192.168.1.183");
	mostrarDispositivo("sensorHumedad", "Humedad_HabitacionBebe" , "192.168.1.184");
  ?>
</table></div>

<form method="post" action="./mostrarLogDispositivo.php?nombreDispositivo=controlador">
    <button type="submit"> Log del controlador</button>
</form>
</body>
</html>