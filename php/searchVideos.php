<?php
	require('./db.php');
	require('./video.php');

	echo json_encode((Video::Instance())->getVideosForQuery($_GET["search_query"]));
?>