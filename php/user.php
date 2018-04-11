<?php 
	class User 
	{
		protected static $userInstance = NULL;

		final protected function __construct() {
		}

		final protected function __clone() {}

		public static function Instance() {
			if ( self::$userInstance === null )
			{
				self::$userInstance = new self();
			}
			return self::$userInstance;
		}

		public function likeVideo ( $userId, $videoId )
		{
			$conn = db::get();
			try
			{
				//echo 'userId : '.$userId.'\n';
				//echo 'videoId : '.$videoId.'\n';
				//begin transaction
				$conn->begin_transaction();
				//update likeordislike status
				$sql = "INSERT INTO userlikesdislikesmapping (uid, vid, likeStatus, dislikeStatus) VALUES ('$userId', '$videoId', 1, 0) ON DUPLICATE KEY UPDATE likeStatus = IF(uid = '$userId' AND vid = '$videoId', (likeStatus + 1)%2, likeStatus), dislikeStatus = IF(uid = '$userId' AND vid = '$videoId' AND likeStatus = 1 AND dislikeStatus = 1, (dislikeStatus + 1)%2, dislikeStatus)";
				if (!$conn->query($sql) || $conn->affected_rows == 0) {
					throw new Exception();
				}
				$conn->commit();
				return ["status" => 1];
			} catch (Exception $e) {
				$conn->rollback();
				return ["status" => 0];
			}
		}

		public function dislikeVideo ( $userId, $videoId )
		{
			$conn = db::get();
			try
			{
				//echo 'userId : '.$userId.'\n';
				//echo 'videoId : '.$videoId.'\n';
				//begin transaction
				$conn->begin_transaction();
				//update likeordislike status
				$sql = "INSERT INTO userlikesdislikesmapping (uid, vid, likeStatus, dislikeStatus) VALUES ('$userId', '$videoId', 0, 1) ON DUPLICATE KEY UPDATE dislikeStatus = IF(uid = '$userId' AND vid = '$videoId', (dislikeStatus + 1)%2, dislikeStatus), likeStatus = IF(uid = '$userId' AND vid = '$videoId' AND dislikeStatus = 1 AND likeStatus = 1, (likeStatus + 1)%2, likeStatus)";
				if (!$conn->query($sql) || $conn->affected_rows == 0) {
					throw new Exception();
				}
				$conn->commit();
				return ["status" => 1];
			} catch (Exception $e) {
				$conn->rollback();
				return ["status" => 0];
			}
		}

		public function viewVideo ( $videoId )
		{
			$conn = db::get();
			$sql = "UPDATE `videos` SET `views`=`views` + 1 WHERE vid='$videoId'";
			if ($conn->query($sql) === TRUE) {
			    echo "views updated successfully";
			} else {
			    echo "Error updating record: " . $conn->error;
			}
		}

		public function comment ( $videoId, $comment )
		{
			$conn = db::get();
			$uid = $_SESSION["uid"];
			$sql = "INSERT INTO `comments` (`uid`,`vid`,`comment`) VALUES ('$uid','$videoId','$comment')";
			if ($conn->query($sql) === TRUE) {
			    return ["status" => 1];
			} else {
			    return ["status" => 0];
			}
		}

	}

	class Faculty extends User
	{
		public function upload( $title, $description, $tags )
		{
			//implementation
		}
	}
?>