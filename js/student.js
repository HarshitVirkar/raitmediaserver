var app = {
	/*images: [
		{
			id:"fe_orientation", 
			url:"../img/events/fe_orientation.jpg",
			category: "event"
		},
		{
			id: "fresher", 
			url: "../img/events/freshers.jpg",
			category: "event"
		},
		{
			id: "ganesh_festival", 
			url:"../img/events/ganesh_festival.jpg",
			category: "event"
		},
		{
			id: "t-shirt_launch", 
			url:"../img/events/t-shirt_launch.jpg",
			category: "event"
		},
		{
			id: "teachers_day",
			url: "../img/events/teachers_day.jpg",
			category: "event"
		},
		{
			id: "horizon", 
			url: "../img/events/horizon.jpg",
			category: "event"
		},
		{
			id: "tie_saree", 
			url: "../img/events/tie_saree.jpg",
			category: "event"
		},
		{
			id: "traditional", 
			url: "../img/events/traditional.jpg",
			category: "event"
		},
		{
			id: "be_farewell", 
			url: "../img/events/be_farewell.jpg",
			category: "event"
		},
		{
			id: "vp",
			url: "../img/committee/vijay_patil.jpg",
			category: "committee"
		},
		{
			id: "princi",
			url: "../img/committee/ramesh_vasappanavara.jpg",
			category: "committee"
		},
		{
			id: "vice-princi",
			url: "../img/committee/mukesh_patil.jpg",
			category: "committee"
		},
		{
			id: "gs",
			url: "../img/committee/jayaraj.jpg",
			category: "committee"
		},
		{
			id: "js",
			url: "../img/committee/suraj.jpg",
			category: "committee"
		},
		{
			id: "cs",
			url: "../img/committee/sushant.jpg",
			category: "committee"
		},
		{
			id: "treas",
			url: "../img/committee/yash.jpg",
			category: "committee"
		},
		{
			id: "cfc",
			url: "../img/committee/abhijit.jpg",
			category: "committee"
		},
		{
			id: "sh",
			url: "../img/committee/rajit.jpg",
			category: "committee"
		},
		{
			id: "mh",
			url: "../img/committee/payal.jpg",
			category: "committee"
		},
		{
			id: "pro",
			url: "../img/committee/rahul.jpg",
			category: "committee"
		},
		{
			id: "th",
			url: "../img/committee/prathamesh.jpg",
			category: "committee"
		},
		{
			id: "payal_img",
			url: "../img/committee/payal_1.jpg",
			category: "contact"
		}
	]
}

app.load_images = function()
{
	for (var i = 0; i < app.images.length; i++)
	{
		var img = new Image();
		img.onload = (function(index, img)
		{
			return function()
			{
				var img_category = app.images[index]["category"];
				switch( img_category )
				{
					case "event":
						app.set_event_image(index, img);
						break;
					case "committee":
						app.set_committee_image(index, img);
						break;	
					case "contact":
						app.set_contact_image(index, img);
						break;
				}		
			}

		})(i, img);
		img.src = app.images[i]["url"];
	}
}

app.set_event_image = function(index, img)
{
	var id = app.images[index]["id"];
	var img_parent = document.querySelector("#"+id+" .event_image");
	img_parent.appendChild(img);
	img_parent.classList.add("img_gradient","fade_in");

}

app.set_committee_image = function(index, img)
{
	var id = app.images[index]["id"];
	document.querySelector("#"+id+" .member_img").appendChild(img);
	var committee_img = document.querySelector("#"+id+" .member_img img");
	committee_img.classList.add("img_grayscale","fade_in");
	document.querySelector("#"+id+" .member_info").classList.add("bg_fade");
}

app.set_contact_image = function(index, img)
{
	var id = app.images[index]["id"];
	document.querySelector("#"+id).appendChild(img);
	var contact_img = document.querySelector("#"+id+" img");
	contact_img.style.height = "150px";
	contact_img.style.width = "150px";
	contact_img.style.borderRadius = "75px";
	contact_img.classList.add("img_grayscale","fade_in");
}*/
}

app.getRecentlyUploadedVideos = function()
{
	var xmlhttp = new XMLHttpRequest();
	xmlhttp.onreadystatechange = function() {
	    if (this.readyState == 4 && this.status == 200) {
	        console.log(this.responseText);
	    }
	}
	xmlhttp.open("GET", "php/recentlyUploaded.php", true);
	xmlhttp.send();
}

app.getVideoData = function(videoId)
{
	var xmlhttp = new XMLHttpRequest();
	xmlhttp.onreadystatechange = function() {
	    if (this.readyState == 4 && this.status == 200) {
	        console.log(this.responseText);
	    }
	}
	xmlhttp.open("GET", "php/getVideo.php?videoId=" + videoId, true);
	xmlhttp.send();
}

app.getComments = function(videoId)
{
	var xmlhttp = new XMLHttpRequest();
	xmlhttp.onreadystatechange = function() {
	    if (this.readyState == 4 && this.status == 200) {
	        console.log(this.responseText);
	    }
	}
	xmlhttp.open("GET", "php/getComments.php?videoId=" + videoId, true);
	xmlhttp.send();
}

app.getRecentlyUploadedVideos();
app.getVideoData(1);
app.getComments(1);