<?php
    $auth = rmsAuth::Instance();
?>
<div class="header flex-container center-v" style="height: 60px; background-color: #f6f6f6">
    <div class="open-nav" style="width: 50px; text-align: center; color: #525252">&#9776</span></div>
    <div style="width: calc(100% - 120px); min-width: 180px">
        <form action="search.php" method="GET" style="width: 100%; margin: 0px auto; max-width: 650px">
            <div class="flex-container" style="border: 1px solid #525252">
                <div style="width: calc(100% - 80px)">
                    <input type="text" name="search_query" placeholder="Search" style="width: 100%; height: 35px; border: 0px; padding: 0px 0px 0px 10px; font-size: 15px; padding-left: 10px">
                </div>
                <div style="width: 80px">
                    <input type="submit" value="Search" style="width: 100%; height: 35px; border: 0px; color: white; background-color: #525252; font-size: 12px; font-weight: bold">
                </div>  
            </div>
        </form>
    </div>
    <?php
        if ($auth->isUserLoggedIn())
        {
            if ($_SESSION["userType"] == "student")
            {
                echo '<div><a href="php/logout.php"><h2 style="color:#690202; margin: 0px; width: 50px; text-align: center; font-size: 15px; font-weight: 400; padding: 0px 10px">LOGOUT</h2></a></div>';
            }
            else
            {
                echo '<div><a href="upload.php"><h2 style="color:#690202; margin: 0px; width: 50px; text-align: center; font-size: 15px; font-weight: 400; padding: 0px 10px">UPLOAD</h2></a></div>';
            }
        }
        else
        {
            echo '<div><a href="login.php"><h2 style="color:#690202; margin: 0px; width: 50px; text-align: center; font-size: 15px; font-weight: 400; padding: 0px 10px">LOGIN</h2></a></div>';
        }
    ?>
</div>