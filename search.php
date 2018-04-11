<?php
    session_start();
    
	require('php/db.php');
    require('php/rmsAuth.php');
	require('php/video.php');

	if (isset($_GET["search_query"]))
	{
        $videoStatus = (Video::Instance())->getVideosForQuery($_GET["search_query"]);
        if ($videoStatus["isVideosReceived"] == false)
        {
            echo $videoStatus["message"];
            exit();
        }
	}
	else
	{
		echo "No result found";
		exit();
	}
?>
<!DOCTYPE html>
<html lang="en">
    <head>
        <title>RAIT Media Server</title>
        <?php require('php/includes/metadata.php'); ?>
    </head>
    <body>
        <div class="outer-container">
            <?php require('php/includes/navigation.php'); ?>
            <div class="inner-container">
                <?php require('php/includes/header.php'); ?>
                <div class="content" style="margin-left: 10%; padding: 40px 0px">  
                    <h2>Search results for <?php echo '<span style="font-weight: 400"><i>'.$_GET["search_query"].'</i></span>'?></h2>
                    <div class="videos flex-container" style="margin-top: 10px; max-width: 1075px">
                        <?php
                            if (count($videoStatus["message"]) == 0)
                            {
                                echo '<p>No results found</p>';  
                            }
                            else
                            {
                                foreach ($videoStatus["message"] as &$videoInfo) {
                                    $vid = $videoInfo["vid"];
                                    $title = $videoInfo["title"];
                                    $fname = $videoInfo["fname"];
                                    $lname = $videoInfo["lname"];
                                    $views = $videoInfo["views"];
                                    echo '<div class="video" style="margin-right: 5px; margin-bottom: 10px" data-vid="'.$vid.'" onclick="app.watchVideo(event)">
                                            <div class="video-thumbnail" style="width: 210px; height: 118px; background-color: #dedede">
                                            </div>
                                            <div class="video-info" style="width: 186px; height: 100px; padding-right: 24px;">
                                                <p style="margin: 8px 0px ; font-weight: 400; overflow: hidden; text-overflow:clip; max-height: 40px">'.$title.'<p>
                                                <p style="color: #888888">'.$fname.' '.$lname.'<p>
                                                <p style="color: #888888">'.$views.' views<p>
                                            </div>
                                        </div>';  
                                }
                            }
                        ?>
                    </div>
                </div>
            </div>
        </div>
    </body> 
    <script>
        document.querySelector(".open-nav").addEventListener("click", function(){
            document.querySelector(".inner-container").classList.toggle("nav-visible");
        });

        app = {}

        app.watchVideo = function(event) {
            window.location.href="/RAITMediaServer/watch.php?v=" + event.currentTarget.getAttribute("data-vid");
        }

    </script> 
</html>       