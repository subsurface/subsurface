var itemsToShow = new Array();	//list of indexes to all dives to view
var items = new Array();
var start;			//index of first element viewed in itemsToShow
var sizeofpage;			//size of viewed page
var numberofwords = 0;		//just for stats
var olditemstoshow;		//to reference the indexes to all dives if changed
//////////////////////////////////
//				//
//		View Model	//
//				//
//////////////////////////////////

/**
*This Method view all items
*View N pages each of sizeofpage size items.
*starting from zero
*/
function showAllDives()
{
	for (var i = 0; i < items.length; i++) {
		itemsToShow.push(i);
	}
	olditemstoshow = itemsToShow;
	start = 0;
	viewInPage();
}

/**
*This function view the 'itemstoshow' in pages.
*It start from 'start' variable.
*It showes N pages each of sizeofpage size.
*/
function viewInPage()
{
	var end = start + sizeofpage - 1;
	if (end >= itemsToShow.length)
		end = itemsToShow.length - 1;
	updateView(start, end);
}

/**
*addHTML this Method puts the HTML of items of given indexes
*@param {array} indexes array of indexes to put in HTML
*/
function updateView(start, end)
{
	var divelist = document.getElementById('diveslist');
	divelist.innerHTML = "";
	for (var i = start; i <= end; i++) {
		divelist.innerHTML += '<ul id="' + itemsToShow[i] + '" onclick="toggleExpantion(this)"</ul>';
		expand(document.getElementById(itemsToShow[i]));
		items[itemsToShow[i]].expanded = true;
	};
	view_pagging(start, end);
}

/**
*addHTML this Method puts the HTML of items of given indexes
*@param {array} indexes array of indexes to put in HTML
*/
function addHTML(indexes)
{
	var divelist = document.getElementById('diveslist');
	divelist.innerHTML = "";
	for (var i = 0; i < indexes.length; i++) {
		divelist.innerHTML += '<ul id="' + indexes[i] + '" onclick="toggleExpantion(this)"</ul>';
		expand(document.getElementById(indexes[i]));
		itemsToShow[indexes[i]].expanded = true;
	};
}

/**
*This Method shows items in a range [start,end]
*@param {integer} start start from this index
*@param {integer} finish at this index.
*/
function view_in_range(start, end)
{
	var ind = new Array();
	if (end >= itemsToShow.length)
		end = itemsToShow.length - 1;
	for (var i = start; i <= end; i++) {
		ind.push(i);
	}
	addHTML(ind);
	view_pagging(start, end);
}

/**
*Show the previous page, Will do nothing if no previous pages
*/
function prev_page()
{
	var end = start + sizeofpage - 1;
	if (start - sizeofpage > 0) {
		start -= sizeofpage;
	} else {
		start = 0;
	}
	if (end - sizeofpage > 0) {
		end -= sizeofpage;
	}
	if (end >= itemsToShow.length) {
		end = itemsToShow.length - 1;
	}
	updateView(start, end)
}

/**
*Show the next page, Will do nothing if no next pages
*/
function next_page()
{
	var end = start + sizeofpage - 1;
	if (end + sizeofpage < itemsToShow.length) {
		end += sizeofpage;
	} else {
		end = itemsToShow.length - 1;
	}
	if (start + sizeofpage < itemsToShow.length) {
		start += sizeofpage;
	}
	updateView(start, end)
}

function view_pagging(start, end)
{
	var page = document.getElementById("pagging");
	page.innerHTML = (start + 1) + ' to ' + (end + 1) + ' of ' + (itemsToShow.length) + ' dives';
}

/**
*Expand all dives in the view.
*/
function expandAll()
{
	for (var i = start; i < start + sizeofpage; i++) {
		if (i >= itemsToShow.length)
			break;
		unexpand(document.getElementById(itemsToShow[i]));
		items[itemsToShow[i]].expanded = false;
	}
}

