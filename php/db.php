<?php

	define('DBSERVERNAME','127.0.0.1');
	define('DBUSERNAME','root');
	define('DBPASSWORD','root');
	define('DBNAME','RAITMediaServer');

	class db {
		private static $dbInstance;

		public static function get() {
			if ( self::$dbInstance == NULL ) {
				self::$dbInstance = new mysqli(DBSERVERNAME, DBUSERNAME, DBPASSWORD, DBNAME, 8889);
				if ( self::$dbInstance->connect_errno ) {
					echo "Failed to connect";
					return;
				}
			}
			return self::$dbInstance;
		}
	}
?>