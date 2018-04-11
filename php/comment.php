<?php
	require('./db.php');
	require('./user.php');

	echo json_encode((Video::Instance())->getCommentsForVideo($_GET["videoId"]));
?>