/**
*Collapse all dives in the view.
*/
function collapseAll()
{
	for (var i = start; i < start + sizeofpage; i++) {
		if (i >= itemsToShow.length)
			break;
		expand(document.getElementById(itemsToShow[i]));
		items[itemsToShow[i]].expanded = true;
	}
}

function setNumberOfDives(e)
{
	var value = e.options[e.selectedIndex].value;
	sizeofpage = parseInt(value, 10);
	var end = start + sizeofpage - 1;
	view_in_range(start, end);
}

function toggleExpantion(ul)
{
	if (!items[ul.id].expanded) {
		expand(ul);
		items[ul.id].expanded = true;
	} else {
		unexpand(ul);
		items[ul.id].expanded = false;
	}
}

function expand(ul)
{
	ul.innerHTML = getlimited(items[ul.id]);
	ul.style.padding = '2px 10px 2px 10px';
}

function unexpand(ul)
{
	ul.innerHTML = getExpanded(items[ul.id]);
	ul.style.padding = '3px 10px 3px 10px';
}

///////////////////////////////////////
//
//			Dive Model
//
//////////////////////////////////////

function getlimited(dive)
{
	return '<div style="height:20px"><div class="item">' + (settings.subsurfaceNumbers === '0' ? dive.number + 1 : dive.subsurface_number) + '</div>' +
	       '<div class="item">' + dive.date + '</div>' +
	       '<div class="item">' + dive.time + '</div>' +
	       '<div class="item_large">' + dive.location + '</div>' +
	       '<div class="item">' + dive.temperature.air + '</div>' +
	       '<div class="item">' + dive.temperature.water + '</div></div>';
};

function getExpanded(dive)
{
	var res = '<table><tr><td class="words">Date: </td><td>' + dive.date +
		  '</td><td class="words">&nbsp&nbsp&nbsp&nbsp&nbspTime: </td><td>' + dive.time +
		  '</td><td class="words">&nbsp&nbsp&nbsp&nbsp&nbspLocation: </td><td>' + '<a onclick=\"Search_list_Modules(\'' + dive.location + '\')\">' +
		  dive.location + '</a>' +
		  '</td></tr></table><table><tr><td class="words">Rating:</td><td>' + putRating(dive.rating) +
		  '</td><td class="words">&nbsp&nbsp&nbspVisibilty:</td><td>' + putRating(dive.visibility) +
		  '</td></tr></table>' +
		  '<table><tr><td class="words">Air temp: </td><td>' + dive.temperature.air +
		  '</td><td class="words">&nbsp&nbsp&nbsp&nbspWater temp: </td><td>' + dive.temperature.water +
		  '</td></tr></table><table><tr><td class="words">DiveMaster: </td><td>' + dive.divemaster +
		  '</td></tr><tr><td class="words"><p>Buddy: </p></td><td>' + dive.buddy +
		  '</td></tr><tr><td class="words">Suit: </td><td>' + dive.suit +
		  '</td></tr><tr><td class="words">Tags: </td><td>' + putTags(dive.tags) +
		  '</td></tr></table><div style="margin:10px;"><p class="words">Notes: </p>' + dive.notes + '</div>';
	if (settings.listOnly === '0') {
		res += '<center><a onclick="showDiveDetails(' + dive.number + ')">show more details</a></center>';
	}
	return res;
};

function putTags(tags)
{
	var result = "";
	for (var i in tags) {
		result += '<a onclick=\"Search_list_Modules(\'' + tags[i] + '\')\">' + tags[i] + '</a>';
		if (i < tags.length - 1)
			result += ', ';
	}
	return result;
}

/**
*@param {integer} rate out of 5
*return HTML string of stars
*/
function putRating(rating)
{
	var result;
	result = '<div>';
	for (var i = 0; i < rating; i++)
		result += ' &#9733; ';
	for (var i = rating; i < 5; i++)
		result += ' &#9734; ';
	result += '</div>';
	return result;
}

///////////////////////////////////////
//
//		Sorting
//
/////////////////////////////////////

