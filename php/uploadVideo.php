<?php
	if (isset($_POST['submit']))
	{
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
	                    echo "Unexpected error occured, please try again later.";
	                } else {
	                    if (file_exists("secure/".$file_name))
	                    {
						 echo $file_name." already exists.";
	                    } else {
	                        move_uploaded_file($_FILES["file"]["tmp_name"], "secure/".$file_name);
	                        echo "Stored in: " . "secure/".$file_name;
	                    }
	                }
	            } else {
	                echo "Invalid video format.";
	            }
	        }else{
	            echo "where is my mojo?? grrr";
	        }
	    } else {
	        echo "Please select a video to upload.";
	    }
	}
?>