<?php
	session_start();
	
	require('php/db.php');
	require('php/rmsAuth.php');
	require('php/video.php');	

	if (isset($_GET["v"]))
	{
		$videoStatus = (Video::Instance())->getVideoData($_SESSION["uid"], $_GET["v"]);
		if (isset($_GET["sca"]) && $_GET["sca"] == "1")
		{
			echo json_encode($videoStatus);
			exit();
		}
		else 
		{
			if ($videoStatus["status"] == 1)
			{
				$view = (Video::Instance())->getWatchPageTemplate($videoStatus["message"]);
			}
			else
			{
				echo "Error ! please try again";
				exit();
			}			
		}
	}
	else
	{
		echo "No video found";
		exit();
	}
?>
<!DOCTYPE html>
<html lang="en">
    <head>
    	<title>RAIT Media Server</title>
		<?php require('php/includes/metadata.php'); ?>
		<!-- Shaka Player compiled library: -->
		    <script src="dist/shaka-player.compiled.js"></script>
		    <!-- Your application source: -->
		    <script src="myapp.js"></script>
		<style>
			hr {
				border-top: 1px solid #fafafa;
			}	
		</style>
    </head>
    <body data-load-comments="true">
    	<div class="outer-container">
			<?php require('php/includes/navigation.php'); ?>
            <div class="inner-container">
				<?php require('php/includes/header.php'); ?>
                <div class="content">
                	<?php echo $view; ?>
                </div>
            </div>
        </div>
    </body> 
	<script>
		app = {};

		app.getRelatedVideos = function(videoId) {
            var xmlhttp = new XMLHttpRequest();
            xmlhttp.onreadystatechange = function() {
                if (this.readyState == 4 && this.status == 200) {
                    var relatedVideos = JSON.parse(this.responseText);
                    console.log(relatedVideos);
                    if (relatedVideos.message.length == 0)
                    {
                        document.getElementsByClassName("videos")[0].innerHTML = "<p>No related videos</p>";
                    }
                    else
                    {
                        (relatedVideos.message).forEach(function(videoInfo){
                            var video = `<div class="video" style="margin-right: 5px" data-vid="`+videoInfo["vid"]+`" onclick="app.watchVideo(event)">
                                            <div class="video-thumbnail" style="width: 210px; height: 118px; background-color: #dedede">
                                            </div>
                                            <div class="video-info" style="width: 186px; height: 84px; padding-right: 24px;">
                                                <p style="margin: 8px 0px ; font-weight: 400">`+videoInfo["title"]+`<p>
                                                <p style="color: #888888">`+videoInfo["fname"]+` `+videoInfo["lname"]+`<p>
                                                <p style="color: #888888">`+videoInfo["views"]+` views<p>
                                            </div>
                                        </div>`;

                            document.getElementsByClassName("videos")[0].innerHTML += video;
    
                        });
                    }
                }
            }
            xmlhttp.open("GET", "php/getRelatedVideos.php?videoId="+videoId, true);
            xmlhttp.send();
		}

        app.watchVideo = function(event) {
            window.location.href="/RAITMediaServer/watch.php?v=" + event.currentTarget.getAttribute("data-vid");
        }

		app.getComments = function(videoId)
		{
			var xmlhttp = new XMLHttpRequest();
			xmlhttp.onreadystatechange = function() {
			    if (this.readyState == 4 && this.status == 200) {
			    	var commentStatus = JSON.parse(this.responseText);
			    	(commentStatus.message).forEach(function(commentInfo){
			    		var commentBlock = `<div class="comment" style="font-size: 15px; 				margin-top: 15px">
										<p style="font-weight: 400; margin-bottom: 5px"> 
											${commentInfo.fname} ${commentInfo.lname}
										</p>
										<p>
											${commentInfo.comment}
										</p>
									</div>`
						var commentsContainer = document.getElementsByClassName("comments");
				        commentsContainer[0].innerHTML = commentsContainer[0].innerHTML + commentBlock;
			    	});
			        
			    }
			}
			xmlhttp.open("GET", "php/getComments.php?videoId=" + videoId, true);
			xmlhttp.send();
		}

		app.likeVideo = function(videoId)
		{
			var xmlhttp = new XMLHttpRequest();
			xmlhttp.onreadystatechange = function() {
			    if (this.readyState == 4 && this.status == 200) {
			    	var response = JSON.parse(this.responseText);
			    	if ( response["status"] == 3 )
			    	{
			    		window.location.href = "/RAITMediaServer/login.php";
			    	}
			    	else if ( response["status"] == 1 )
			    	{
			    		var likeStatus = document.getElementById("likes").getAttribute('data-like');
			    		if ( likeStatus == 1 ) {
			    			app.changeLikeCount(0, -1);
			    		} else {
			    			app.changeLikeCount(1, +1);
			    			var dislikeStatus = document.getElementById("dislikes").getAttribute('data-dislike');
			    			if ( dislikeStatus == 1 ) {
			    				app.changeDisLikeCount(0, -1);
			    			}
			    		}
			    	}
			    }
			}
			xmlhttp.open("GET", "php/likeVideo.php?vid=" + videoId, true);
			xmlhttp.send();
		}

		app.dislikeVideo = function(videoId)
		{
			var xmlhttp = new XMLHttpRequest();
			xmlhttp.onreadystatechange = function() {
			    if (this.readyState == 4 && this.status == 200) {
			    	var response = JSON.parse(this.responseText);
			    	if ( response["status"] == 3 )
			    	{
			    		window.location.href = "/RAITMediaServer/login.php";
			    	} 
			    	else if ( response["status"] == 1 )
			    	{
			    		var dislikeStatus = document.getElementById("dislikes").getAttribute('data-dislike');
			    		if ( dislikeStatus == 1 ) {
			    			app.changeDisLikeCount(0, -1);
			    		} else {
			    			app.changeDisLikeCount(1, +1);
			    			var likeStatus = document.getElementById("likes").getAttribute('data-like');
			    			if ( likeStatus == 1 ) {
			    				app.changeLikeCount(0, -1);
			    			}
			    		}
			    	}
			    }
			}
			xmlhttp.open("GET", "php/dislikeVideo.php?vid=" + videoId, true);
			xmlhttp.send();
		}

		app.changeLikeCount = function(status, changeBy)
		{
			document.getElementById("likes").setAttribute('data-like', status);
			var count = document.getElementById("likeCount");
    		count.innerHTML = (parseInt(count.innerHTML) + (changeBy)).toString();
    		if ( status == 1 ){
	    		count.style.color = "black";
	    		var likes = document.getElementById("likes");
	    		likes.style.backgroundColor = "white";
    		} else {
				count.style.color = "white";
	    		var likes = document.getElementById("likes");
	    		likes.style.backgroundColor = "rgba(255, 255, 255, 0)";
    		}
		}

		app.changeDisLikeCount = function(status, changeBy)
		{
			document.getElementById("dislikes").setAttribute('data-dislike', status);
			var count = document.getElementById("dislikeCount");
    		count.innerHTML = (parseInt(count.innerHTML) + (changeBy)).toString();
    		if ( status == 1 ){
	    		count.style.color = "black";
	    		var dislikes = document.getElementById("dislikes");
	    		dislikes.style.backgroundColor = "white";
    		} else {
				count.style.color = "white";
	    		var dislikes = document.getElementById("dislikes");
	    		dislikes.style.backgroundColor = "rgba(255, 255, 255, 0)";
    		}
		}

		var form = document.getElementById("commentForm");
		form.addEventListener("submit", function(event) {
			event.preventDefault();
			var url = form.getAttribute("action");

			var xmlhttp = new XMLHttpRequest();
			xmlhttp.onreadystatechange = function() {
			    if (this.readyState == 4 && this.status == 200) {
			    	var response = JSON.parse(this.responseText);
			    	if ( response["status"] == 3 )
			    	{
			    		window.location.href = "/RAITMediaServer/login.php"
			    	}
			    	else if ( response["status"] == 1 )
			    	{
			    		document.getElementById("commentInput").value = "";
				        var commentBlock = document.getElementsByClassName("comments");
				        commentBlock[0].innerHTML = "";
				        app.getComments(document.getElementsByClassName("video")[0].id);
			    	}
			    }
			}
			xmlhttp.open("POST", "php/commentOnVideo.php?", true);
			xmlhttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
			var comment = document.getElementById("commentInput").value;
			var vid = document.getElementsByClassName("video")[0].id;
			xmlhttp.send("vid="+vid+"&comment="+comment);
		});

		document.querySelector(".open-nav").addEventListener("click", function(){
		    document.querySelector(".inner-container").classList.toggle("nav-visible");
		});

		document.getElementById("likes").addEventListener("click", function(){
			app.likeVideo(app.videoId);
		});

		document.getElementById("dislikes").addEventListener("click", function(){
			app.dislikeVideo(app.videoId);
		});

		document.addEventListener("DOMContentLoaded", function(event) {
			console.log(document.getElementsByClassName("player-container")[0].getAttribute("data-resourcePath"))
			initApp(document.getElementsByClassName("player-container")[0].getAttribute("data-resourcePath"))
			app.videoId = document.getElementsByClassName("video")[0].id;
            app.getComments(app.videoId);
            app.getRelatedVideos(app.videoId);
        });
	</script>
</html>      