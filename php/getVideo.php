<?php
	require('./db.php');
	require('./video.php');

	echo json_encode((Video::Instance())->getVideoData($_GET["videoId"]));
?>