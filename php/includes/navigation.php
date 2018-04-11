<div class="navigation">
    <div>
        <a style="font-weight: 400; color: #525252" href="index.php"><div class="section">Home</div></a> 
        <?php
	        $auth = rmsAuth::Instance();

	        if ($auth->isUserLoggedIn())
	        {
	        	if ($_SESSION["userType"] == "faculty")
	        	{
	        		echo '<a style="font-weight: 400; color: #525252" href="upload.php"><div class="section">Upload</div></a> ';
	        	}
	        	echo '<a style="font-weight: 400; color: #525252" href="php/logout.php"><div class="section">Logout</div></a> ';
	        }
	        else
	        {
	            echo '<a style="font-weight: 400; color: #525252" href="login.php"><div class="section">Sign In</div></a> ';
	        }
	    ?>                   
    </div>
</div>