/*
this variables keep the state of
each col. sorted asc or des
*/
var number = false;
var time = true;
var date = true;
var air = true;
var water = true;
var locat = true;

function list_sort(sortOn)
{
	switch (sortOn) {
	case '1': //number
		if (number) {
			sort_it(sortOn, cmpNumAsc);
			number = 1 - number;
		} else {
			sort_it(sortOn, cmpNumDes);
			number = 1 - number;
		}
		break;
	case '2': //date
		if (date) {
			sort_it(sortOn, cmpDateAsc);
			date = 1 - date;
		} else {
			sort_it(sortOn, cmpDateDes);
			date = 1 - date;
		}
		break;
	case '3': //time
		if (time) {
			sort_it(sortOn, cmpTimeDes);
			time = 1 - time;
		} else {
			sort_it(sortOn, cmpTimeAsc);
			time = 1 - time;
		}
		break;
	case '4': //Air temp
		if (air) {
			sort_it(sortOn, cmpAtempDes);
			air = 1 - air;
		} else {
			sort_it(sortOn, cmpAtempAsc);
			air = 1 - air;
		}
		break;
	case '5': //Water temp
		if (water) {
			sort_it(sortOn, cmpWtempDes);
			water = 1 - water;
		} else {
			sort_it(sortOn, cmpWtempAsc);
			water = 1 - water;
		}
		break;
	case '6': //Water temp
		if (locat) {
			sort_it(sortOn, cmpLocationDes);
			locat = 1 - locat;
		} else {
			sort_it(sortOn, cmpLocationAsc);
			locat = 1 - locat;
		}
		break;
	}
}

/*
*sorting interface for different coloumns
*/

function cmpLocationAsc(j, iSmaller)
{
	return items[j].location < items[iSmaller].location;
}

function cmpLocationDes(j, iSmaller)
{
	return items[j].location > items[iSmaller].location;
}

function cmpNumAsc(j, iSmaller)
{
	return items[j].subsurface_number < items[iSmaller].subsurface_number;
}

function cmpNumDes(j, iSmaller)
{
	return items[j].subsurface_number > items[iSmaller].subsurface_number;
}

function cmpTimeAsc(j, iSmaller)
{
	return items[j].time < items[iSmaller].time;
}

function cmpTimeDes(j, iSmaller)
{
	return items[j].time > items[iSmaller].time;
}

function cmpDateAsc(j, iSmaller)
{
	return items[j].date < items[iSmaller].date;
}

function cmpDateDes(j, iSmaller)
{
	return items[j].date > items[iSmaller].date;
}

function cmpAtempAsc(j, iSmaller)
{
	return parseInt(items[j].temperature.air, 10) < parseInt(items[iSmaller].temperature.air, 10);
}

function cmpAtempDes(j, iSmaller)
{
	return parseInt(items[j].temperature.air, 10) > parseInt(items[iSmaller].temperature.air, 10);
}

function cmpWtempAsc(j, iSmaller)
{
	return parseInt(items[j].temperature.water, 10) < parseInt(items[iSmaller].temperature.water, 10);
}

function cmpWtempDes(j, iSmaller)
{
	return parseInt(items[j].temperature.water, 10) > parseInt(items[iSmaller].temperature.water, 10);
}

function sort_it(sortOn, function_)
{
	var res = new Array();
	var visited = new Array(itemsToShow.length);
	for (var j = 0; j < itemsToShow.length; j++) {
		visited[j] = false;
	}
	for (var i = 0; i < itemsToShow.length; i++) {
		for (var j = 0; j < itemsToShow.length; j++)
			if (visited[j] === false)
				var iSmaller = j;
		for (var j = 0; j < itemsToShow.length; j++) {
			if (function_(itemsToShow[j], itemsToShow[iSmaller])) {
				if (visited[j] === false) {
					iSmaller = j;
				}
			}
		}
		visited[iSmaller] = true;
		res.push(itemsToShow[iSmaller]);
	}
	itemsToShow = res;
	start = 0;
	viewInPage();
}

