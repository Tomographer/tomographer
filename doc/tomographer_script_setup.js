
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

function setupSpanArrows(tag) {
    // see http://stackoverflow.com/a/16292287/1694896
    var fixspanarrow = function(obj) {
	var content = obj.text().trim();
	obj.toggleClass('arrowright', content.indexOf('\u25BA') != -1);
	obj.toggleClass('arrowdown', content.indexOf('\u25BC') != -1);
    };
    $("table.directory", tag)
    .each(function() {
	      $("span.arrow", tag)
	      .each(function(){
			fixspanarrow($(this));
			$(this).bind("DOMSubtreeModified", function() {
					 fixspanarrow($(this));
				     });
		    });
	  });
};

function fixNavTreeArrows(tag) {
    var fixarrows = function (tree) {
	$("img[src='arrowdown.png']", tree).attr('src', 'myarrowdown.png');
	$("img[src='arrowright.png']", tree).attr('src', 'myarrowright.png');
    };
    (function(fixeddivobject){
	 var observer = new MutationObserver(function(mutations) {
						 fixarrows(fixeddivobject);
					     });
	 observer.observe(fixeddivobject, {
			      childList: true,
			      attributes: true,
			      attributeFilter: ['src'],
			      subtree: true
			  });
     })($("div#nav-tree", tag)[0]);
};

function mystrtrim(str) {
    var rx_trim = /^[\n\s\uFEFF\xA0]+|[\n\s\uFEFF\xA0]+$/g;
    return str.replace(rx_trim, '');
}

function setupHomeLink(doc) {
    var projectname = $("#projectname", doc);
    $('<span id="innerprojectname"></span>').prependTo(projectname);
    projectname.contents().each(function(){
				    if (this.nodeType == 3) {
					// text node
					$(this).appendTo($("#innerprojectname", projectname));
				    }
				});
    var inner = $("#innerprojectname", projectname);
    inner.text(mystrtrim(inner.text()));
    inner.click(function(){location.href="index.html";});
};


$(document).ready(function() {
		      setupExternalLinks($(document));
		      setupAllDirectoryLinks($(document));
		      setupSpanArrows($(document));
		      fixNavTreeArrows($(document));
		      setupHomeLink($(document));
		 });



