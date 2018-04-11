<?php
	require('./rmsAuth.php');

	(rmsAuth::Instance())->logout();
	header('Location: ../index.php');
?>