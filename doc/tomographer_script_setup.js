
/* javascript setup */

function setupExternalLinks(tag) {
    var abs_url_regexp = new RegExp('^[A-Za-z0-9_]+://');
    $("a", tag)
	.each(function() {
		  if (abs_url_regexp.test($(this).attr('href'))) {
		      $(this).attr("target", "_blank");
		  }
	      });
};

function setupAllDirectoryLinks(tag) {
    $("table.directory td.entry", tag)
	.each(function() {
		  $("a", tag)
		      .each(function(){
				$(this).attr("title", $(this).text())
			    });
	      });
};


$(document).ready(function() {
		      setupExternalLinks($(document));
		      setupAllDirectoryLinks($(document));
		 });



