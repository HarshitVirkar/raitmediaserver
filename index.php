<?php
    session_start();
    
    require('php/db.php');
    require('php/rmsAuth.php');
    require('php/video.php');
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
                    <div style="margin-left: 10%; max-width: 1075px; padding: 20px 0px">
                        <div class="recently-uploaded">
                            <h2>Recently uploaded</h2>
                            <div class="videos flex-container" style="margin-top: 24px">
                            </div>
                        </div>
                        <div class="recently-viewed">
                            <h2>Recently viewed</h2>
                            <div class="videos flex-container" style="margin-top: 24px">
                                <p>Yet to be implemented</p>
                            </div>
                        </div>
                    </div>  
                </div>
            </div>
        </div>
    </body>
    <script>
        app = {};

        app.getRecentlyUploadedVideos = function()
        {
            var xmlhttp = new XMLHttpRequest();
            xmlhttp.onreadystatechange = function() {
                if (this.readyState == 4 && this.status == 200) {
                    var recentUploads = JSON.parse(this.responseText);
                    if (recentUploads.status == 1) {
                        if (recentUploads.message.length == 0) {
                            document.getElementsByClassName("videos")[0].innerHTML = "<p>No videos uploaded</p>";
                        } else {
                            (recentUploads.message).forEach(function(videoInfo){
                                var video = `<div class="video" style="margin-right: 5px; margin-bottom: 10px" data-vid="`+videoInfo["vid"]+`" onclick="app.watchVideo(event)">
                                                <div class="video-thumbnail" style="width: 210px; height: 118px; background-color: #dedede">
                                                </div>
                                                <div class="video-info" style="width: 186px; height: 100px; padding-right: 24px;">
                                                    <p style="margin: 8px 0px; font-weight: 400; overflow: hidden; text-overflow:clip; max-height: 40px">`+videoInfo["title"]+`<p>
                                                    <p style="color: #888888">`+videoInfo["fname"]+` `+videoInfo["lname"]+`<p>
                                                    <p style="color: #888888">`+videoInfo["views"]+` views<p>
                                                </div>
                                            </div>`;

                                document.getElementsByClassName("videos")[0].innerHTML += video;
        
                            });
                        }
                    } else {
                        document.getElementsByClassName("videos")[0].innerHTML = "<p>Error getting videos</p>";
                    }
                }
            }
            xmlhttp.open("GET", "php/recentlyUploaded.php", true);
            xmlhttp.send();
        }

        app.watchVideo = function(event) {
            window.location.href="/RAITMediaServer/watch.php?v=" + event.currentTarget.getAttribute("data-vid");
        }

        document.querySelector(".open-nav").addEventListener("click", function(){
            document.querySelector(".inner-container").classList.toggle("nav-visible");
        });

        document.addEventListener("DOMContentLoaded", function(event) {
            app.getRecentlyUploadedVideos();
        });
    </script> 
</html>     