///////////////////////////////////////
//
//		Searching
//
//////////////////////////////////////

function Set()
{
	this.keys = new Array();
}

Set.prototype.contains = function(key)
{
	return (this.keys.indexOf(key) >= 0) ? true : false;
}

Set.prototype.push = function(key)
{
	if (!this.contains(key)) {
		this.keys.push(key);
	}
};

Set.prototype.isEmpty = function()
{
	return this.keys.length <= 0 ? true : false;
};

Set.prototype.forEach = function(do_) {
	this.keys.forEach (do_);
};

Set.prototype.Union = function(another_set)
{
	if (another_set === null) {
		return;
	}
	for (var i = 0; i < another_set.keys.length; i++) {
		this.push(another_set.keys[i]);
	};
};

function Node(value)
{
	this.children = new Array();
	this.value = value;
	this.key = new Set();
}

function Search_list_Modules(searchfor)
{
	document.getElementById("search_input").value = searchfor;
	SearchModules(searchfor);
}

function SearchModules(searchfor)
{
	var resultKeys = new Set(); //set

	if (searchfor.length <= 0) {
		//exit searching mode
		document.getElementById("search_input").style.borderColor = "initial";
		start = 0;
		itemsToShow = olditemstoshow;
		viewInPage();
		return;
	}

	searchingModules.forEach (function(x) {
		resultKeys.Union(x.search(searchfor));
	});

	if (searchingModules["location"].enabled === true)
		resultKeys.Union(searchingModules["location"].search(searchfor));

	if (searchingModules["divemaster"].enabled === true)
		resultKeys.Union(searchingModules["divemaster"].search(searchfor));

	if (searchingModules["buddy"].enabled === true)
		resultKeys.Union(searchingModules["buddy"].search(searchfor));

	if (searchingModules["notes"].enabled === true)
		resultKeys.Union(searchingModules["notes"].search(searchfor));

	if (searchingModules["tags"].enabled === true)
		resultKeys.Union(searchingModules["tags"].search(searchfor));

	if (resultKeys.isEmpty()) {
		//didn't find keys
		document.getElementById("search_input").style.borderColor = "red";
		itemsToShow = [];
		viewInPage();

		return;
	}
	//found keys
	document.getElementById("search_input").style.borderColor = "initial";
	itemsToShow = resultKeys.keys;
	start = 0;
	viewInPage();
}
///////////////////////////////////////
function SearchModule(enabled)
{
	this.head = new Node();
	this.enabled = enabled;
}

SearchModule.prototype.Enter_search_string = function(str, diveno)
{
	if (str == "" || !str)
		return;
	var res = str.toLowerCase().split(" ");
	for (var i = 0; i < res.length; i++) {
		insertIn(res[i], diveno, this.head);
		numberofwords++;
	}
}

SearchModule.prototype.Enter_search_tag = function(tags, diveno)
{
	if (!tags)
		return;
	for (var i = 0; i < tags.length; i++) {
		insertIn(tags[i], diveno, this.head);
		numberofwords++;
	}
}

SearchModule.prototype.search = function(x)
{
	return searchin(x.toLowerCase(), this.head);
}
////////////////////////////////////////

function insertIn(value, key, node)
{
	node.key.push(key);
	if (value.length <= 0)
		return;

	var this_char = value[0];
	value = value.substring(1, value.length);

	var i;
	for (i = 0; i < node.children.length; i++) {
		if (node.children[i].value == this_char) {
			return insertIn(value, key, node.children[i]);
		}
	}
	node.children[i] = new Node(this_char);
	insertIn(value, key, node.children[i]);
}

function searchin(value, node)
{
	if (value.length <= 0 || node.children.length <= 0)
		return node.key;

	var this_char = value[0];
	value = value.substring(1, value.length);

	for (var i = 0; i < node.children.length; i++) {
		if (node.children[i].value[0] == this_char) {
			return searchin(value, node.children[i]);
		}
	}
	return null;
}

