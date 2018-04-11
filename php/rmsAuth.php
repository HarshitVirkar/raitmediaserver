<?php

	class rmsAuth
	{
		private static $authInstance = NULL;

		public static function Instance() {
			if ( self::$authInstance == NULL ) {
				self::$authInstance = new rmsAuth();
			}
			return self::$authInstance;
		}

		public function login($uid, $password)
		{
			$conn = db::get();

			$uid = $conn->real_escape_string($uid);
			$password = $conn->real_escape_string($password);

			if (empty($uid) || empty($password)) {
				return ["isUserLoggedIn" => false, "message" => "userId/password field cannot be empty"];
			} else {
				$sql = "SELECT * FROM users WHERE uid = '$uid'";

				if ($result = $conn->query($sql)) {
				    $row_count = $result->num_rows;
				    if ( $row_count == 0 ) {
				    	return ["isUserLoggedIn" => false, "message" => "User doesnt't exist"];
				    } else {
				    	while ($row = $result->fetch_assoc()) {
					        $hashedPwdCheck = password_verify($password, $row["password"]);
					        if ( $hashedPwdCheck == false ) {
					        	return ["isUserLoggedIn" => false, "message" => "Incorrect password"];
					        } elseif ( $hashedPwdCheck == true ) {
					        	$this->setSessionVaribles($uid, $row["userType"]);
					        	return ["isUserLoggedIn" => true, "message" => NULL];
					        }
					    }
				    }
				} else {
				    return ["isUserLoggedIn" => false, "message" => "Error : " . $conn->error];
				}
			}
		}

		private function setSessionVaribles($uid, $utype)
		{
			$_SESSION["loggedIn"] = true;
			$_SESSION["uid"] = $uid;
			$_SESSION["userType"] = $utype;
		}

		public function isUserLoggedIn()
		{
			if(isset($_SESSION['loggedIn']) && $_SESSION['loggedIn'] == true){
				return true;
			}
			return false;
		}

		public function logout()
		{
			session_start();
			session_unset();
			session_destroy();
		}
	}

?>