
"use strict";


function doAutoToc(autotocelemobj)
{
    var autotocelem = $(autotocelemobj);
    var container = $(autotocelem).parent();
    var autoul = $('<ul />').appendTo(autotocelem);

/*    var do_heading = function(hXelem, h_level, toc_parent_ul, max_level) {
        // add an <LI> item with the title wrapped in a <P> element.
	var li = $('<li />').appendTo(toc_parent_ul);
	li.addClass('auto-toc-item').addClass('auto-toc-item-h'+h_level);
	$('<p />').text($(hXelem).text()).appendTo(li);
        // add potential children.
	if (h_level < max_level) {
	    if (childrenh.length) {
		var thisul = $('<ul />').appendTo(li);
		childrenh.each(function() {
				   do_heading(this, h_level+1, thisul, max_level);
			       });
	    }
	}
        };*/

    var max_level = 3;
    var i;
    for (i = 1; i < 6; ++i) {
	if (autotocelem.is('.auto-toc-h'+i)) {
	    max_level = i;
	    break;
	}
    }
    //console.log('max_level='+max_level);

    function h_level(hXelem) {
        var htagname = $(hXelem).prop("tagName");
        if (htagname.length == 0 || htagname[0].toLowerCase() != 'h') {
            return -1;
        }
        return parseInt(htagname.substr(1));
    };

    var toc_builder = {};
    toc_builder.baseul = autoul;
    toc_builder.container = container;
    toc_builder.max_level = max_level;
    toc_builder.current_parent_li_level_N = 0;
    toc_builder.current_parent_li = null;
    toc_builder.current_parent_li_ul = toc_builder.baseul;

    // handle a header. Keep & update state in toc_builder.
    function handle_header(hXelem)
    {
        var this_N = h_level(hXelem);
        if (this_N > toc_builder.max_level) {
            return;
        }
        // first, adapt the level.
        //console.log(toc_builder.current_parent_li_ul);
        //console.log('this_N='+this_N+', toc_builder.current_parent_li_level_N='
        //            +toc_builder.current_parent_li_level_N);
        if (this_N > toc_builder.current_parent_li_level_N) {
            if (toc_builder.current_parent_li_ul === null) {
                toc_builder.current_parent_li_ul = $('<ul />').appendTo(toc_builder.current_parent_li);
            }
            while (this_N > toc_builder.current_parent_li_level_N+1) {
                toc_builder.current_parent_li = $('<li />').appendTo(toc_builder.current_parent_li_ul);
                toc_builder.current_parent_li_ul = $('<ul />').appendTo(toc_builder.current_parent_li);
                toc_builder.current_parent_li_level_N += 1;
            }
        } else {
            while (this_N <= toc_builder.current_parent_li_level_N) {
                // console.log('iter parent li:');
                // console.log(toc_builder.current_parent_li);

                toc_builder.current_parent_li = toc_builder.current_parent_li.parent().closest('li');
                toc_builder.current_parent_li_level_N -= 1;
            }
            // has to exist by construction:
            // console.log('parent li:');
            // console.log(toc_builder.current_parent_li);
            if (toc_builder.current_parent_li.length == 0) {
                toc_builder.current_parent_li = null;
                toc_builder.current_parent_li_ul = toc_builder.baseul;
            } else {
                toc_builder.current_parent_li_ul = $(toc_builder.current_parent_li).children('ul');
            }
            // console.log('found li/ul:');
            // console.log(toc_builder.current_parent_li_ul);
        }
        // now, current_parent_li_ul is not null and we have to insert ourselves there.
        // console.log(toc_builder.current_parent_li_ul);
        var li = $('<li />').appendTo(toc_builder.current_parent_li_ul);
        var p = $('<p />').appendTo(li);
        var hXelem_id = $(hXelem).attr('id');
        var a_attrs;
        if (hXelem_id) {
            a_attrs = { 'href': '#'+hXelem_id };
        } else {
            a_attrs = { 'class': 'disabled' };
        }
        $('<a />', a_attrs).text($(hXelem).text()).appendTo(p);
        toc_builder.current_parent_li = li;
        toc_builder.current_parent_li_ul = null;
        toc_builder.current_parent_li_level_N += 1;
        // console.log(li);
    };


    $('h1,h2,h3,h4,h5,h6', container).each(
        function() {
            handle_header(this);
	});
}

$(document).ready(function(){
		      $('.auto-toc', this).each(function(){ doAutoToc(this); });
		  });