//trips

var tripsShown;


/**
*This is the main function called to show/hide trips
*/
function toggleTrips()
{
	var trip_button = document.getElementById('trip_button');
	if (tripsShown) {
		tripsShown = false;
		trip_button.style.backgroundColor = "#dfdfdf";
		viewInPage();
	} else {
		showtrips();
		trip_button.style.backgroundColor = "#5f7f8f";
		tripsShown = true;
	}
}

function showtrips()
{
	var divelist = document.getElementById('diveslist');
	divelist.innerHTML = "";
	for (var i = 0; i < trips.length; i++) {
		divelist.innerHTML += '<ul id="trip_' + i + '" class="trips" onclick="toggle_trip_expansion(' + i + ')">' +
				      trips[i].name + ' ( ' + trips[i].dives.length + ' dives)' + '</ul>' + '<div id="trip_dive_list_' + i + '"></div>';
	};
	for (var i = 0; i < trips.length; i++) {
		unexpand_trip(i);
	}
}

function toggle_trip_expansion(trip)
{
	if (trips[trip].expanded === true) {
		unexpand_trip(trip);
	} else {
		expand_trip(trip);
	}
}

function expand_trip(trip)
{
	trips[trip].expanded = true;
	var d = document.getElementById("trip_dive_list_" + trip);
	for (var j in trips[trip].dives) {
		d.innerHTML += '<ul id="' + trips[trip].dives[j].number + '" onclick="toggleExpantion(this)" onmouseover="highlight(this)"' +
			       ' onmouseout="unhighlight(this)">' + getlimited(trips[trip].dives[j]) + '</ul>';
	}
}

function unexpand_trip(trip)
{
	trips[trip].expanded = false;
	var d = document.getElementById("trip_dive_list_" + trip);
	d.innerHTML = '';
}

function getItems()
{
	var count = 0;
	for (var i in trips) {
		for (var j in trips[i].dives) {
			items[count++] = trips[i].dives[j];
		}
	}
}

////////////////////////canvas///////////////////

/*
Some Global variables that hold the current shown dive data.
*/
var dive_id; //current shown ID
var points;  //reference to the samples array of the shown dive.
var ZERO_C_IN_MKELVIN = 273150;
var plot1;

/**
*Return the HTML string for a dive cylinder entry in the table.
*/
function get_cylinder_HTML(cylinder)
{
	return '<tr><td class="Cyl">' + cylinder.Type + '</td><td class="Cyl">' + cylinder.Size + '</td><td class="Cyl">' + cylinder.WPressure + '</td>' +
	       '<td class="Cyl">' + cylinder.SPressure + '</td><td class="Cyl">' + cylinder.EPressure + '</td><td class="Cyl">' + cylinder.O2 + '</td></tr>';
}

/**
*Return HTML table of cylinders of a dive.
*/
function get_cylinders_HTML(dive)
{
	var result = "";
	result += '<h2 class="det_hed">Dive equipments</h2><table><tr><td class="words">Type</td><td class="words">Size</td><td class="words">Work Pressure</td><td class="words">Start Pressure</td><td class="words">End Pressure</td><td class="words">O2</td></tr>';
	for (var i in dive.Cylinders) {
		result += get_cylinder_HTML(dive.Cylinders[i]);
	}
	result += '</table>';
	return result;
}

/**
Return the HTML string for a bookmark entry in the table.
*/
function get_bookmark_HTML(event)
{
	return '<tr><td class="Cyl">' + event.name + '</td><td class="Cyl">' + int_to_time(event.time) + '</td></tr>';
}

/**
*Return HTML table of bookmarks of a dive.
*/
function get_bookmarks_HTML(dive)
{
	var result = "";
	result += '<h2 class="det_hed">Events</h2><table><tr><td class="words">Name</td><td class="words">Time</td></tr>';
	for (var i in dive.events) {
		result += get_bookmark_HTML(dive.events[i]);
	}
	result += '</table>';
	return result;
}


