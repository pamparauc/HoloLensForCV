<?php
global $finalArray;
global $arrayContrast, $arrayBrigthness, $faceArray, $edgeArray, $colorArray;
// contrast
$contrast = -1;
$arrayContrast=array();
$arrayBrigthness=array();
$faceArray=array();
$edgeArray=array();
$colorArray=array();
$finalArray=array();
if (!empty($_POST["contrast"]))
  $contrast= $_POST["contrast"];
if($contrast>-1)
{
	$arrayContrast = array("Contrast"=>$contrast);
	$finalArray = $arrayContrast;
}

// brigthness
$brigthness = -1;
if(!empty($_POST['brigthness']))
	$brigthness = $_POST["brigthness"];
if($brigthness>-1)
{
	$arrayBrigthness = array("Brigthness"=>$brigthness);
	$finalArray = array_merge($arrayContrast, $arrayBrigthness);
}

// face Detection
$faceDetection = $_POST["face"];
if($faceDetection == 1) // 1-Yes , 2-No  => see web.html file
{
	$faceArray = array("Face-detection"=>"true");
	$finalArray = array_merge($finalArray, $faceArray);
}

// edge Detection
$edgeDetection = $_POST["edge"];
// 1-No, 2-Highlight Edges
// 3-Highlight Background Over Edges, 4 - Color Background Highlight Edges => see web.html page
if($edgeDetection != 1) 
{
	switch($edgeDetection)
	{
		case 2:
			$edgeArray = array("Edge-enhancement" => "Highlight Edges");
			break;
		case 3:
			$edgeArray = array("Edge-enhancement" => "Highlight Background Over Edges");
			break;
		case 4:
			$edgeArray = array("Edge-enhancement" => "Color Background Highlight Edges");
			break;
	}
	$finalArray = array_merge($finalArray, $edgeArray);
}
if (!empty($_POST["html5colorpicker1"]) && !empty($_POST["html5colorpicker2"])
	&& $_POST["html5colorpicker1"] != $_POST["html5colorpicker2"])
{
	list($ri, $gi, $bi) = sscanf($_POST["html5colorpicker1"], "#%02x%02x%02x");
	list($rf, $gf, $bf) = sscanf($_POST["html5colorpicker2"], "#%02x%02x%02x");
	$colorArray = array(
		"From" => array("R"=>$ri, "G"=>$gi, "B"=>$bi),
		"To" => array("R"=>$rf, "G"=>$gf, "B"=>$bf)
	);
	$replace_color = array("Color-modification"=>$colorArray);
	$finalArray += $replace_color;
} 
$file="config.json";
if(!empty($finalArray))
{
	file_put_contents($file,json_encode($finalArray));
	header('Location: http://stud.usv.ro/~cpamparau/config.json');
}
else
{
	header('Location: http://stud.usv.ro/~cpamparau');
}
?>