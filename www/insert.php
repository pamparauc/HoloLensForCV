<?php
global $finalArray;
global $arrayContrast, $arrayBrigthness, $faceArray, $edgeArray, $colorArray;
// contrast
$contrast = -1;
if (!empty($_POST["contrast"]))
  $contrast= $_POST["contrast"];
if($contrast>-1)
{
	$arrayContrast = array("modify-contrast"=>$contrast);
}

// brigthness
$brigthness = -1;
if(!empty($_POST['brigthness']))
	$brigthness = $_POST["brigthness"];
if($brigthness>-1)
{
	$arrayBrigthness = array("modify-brigthness"=>$brigthness);
	$finalArray = array_merge($arrayContrast, $arrayBrigthness);
}

// face Detection
$faceDetection = $_POST["face"];
if($faceDetection == "Yes")
{
	$faceArray += ["apply-face-detection" => "true"];
	$finalArray = array_merge($finalArray, $faceArray);
}
print_r(json_encode($finalArray)); 
$edgeDetection = $_POST["edge"];
if($edgeDetection == "Yes"){
	$edgeArray += ["apply-edge-detection" => "true"];
	$finalArray += $faceArray;
}

if (!empty($_POST["init_R"]) && !empty($_POST["initG"]) && !empty($_POST["initB"]) &&
	!empty($_POST["finalR"]) && !empty($_POST["finalG"]) && !empty($_POST["finalB"]))
{
	$initR = $_POST["init_R"];
	$initG = $_POST["initG"];
	$initB = $_POST["initB"];
	$finalR = $_POST["finalR"];
	$finalG = $_POST["finalG"];
	$finalB = $_POST["finalB"];
	$colorArray = array(
		"initial" => array("R"=>$init_R, "G"=>$init_G, "B"=>$init_B),
		"final" => array("R"=>$final_R, "G"=>$final_G, "B"=>$final_B)
	);
	$finalArray += $colorArray;
} 

?>