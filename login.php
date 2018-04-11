<?php
	session_start();
	require('php/db.php');
	require('php/rmsAuth.php');

	$auth = rmsAuth::Instance();
	
	if ($auth->isUserLoggedIn())
	{
		header('Location: index.php');
		exit();
	}

	if (isset($_POST['submit'])) {
		$authStatus = $auth->login($_POST["uid"], $_POST["password"]);
		if ($authStatus["isUserLoggedIn"] == true) {
			header('Location: index.php');
			exit();
		} else {
			$error = $authStatus['message'];
			echo '<script type="text/javascript">alert("'.$error.'");</script>';
		}
	}
?>
<!DOCTYPE html>
<html style="height: 100%">
	<head>
		<?php require('php/includes/head.php'); ?>
		<link rel="stylesheet" href="css/index.css">	
	</head>
	<body style="height: 100%" class="animated fadeIn">
		<div class="jumbotron" style="height: 100%">
			<div class="container">			
				<div class="login_form">
					<div class="form_bg">
						<div class="title">
							login
						</div>
						<form id="login" action="" method="POST">
							<div class="form-group">
							    <input type="text" class="form-control" id="username" name="uid" placeholder="Enter Roll No">
							</div>
							<div class="form-group">
							    <input type="password" class="form-control" id="password" name="password" placeholder="Enter Password"> 
							</div>
							<div style="text-align: center">
							    <button type="submit" class="form-control" name="submit" class="submit"> 
									<span>login</span>
									<svg class="disable" version="1.1" id="loader-1" xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" x="0px" y="0px"
									width="23px" height="23px" viewBox="0 0 40 40" enable-background="new 0 0 40 40" xml:space="preserve">
										<path opacity="0.4" fill="#000" d="M20.201,5.169c-8.254,0-14.946,6.692-14.946,14.946c0,8.255,6.692,14.946,14.946,14.946
										s14.946-6.691,14.946-14.946C35.146,11.861,28.455,5.169,20.201,5.169z M20.201,31.749c-6.425,0-11.634-5.208-11.634-11.634
										c0-6.425,5.209-11.634,11.634-11.634c6.425,0,11.633,5.209,11.633,11.634C31.834,26.541,26.626,31.749,20.201,31.749z"/>
										<path fill="#000" d="M26.013,10.047l1.654-2.866c-2.198-1.272-4.743-2.012-7.466-2.012h0v3.312h0
										C22.32,8.481,24.301,9.057,26.013,10.047z">
											<animateTransform attributeType="xml"
											  	attributeName="transform"
											    type="rotate"
											    from="0 20 20"
											    to="360 20 20"
											    dur="0.4s"
											    repeatCount="indefinite"/>
										</path>
									</svg>								
							    </button>
							</div>
							<!--<div style="text-align: center">	
								<button type="submit" name="submit" class="submit">
									<span>login</span>
									<svg class="disable" version="1.1" id="loader-1" xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" x="0px" y="0px"
									width="23px" height="23px" viewBox="0 0 40 40" enable-background="new 0 0 40 40" xml:space="preserve">
										<path opacity="0.4" fill="#000" d="M20.201,5.169c-8.254,0-14.946,6.692-14.946,14.946c0,8.255,6.692,14.946,14.946,14.946
										s14.946-6.691,14.946-14.946C35.146,11.861,28.455,5.169,20.201,5.169z M20.201,31.749c-6.425,0-11.634-5.208-11.634-11.634
										c0-6.425,5.209-11.634,11.634-11.634c6.425,0,11.633,5.209,11.633,11.634C31.834,26.541,26.626,31.749,20.201,31.749z"/>
										<path fill="#000" d="M26.013,10.047l1.654-2.866c-2.198-1.272-4.743-2.012-7.466-2.012h0v3.312h0
										C22.32,8.481,24.301,9.057,26.013,10.047z">
											<animateTransform attributeType="xml"
											  	attributeName="transform"
											    type="rotate"
											    from="0 20 20"
											    to="360 20 20"
											    dur="0.4s"
											    repeatCount="indefinite"/>
										</path>
									</svg>								
								</button>
							</div>-->												
						</form>					
					</div>
				</div>		
			</div>
		</div>
		<?php require('php/includes/scripts.php'); ?>
		<script src="js/index.js"></script>
	</body>	
</html>