/**
*Return HTML main data of a dive
*/
function get_dive_HTML(dive)
{
	return '<h2 class="det_hed">Dive Information</h2><table><tr><td class="words">Date: </td><td>' + dive.date +
	       '</td><td class="words">&nbsp&nbsp&nbsp&nbsp&nbspTime: </td><td>' + dive.time +
	       '</td><td class="words">&nbsp&nbsp&nbsp&nbsp&nbspLocation: </td><td>' + '<a onclick=\"Search_list_Modules(\'' + dive.location + '\')\">' +
	       dive.location + '</a>' +
	       '</td></tr></table><table><tr><td class="words">Rating:</td><td>' + putRating(dive.rating) +
	       '</td><td class="words">&nbsp&nbsp&nbspVisibilty:</td><td>' + putRating(dive.visibility) +
	       '</td></tr></table>' +
	       '<table><tr><td class="words">Air temp: </td><td>' + dive.temperature.air +
	       '</td><td class="words">&nbsp&nbsp&nbsp&nbspWater temp: </td><td>' + dive.temperature.water +
	       '</td></tr></table><table><tr><td class="words">DiveMaster: </td><td>' + dive.divemaster +
	       '</td></tr><tr><td class="words"><p>Buddy: </p></td><td>' + dive.buddy +
	       '</td></tr><tr><td class="words">Suit: </td><td>' + dive.suit +
	       '</td></tr><tr><td class="words">Tags: </td><td>' + putTags(dive.tags) +
	       '</td></tr></table><div style="margin:10px;"><p class="words">Notes: </p>' + dive.notes + '</div>';
};

function mkelvin_to_C(mkelvin)
{
	return (mkelvin - ZERO_C_IN_MKELVIN) / 1000.0;
}

function mbar_to_bar(mbar)
{
	return mbar / 1000;
}

function mm_to_meter(mm)
{
	return mm / (1000);
}

function format_two_digit(n)
{
	return n > 9 ? "" + n : "0" + n;
}

function int_to_time(n)
{
	return Math.floor((n) / 60) + ":" + format_two_digit((n) % (60)) + " min";
}

