<?php
	class Video 
	{
		private static $videoInstance = NULL;

		public static function Instance() {
			if ( self::$videoInstance == NULL ) {
				self::$videoInstance = new Video();
			}
			return self::$videoInstance;
		}

		public function getVideoData($userId, $videoId)
		{
			$conn = db::get();
			$like = -1;
			$dislike = -1;
			$sql = "SELECT likeStatus, dislikeStatus FROM userlikesdislikesmapping WHERE uid='$userId' AND vid='$videoId'";
			if ($result = $conn->query($sql)) {
				while ($row = $result->fetch_assoc()) {
					$like = $row['likeStatus'];
					$dislike = $row['dislikeStatus'];
			    }
			} else {
				return ["status" => 0];
			}

			$sql = "SELECT videos.resourcePath, videos.thumbnailPath, videos.resourcePath, videos.title, videos.description, videos.views, videos.likes, videos.dislikes, videos.vid, users.fname, users.lname FROM videos INNER JOIN users ON videos.uid=users.uid WHERE vid='$videoId'";
			if ($result = $conn->query($sql)) {
		    	$video = ["status" => 1];
		    	while ($row = $result->fetch_assoc()) {
		    		$row["like"] = $like;
		    		$row["dislike"] = $dislike;
		    		$video["message"] = $row;
			    }
			    return $video;
			} else {
			    return ["status" => 0];
			}
		}

		public function getRecentlyUploadedVideoData()
		{
			$conn = db::get();
			$sql = "SELECT users.fname, users.lname, videos.vid, videos.resourcePath, videos.title, videos.views FROM videos INNER JOIN users ON videos.uid=users.uid ORDER BY videos.vid DESC LIMIT 10";
			if ($result = $conn->query($sql)) {
		    	$videos = ["status" => 1];
		    	$data = array();
		    	while ($row = $result->fetch_assoc()) {
		    		array_push($data, $row);
			    }
			    $videos["message"] = $data;
			    return $videos;
			} else {
			    return ["status" => 0, "message" => "Error : " . $conn->error];
			}
		}

		public function getRelatedVideos($videoId)
		{
			$conn = db::get();
			$sql = "SELECT users.fname, users.lname, videos.vid, videos.resourcePath, videos.title, videos.views FROM videos INNER JOIN users ON users.uid=videos.uid WHERE vid IN (SELECT videotagsmapping.vid FROM videotagsmapping WHERE tid IN (SELECT videotagsmapping.tid FROM videotagsmapping WHERE vid='$videoId'))";
			if ($result = $conn->query($sql)) {
		    	$videos = ["status" => 1];
		    	$data = array();
		    	while ($row = $result->fetch_assoc()) {
		    		array_push($data, $row);
			    }
			    $videos["message"] = $data;
			    return $videos;
			} else {
			    return ["status" => 0, "message" => "Error : " . $conn->error];
			}
		}

		public function getCommentsForVideo($videoId)
		{
			$conn = db::get();
			$sql = "SELECT comments.comment, users.fname, users.lname FROM comments INNER JOIN users ON comments.uid=users.uid WHERE comments.vid='$videoId' ORDER BY comments.cid DESC LIMIT 20";
			if ($result = $conn->query($sql)) {
		    	$comments = ["status" => 1];
		    	$data = array();
		    	while ($row = $result->fetch_assoc()) {
		    		array_push($data, $row);
			    }
			    $comments["message"] = $data;
			    return $comments;
			} else {
			    return ["isCommentsReceived" => false, "message" => "Error : " . $conn->error];
			}
		}

		public function getVideosForQuery($keyword)
		{
			$conn = db::get();
			$sql = "SELECT videos.vid, videos.title, videos.views, users.fname, users.lname FROM videos INNER JOIN users ON videos.uid=users.uid WHERE vid IN (SELECT videoTagsMapping.vid FROM videoTagsMapping INNER JOIN tags ON tags.tid=videoTagsMapping.tid WHERE tag='$keyword')";
			if ($result = $conn->query($sql)) {
		    	$videos = ["isVideosReceived" => true];
		    	$data = array();
		    	while ($row = $result->fetch_assoc()) {
		    		array_push($data, $row);
			    }
			    $videos["message"] = $data;
			    return $videos;
			} else {
			    return ["isVideosReceived" => false, "message" => "Error : " . $conn->error];
			}
		}

		public function getWatchPageTemplate($videoInfo)
		{

			$likes = $videoInfo["likes"];
			$dislikes = $videoInfo["dislikes"];
			$likeStatus = $videoInfo["like"];
			$dislikeStatus = $videoInfo["dislike"];

			if ($likeStatus == 1) {
				$likeBtnTemplate = '<div id="likes" style="color: white; width: 50%; max-width: 140px; border: 1px solid white; line-height: 38px; text-align: center; margin-right: 3px; background-color: white" data-like="'.$likeStatus.'">
										<p>
											<span id="likeCount" style="color: black">'.$likes.'</span>
											<span style="color: #888888; margin-bottom: 5px">Likes</span>
										</p>
								</div>';
			} else {
				$likeBtnTemplate = '<div id="likes" style="color: white; width: 50%; max-width: 140px; border: 1px solid white; line-height: 38px; text-align: center; margin-right: 3px" data-like="'.$likeStatus.'">
										<p>
											<span id="likeCount">'.$likes.'</span>
											<span style="color: #888888; margin-bottom: 5px">Likes</span>
										</p>
								</div>';
			}

			if ($dislikeStatus == 1) {
				$dislikeBtnTemplate = '<div id="dislikes" style="color: white; width: 50%; max-width: 140px; border: 1px solid white; line-height: 38px; text-align: center; margin-left: 3px; background-color: white" data-dislike="'.$dislikeStatus.'">
									<p>
										<span id="dislikeCount" style="color: black">'.$dislikes.'</span>
										<span style="color: #888888; margin-bottom: 5px">Dislikes</span>
									</p>
								</div>';
			} else {
				$dislikeBtnTemplate = '<div id="dislikes" style="color: white; width: 50%; max-width: 140px; border: 1px solid white; line-height: 38px; text-align: center; margin-left: 3px" data-dislike="'.$dislikeStatus.'">
									<p>
										<span id="dislikeCount">'.$dislikes.'</span>
										<span style="color: #888888; margin-bottom: 5px">Dislikes</span>
									</p>
								</div>';
			}

			return '<div style="background-color: #191919">
					 	<div class="player-container" style="padding-top: 25px" data-resourcePath="'.$videoInfo["resourcePath"].'">
							<div class="player" style="width: 640px; height: 360px; margin: 0px auto; background-color: #424244">
							<video id="video"
						           width="640"
						           height="360"
						           poster="//shaka-player-demo.appspot.com/assets/poster.jpg"
						           controls autoplay>
						        </video>
							</div>
						</div>
						<div class="video" id="'.$videoInfo["vid"].'" style="padding: 25px 15px; max-width: 640px; margin: 0px auto">
							<div id="title" style="color: white">
								<h2 style="margin-top: 0px">'.$videoInfo["title"].'</h2>
							</div>
							<div id="views" style="color: #888888">
								<p>'.$videoInfo["views"].' views </p>
							</div>
							<div class="flex-container" style="margin-top: 20px">
							'.$likeBtnTemplate.$dislikeBtnTemplate.'
							</div>
							<div class="info" style="margin-top: 20px">
								<div id="uploader" style="color: white; width: 60%">
									<div id="name">
										<p>
											<span style="color:#888888; margin-bottom: 5px">Published by</span>
												<br>
												'.$videoInfo["fname"].' '.$videoInfo["lname"].'
										</p>
									</div>
								</div>
								<div id="description" style="margin-top: 10px; color: #888888">
									<p>'.$videoInfo["description"].'</p>
								</div>
							</div>
						</div>
					</div>
					<div style="max-width: 645px; margin: 0px auto; padding: 10px 15px 25px 15px">
						<div>
							<h2>Related Videos</h2>
							<hr>
							<div class="videos flex-container">
							</div
						</div>
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
						</div>
					</div>';
		}
	}
?>