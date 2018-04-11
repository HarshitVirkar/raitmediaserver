<?php
    session_start();
    
    require('php/db.php');
    require('php/rmsAuth.php');
    require('php/video.php');

    $auth = rmsAuth::Instance();

    if (!$auth->isUserLoggedIn()) {
        header('Location: login.php');
        exit();
    }

    if ($_POST["uploadVideo"]) {
        $file_name = $_FILES['file']['name'];
        $file_type = $_FILES['file']['type'];
        $file_size = $_FILES['file']['size'];

        $allowed_extensions = array("webm", "mp4", "ogv", "mov");
        $file_size_max = 2147483648;
        $pattern = implode ($allowed_extensions, "|");

        if (!empty($file_name))
        {    //here is what I changed - as you can see above, I used implode for the array
            // and I am using it in the preg_match. You pro can do the same with file_type,
            // but I will leave that up to you
            if (preg_match("/({$pattern})$/i", $file_name) && $file_size < $file_size_max)
            {
                if (($file_type == "video/webm") || ($file_type == "video/mp4") || ($file_type == "video/ogv"))
                {
                    if ($_FILES['file']['error'] > 0)
                    {
                        echo json_encode(["status" => 0, "data" => "Unexpected error occured, please try again later."]);
                    } else {
                        if (file_exists("secure/".$file_name))
                        {
                            echo json_encode(["status" => 0, "data" => $file_name." already exists."]);
                        } else {
                            echo json_encode(["status" => 1]);
                            // move_uploaded_file($_FILES["file"]["tmp_name"], "secure/".$file_name);
                            // echo "Stored in: " . "secure/".$file_name;
                        }
                    }
                } else {
                    echo json_encode(["status" => 0, "data" => "Invalid video format."]);
                }
            }else{
                echo json_encode(["status" => 0, "data" => "Invalid file format or file too large"]);
            }
        } else {
            echo json_encode(["status" => 0, "data" => "Please select a video to upload."]);
        }
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
                <div class="content">
                    <?php
                        if ($_SESSION["userType"] == "student")
                        {
                            echo '<div style="width: 100%; margin: 25px auto; max-width: 500px; text-align: center">
                                    <h2>Not authorized to upload video</h2>
                                </div>';

                        }
                        else 
                        {
                    ?>
                    <div style="width: calc(100% - 20px); margin: 25px auto; max-width: 500px; padding: 0px 10px">
                            <form id="uploadForm" enctype="multipart/form-data" method="POST" style="width: 100%; margin: 0px">
                                <div style="margin-bottom: 10px">
                                    <input type="text" id="titleInput" name="videoTitle" placeholder="Add your video title" style="width: calc(100% - 12px); height: 33px; border: 1px solid #888888; padding: 0px; font-size: 15px; padding-left: 10px" required>
                                </div>
                                <div style="margin-bottom: 10px">
                                    <input type="text" id="descriptionInput" name="videoDescription" placeholder="Add your video description" style="width: calc(100% - 12px); height: 33px; border: 1px solid #888888; padding: 0px; font-size: 15px; padding-left: 10px" required>
                                </div>
                                <div style="width: 100px; margin-bottom: 10px">
                                    <input type="file" name="file" id="fileToUpload" value="Choose file" required>
                                </div> 
                                <div style="width: 80px; margin: 0px auto">
                                    <input type="submit" value="submit" name="uploadVideo" style="width: 100%; height: 35px; border: 0px; color: white; background-color: #78122c; font-size: 12px">
                                </div>  
                            </form>
                            <div id="uploadContainer" style="padding: 20px 0px">
                                <div id="progressBar" style="width: 0%; height: 20px; background-color: #690202; -webkit-transition: width 1s;transition: width 1s">
                                    
                                </div>
                                <div id="uploadProgress" style="text-align: center; font-size: 20px">
                                    <p></p>
                                </div>
                            </div>
                            <div id="error" style="padding: 20px 0px; text-align: center; color: red">
                                <p></p>
                            </div>
                        </div> 
                    </div>
                    <?php } ?>
            </div>
        </div>
    </body>
    <script>
        document.querySelector(".open-nav").addEventListener("click", function(){
            document.querySelector(".inner-container").classList.toggle("nav-visible");
        });

        function fileUploadStarted(event)
        {
            document.getElementById("uploadForm").style.display = "none";
            document.getElementById("uploadContainer").style.display = "block";
        }

        function fileUploadProgress(event)
        {
            var totalProgress = Math.round(event.loaded / event.total * 100);
            var progressBar = document.querySelector("#progressBar");
            var uploadProgress = document.querySelector("#uploadProgress p");
            progressBar.style.width = uploadProgress.innerHTML = totalProgress + "%";

        }

        function fileUploadCompleted(event)
        {
            document.getElementById("uploadForm").style.display = "block";
            document.getElementById("uploadContainer").style.display = "none";
        }

        var uploadVideoForm = document.querySelector("#uploadForm");
        uploadVideoForm.addEventListener("submit", function(event){
            event.preventDefault();

            var form = event.target;

            var xhttp = new XMLHttpRequest();
            xhttp.upload.addEventListener("loadstart", fileUploadStarted, false);  
            xhttp.upload.addEventListener("progress", fileUploadProgress);  
            xhttp.upload.addEventListener("load", fileUploadCompleted);
            xhttp.onreadystatechange = function() {
                if (this.readyState == 4) {
                    if (this.status == 200) {
                        console.log(this.responseText);
                        var response = JSON.parse(this.responseText)
                        if (response["status"] == 1) {
                            document.querySelector("#error p").innerHTML = response["data"];
                        } else {
                            document.querySelector("#error p").innerHTML = response["data"];
                        }
                    }
                }
            }

            if (form.method.toLowerCase() === "post") {
                var formData = new FormData(form);
                formData.append("uploadVideo", "true");
                xhttp.open("post", form.action, true);
                xhttp.send(formData);
            }
        }, false);

    </script> 
</html>     