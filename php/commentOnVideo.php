<?php
	session_start();
	
	require('./db.php');
	require('./rmsAuth.php');
	require('./user.php');

	$auth = rmsAuth::Instance();

	if (!$auth->isUserLoggedIn())
	{
		echo json_encode(["status"=>3]);
		exit();
	}

	echo json_encode((User::Instance())->comment($_POST["vid"], $_POST["comment"]));
?>