/**
*Main canvas draw function
*this calls the axis and grid initialization functions.
*/
function canvas_draw()
{
	document.getElementById("chart1").innerHTML = "";
	var d1 = new Array(); //depth
	var d2 = new Array(); //pressure
	var d3 = new Array(); //events
	var d4 = new Array(); //temperature
	var last = 0;
	for (var i = 0; i < items[dive_id].samples.length; i++) {
		d1.push([
			items[dive_id].samples[i][0] / 60,
			-1 * mm_to_meter(items[dive_id].samples[i][1])
		]);
		if (items[dive_id].samples[i][2] != 0) {
			d2.push([
				items[dive_id].samples[i][0] / 60,
				mbar_to_bar(items[dive_id].samples[i][2])
			]);
		}
		if (items[dive_id].samples[i][3] != 0) {
			d4.push([
				items[dive_id].samples[i][0] / 60,
				mkelvin_to_C(items[dive_id].samples[i][3]),
			]);
			last = items[dive_id].samples[i][3];
		} else {
			if (last != 0) {
				d4.push([
					items[dive_id].samples[i][0] / 60,
					mkelvin_to_C(last),
				]);
			}
		}
	}
	for (var i = 0; i < items[dive_id].events.length; i++) {
		d3.push([
			items[dive_id].events[i].time / 60,
			0,
		]);
	}
	plot1 = $.jqplot('chart1', [
					   d1,
					   d2,
					   d3,
					   d4,
				   ],
			 {
				 grid : {
					 drawBorder : true,
					 shadow : false,
					 background : 'rgba(0,0,0,0)'
				 },
				 highlighter : {
					 show : true,
					 tooltipContentEditor: function(str, seriesIndex, pointIndex, jqPlot) {
						if(seriesIndex===2)
						return items[dive_id].events[pointIndex].name;
						else
						return str;
					 }
				 },
				 seriesDefaults : {
					 shadowAlpha : 0.1,
					 shadowDepth : 2,
					 fillToZero : true
				 },
				 series :[
					 {
						 color : 'rgba(35,58,58,.6)',
						 negativeColor : 'rgba(35,58,58,.6)',
						 showMarker : true,
						 showLine : true,
						 fill : true,
						 yaxis : 'yaxis',
					 },
					 {
						 color : 'rgba(44, 190, 160, 0.7)',
						 showMarker : false,
						 rendererOptions : {
							 smooth : true,
						 },
						 yaxis : 'y2axis',
					 },
					 {
						  showLine:false,
						  markerOptions: { size: 10, style:"o" },
						  pointLabels: { show:false, } ,
					 },
					 {
						  showLine:true,
						  showMarker : false,
						  pointLabels: { show:false, } ,
						  yaxis : 'y3axis',
					 },
				 ],
				 axes : {
					 xaxis : {
						 pad : 1.0,
						 tickRenderer : $.jqplot.CanvasAxisTickRenderer,
						 tickOptions : {
							 showGridline : false,
							 formatString : '%i min',
							 angle : -30
						 }
					 },
					 yaxis : {
						 max : 0,
						 tickRenderer : $.jqplot.CanvasAxisTickRenderer,
						 tickOptions : {
						 formatter: function(format, value) { return -1 * value + "m"; },
							 formatString : '%.2fm'
						 },
						 pad : 2.05
					 },
					 y2axis : {
						 tickRenderer : $.jqplot.CanvasAxisTickRenderer,
						 tickOptions : {
							 formatString : '%ibar'
						 },
						 pad : 3.05
					 },
					 y3axis : {
						padMax : 3.05,
						tickOptions: {
							showGridline: false,
							showMark: false,
							showLabel: false,
							shadow: false,
							formatString : '%i C',
						},
					 },
				 }
			 });
}

/**
*Initialize the detailed view,
*set the global variables
*Fill the dive data
*Hide the list and show the canvas view.
*this is called to view the dive details.
*/
function showDiveDetails(dive)
{
	//set global variables
	dive_id = dive;
	points = items[dive_id].samples;

	//draw the canvas and initialize the view
	document.getElementById("diveinfo").innerHTML = get_dive_HTML(items[dive_id]);
	document.getElementById("dive_equipments").innerHTML = get_cylinders_HTML(items[dive_id]);
	document.getElementById("bookmarks").innerHTML = get_bookmarks_HTML(items[dive_id]);
	setDiveTitle(items[dive_id]);

	//hide the list of dives and show the canvas.
	document.getElementById("diveListPanel").style.display = 'none';
	document.getElementById("divePanel").style.display = 'block';
	canvas_draw();
}

function setDiveTitle(dive)
{
	document.getElementById("dive_no").innerHTML = "Dive No. " + (settings.subsurfaceNumbers === '0' ?
		dive.number + 1 : dive.subsurface_number);
	document.getElementById("dive_location").innerHTML = dive.location;
}

/**
*Show the list view and hide the detailed list view.
*this function have to clear any data saved by showDiveDetails
*/
function unshowDiveDetails(dive)
{
	start = dive_id;
	viewInPage();
	plot1 = null;
	document.getElementById("diveListPanel").style.display = 'block';
	document.getElementById("divePanel").style.display = 'none';
}

function nextDetailedDive()
{
	if (dive_id < items.length) {
		showDiveDetails(++dive_id);
	}
}

function prevDetailedDive()
{
	if (dive_id > 0) {
		showDiveDetails(--dive_id);
	}
}

/**
*This function handles keyboard events
*shift to next/prev dives by keyboard arrows.
*/
function switchDives(e)
{
	if (document.getElementById("divePanel").style.display == 'block') {
		e = e || window.event;
		if (e.keyCode == '37') {
			prevDetailedDive();
		} else if (e.keyCode == '39') {
			nextDetailedDive();
		}
	}
}
