$(function() {
	/*$('.submit').on('click', function() {
		$(this).prop('disabled','disabled');
		$('.submit svg, .submit span').toggleClass('disable');
	})*/

	$('#login').submit(function() {
		$('.submit').prop('disabled','disabled');
		$('.submit svg, .submit span').toggleClass('disable');
	});	
});