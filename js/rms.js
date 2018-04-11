app = {};

app.getRecentlyUploadedVideos = function()
{
    var xmlhttp = new XMLHttpRequest();
    xmlhttp.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
            var recentUploads = JSON.parse(this.responseText);
            history.replaceState(recentUploads, 'RAIT Media Server', 'http://localhost:8888/RAITMediaServer/index.php');
            if (recentUploads.message.length == 0)
            {
                document.getElementsByClassName("videos")[0].innerHTML = "<p>No videos uploaded</p>";
            }
            else
            {
                (recentUploads.message).forEach(function(videoInfo){
                    var div = document.createElement("div");
                    div.className = "video";
                    div.style.marginRight = "5px";
                    div.setAttribute("data-vid", videoInfo["vid"]);
                    div.innerHTML = `
                        <div class="video-thumbnail" style="width: 210px; height: 118px; background-color: #dedede">
                        </div>
                        <div class="video-info" style="width: 186px; height: 84px; padding-right: 24px;">
                            <p style="margin: 8px 0px ; font-weight: 400">`+videoInfo["title"]+`<p>
                            <p style="color: #888888">`+videoInfo["fname"]+` `+videoInfo["lname"]+`<p>
                            <p style="color: #888888">`+videoInfo["views"]+` views<p>
                        </div>`;

                    document.getElementsByClassName("videos")[0].appendChild(div);

                    div.addEventListener("click", function(e) {
                        var vid = e.currentTarget.getAttribute("data-vid"); 
                        history.pushState({id:"watchPage"}, 'RAIT Media Server', 'http://localhost:8888/RAITMediaServer/watch.php?v='+ vid);
                        app.displayWatchPage(vid);
                        /*window.location.href="http://localhost:8888/RaITMediaServer/watch.php?v=" + vid;*/        
                    });    
                });
            }
        }
    }
    xmlhttp.open("GET", "php/recentlyUploaded.php", true);
    xmlhttp.send();
}

app.getVideoData = function(videoId)
{
    var xmlhttp = new XMLHttpRequest();
    xmlhttp.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
            console.log(this.responseText);
        }
    }
    xmlhttp.open("GET", "php/getVideo.php?videoId=" + videoId, true);
    xmlhttp.send();
}

window.onpopstate = function (event) {
    if(history.state)
    {
        console.log(history.state);
    }
};

app.getComments = function(videoId)
{
	var xmlhttp = new XMLHttpRequest();
	xmlhttp.onreadystatechange = function() {
	    if (this.readyState == 4 && this.status == 200) {
	    	console.log(this.responseText);
	    	var commentStatus = JSON.parse(this.responseText);
	    	(commentStatus.message).forEach(function(commentInfo){
	    		var commentView = commentInfo["template"];
		        /*var div = document.createElement("div");
		        div.className = "comment";
		        div.style.fontSize = "15px";
		        div.style.marginTop = "15px";
		        div.innerHTML = '\
					<p style="font-weight: 400; margin-bottom: 5px">' + commentInfo["fname"] +  ' ' + commentInfo["lname"] + '</p>\
					<p>' + commentInfo["comment"] + '</p>';*/

		        var commentBlock = document.getElementsByClassName("comments");
		        commentBlock[0].innerHTML = commentBlock[0].innerHTML + commentView;
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
	    		window.location.href = "http://localhost:8888/RaITMediaServer/login.php";
	    	}
	    	else if ( response["status"] == 1 )
	    	{
	    		var count = document.getElementById("likeCount");
	    		count.innerHTML = (parseInt(count.innerHTML) + 1).toString();
	    		count.style.color = "black";
	    		var likes = document.getElementById("likes");
	    		likes.style.border = "none";
	    		likes.style.backgroundColor = "#ffffff";
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
	    		window.location.href = "http://localhost:8888/RaITMediaServer/login.php";
	    	} 
	    	else if ( response["status"] == 1 )
	    	{
	    		var count = document.getElementById("dislikeCount");
	    		count.style.color = "black";
	    		count.innerHTML = (parseInt(count.innerHTML) + 1).toString();
	    		var dislikes = document.getElementById("dislikes");
	    		dislikes.style.border = "none";
	    		dislikes.style.backgroundColor = "#ffffff";
	    	}
	    }
	}
	xmlhttp.open("GET", "php/dislikeVideo.php?vid=" + videoId, true);
	xmlhttp.send();
}

app.displayWatchPage = function(vid)
{
	var xmlhttp = new XMLHttpRequest();
	xmlhttp.onreadystatechange = function() {
	    if (this.readyState == 4 && this.status == 200) {
	    	console.log(this.responseText);
	    	var response = JSON.parse(this.responseText);
	    	if ( response["status"] == 1 )
	    	{
	    		var videoInfo = response["message"],
					vid = videoInfo["vid"],
					title = videoInfo["title"],
					views = videoInfo["views"],
					likes = videoInfo["likes"],
					dislikes = videoInfo["dislikes"],
					description = videoInfo["description"],
					fname = videoInfo["fname"],
					lname = videoInfo["lname"];
				document.getElementsByClassName("content")[0].innerHTML = `<div style="background-color: #191919">
					 	<div class="player-container" style="padding-top: 25px">
							<div class="player" style="width: 640px; height: 360px; margin: 0px auto; background-color: #424244">
							</div>
						</div>
						<div class="video" id=${videoInfo.vid} style="padding: 25px 15px; max-width: 640px; margin: 0px auto">
							<div id="title" style="color: white">
								<h2 style="margin-top: 0px">` + title + `</h2>
							</div>
							<div id="views" style="color: #888888">
								<p> ${videoInfo.views} views </p>
							</div>
							<div class="flex-container" style="margin-top: 20px">
								<div id="likes" style="color: white; width: 50%; max-width: 140px; border: 1px solid white; line-height: 38px; text-align: center; margin-right: 3px">
										<p>
											<span id="likeCount">${videoInfo.likes}</span>
											<span style="color: #888888; margin-bottom: 5px">Likes</span>
										</p>
								</div>
								<div id="dislikes" style="color: white; width: 50%; max-width: 140px; border: 1px solid white; line-height: 38px; text-align: center; margin-left: 3px">
									<p>
										<span id="dislikeCount">${videoInfo.dislikes}</span>
										<span style="color: #888888; margin-bottom: 5px">Dislikes</span>
									</p>
								</div>
							</div>
							<div class="info" style="margin-top: 20px">
								<div id="uploader" style="color: white; width: 60%">
									<div id="name">
										<p>
											<span style="color:#888888; margin-bottom: 5px">Published by</span>
												<br>
												${videoInfo.fname} ${videoInfo.lname}
										</p>
									</div>
								</div>
								<div id="description" style="margin-top: 10px; color: #888888">
									<p>${videoInfo.description}</p>
								</div>
							</div>
						</div>
					</div>
					<div style="max-width: 640px; ; margin: 0px auto; padding: 10px 15px 25px 15px">
					<div style="width: 100%; margin: 25px 0px">
						<form id="commentForm" action="commentOnVideo.php" method="POST" style="width: 100%; margin: 0px">
							<div class="flex-container">
							    <div style="width: calc(100% - 80px)">
									<input type="text" id="commentInput" placeholder="Add your comment" style="width: 100%; height: 33px; border: 1px solid #888888; padding: 0px; font-size: 15px; padding-left: 10px">
					    		</div>
					    		<div style="width: 80px">
					    			<input type="submit" value="Comment" style="width: 100%; height: 35px; border: 0px; color: white; background-color: #78122c; font-size: 12px">
					    		</div>	
							</div>

						</form>
					</div>
					<div>
						<h2>Comments</h2>
						<hr>
						<div class="comments"></div>
					</div>`;
	    	}
	    }
	}
	xmlhttp.open("GET", "watch.php?v=" + vid + "&sca=1", true);
	xmlhttp.send();
}

/*var form = document.getElementById("commentForm");
form.addEventListener("submit", function(event) {
	event.preventDefault();
	var url = form.getAttribute("action");

	var xmlhttp = new XMLHttpRequest();
	xmlhttp.onreadystatechange = function() {
	    if (this.readyState == 4 && this.status == 200) {
	    	var response = JSON.parse(this.responseText);
	    	if ( response["status"] == 3 )
	    	{
	    		window.location.href = "http://localhost:8888/RaITMediaServer/login.php"
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
});*/

document.querySelector(".open-nav").addEventListener("click", function(){
    document.querySelector(".inner-container").classList.toggle("nav-visible");
});

/*document.getElementById("likes").addEventListener("click", function(){
	app.likeVideo(document.getElementsByClassName("video")[0].id);
});

document.getElementById("dislikes").addEventListener("click", function(){
	app.dislikeVideo(document.getElementsByClassName("video")[0].id);
});*/

document.addEventListener("DOMContentLoaded", function () {
	console.log("DOM loaded");
	app.getRecentlyUploadedVideos();
}, false);