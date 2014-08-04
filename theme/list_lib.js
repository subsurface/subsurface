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
	viewInPage();
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
		  '</td><td class="words">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Time: </td><td>' + dive.time +
		  '</td><td class="words">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Location: </td><td>' + '<a onclick=\"Search_list_Modules(\'' + dive.location + '\', {location:true, divemaster:false, buddy:false, notes:false, tags:false,})\">' + dive.location + '</a>' +
		  '</td></tr></table><table><tr><td class="words">Rating:</td><td>' + putRating(dive.rating) +
		  '</td><td class="words">&nbsp;&nbsp;&nbsp;Visibilty:</td><td>' + putRating(dive.visibility) +
		  '</td></tr></table>' +
		  '<table><tr><td class="words">Air temp: </td><td>' + dive.temperature.air +
		  '</td><td class="words">&nbsp;&nbsp;&nbsp;&nbsp;Water temp: </td><td>' + dive.temperature.water +
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
		result += '<a onclick=\"Search_list_Modules(\'' + tags[i] + '\', {location:false, divemaster:false, buddy:false, notes:false, tags:true,})\">' + tags[i] + '</a>';
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
var number = true;
var time = true;
var date = true;
var air = true;
var water = true;
var locat = true;

/*
This variable keep the state of the col.
which is sorted upon it.
*/
var sort_based_on = 1; // sorting is based on number by default.

function change_sort_col(sortOn)
{
	sort_based_on = sortOn;
	toggle_sort_state(sortOn);
	list_sort(sortOn);
}

function list_sort(sortOn)
{
	switch (sortOn) {
	case '1': //number
		if (number) {
			sort_it(sortOn, cmpNumAsc);
		} else {
			sort_it(sortOn, cmpNumDes);
		}
		break;
	case '2': //date
		if (date) {
			sort_it(sortOn, cmpDateAsc);
		} else {
			sort_it(sortOn, cmpDateDes);
		}
		break;
	case '3': //time
		if (time) {
			sort_it(sortOn, cmpTimeDes);
		} else {
			sort_it(sortOn, cmpTimeAsc);
		}
		break;
	case '4': //Air temp
		if (air) {
			sort_it(sortOn, cmpAtempDes);
		} else {
			sort_it(sortOn, cmpAtempAsc);
		}
		break;
	case '5': //Water temp
		if (water) {
			sort_it(sortOn, cmpWtempDes);
		} else {
			sort_it(sortOn, cmpWtempAsc);
		}
		break;
	case '6': //Location
		if (locat) {
			sort_it(sortOn, cmpLocationDes);
		} else {
			sort_it(sortOn, cmpLocationAsc);
		}
		break;
	}
}

function toggle_sort_state(sortOn)
{
	switch (sortOn) {
	case '1': //number
		number = 1 - number;
		break;
	case '2': //date
		date = 1 - date;
		break;
	case '3': //time
		time = 1 - time;
		break;
	case '4': //Air temp
		air = 1 - air;
		break;
	case '5': //Water temp
		water = 1 - water;
		break;
	case '6': //Location
		locat = 1 - locat;
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

Set.prototype.intersect = function(another_set)
{
	if (another_set === null) {
		return;
	}
	var result = new Array();
	for (var i = 0; i < another_set.keys.length; i++) {
		if(this.contains(another_set.keys[i])) {
			result.push(another_set.keys[i]);
		}
	};
	this.keys = result;
};

function Node(value)
{
	this.children = new Array();
	this.value = value;
	this.key = new Set();
}

function Search_list_Modules(searchfor, searchOptions)
{
	document.getElementById("search_input").value = searchfor;
	SearchModules(searchfor, searchOptions);
}

function SearchModules(searchfor, searchOptions)
{
	unshowDiveDetails(dive_id);
	var resultKeys = new Set(); //set

	if (searchfor.length <= 0) {
		//exit searching mode
		document.getElementById("search_input").style.borderColor = "initial";
		start = 0;
		itemsToShow = olditemstoshow;
		list_sort(sort_based_on);
		viewInPage();
		return;
	}

	var keywords = searchfor.split(" ");

	if (searchOptions === null) {
		searchOptions = {};
		searchOptions.location = searchingModules["location"].enabled;
		searchOptions.divemaster = searchingModules["divemaster"].enabled;
		searchOptions.buddy = searchingModules["buddy"].enabled;
		searchOptions.notes = searchingModules["notes"].enabled;
		searchOptions.tags = searchingModules["tags"].enabled;
	}

	for (var i = 0; i < keywords.length; i++) {
		var keywordResult = new Set();

		if (searchOptions.location === true)
			keywordResult.Union(searchingModules["location"].search(keywords[i]));

		if (searchOptions.divemaster === true)
			keywordResult.Union(searchingModules["divemaster"].search(keywords[i]));

		if (searchOptions.buddy === true)
			keywordResult.Union(searchingModules["buddy"].search(keywords[i]));

		if (searchOptions.notes === true)
			keywordResult.Union(searchingModules["notes"].search(keywords[i]));

		if (searchOptions.tags === true)
			keywordResult.Union(searchingModules["tags"].search(keywords[i]));

		if (resultKeys.isEmpty()) {
			resultKeys.Union(keywordResult);
		} else {
			resultKeys.intersect(keywordResult);
		}
	}

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
	list_sort(sort_based_on);
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
	if (str === "" || !str)
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

//stats

var statsShows;

/**
*This is the main function called to show/hide trips
*/
function toggleStats()
{
	var stats_button = document.getElementById('stats_button');
	if (statsShows) {
		statsShows = false;
		stats_button.style.backgroundColor = "#dfdfdf";
		document.getElementById('diveListPanel').style.display='block';
		document.getElementById('diveStat').style.display='none';
	} else {
		document.getElementById('diveListPanel').style.display='none';
		document.getElementById('diveStat').style.display='block';
		stats_button.style.backgroundColor = "#5f7f8f";
		statsShows = true;
		showStats();
	}
}

function showStats()
{
	document.getElementById('diveStatsData').innerHTML = '';
	document.getElementById('diveStatsData').innerHTML += getDiveStats();
}

function getDiveStats(){
	var res = "";
	res += '<table><tr id="stats_header">';
	res += '<td class="statscell">Year</td><td class="statscell">#</td><td class="statscell">Total Time</td><td class="statscell">Avarage Time</td><td class="statscell">Shortest Time</td><td class="statscell">Longest Time</td><td class="statscell">Avarage Depth</td><td class="statscell">Min Depth</td><td class="statscell">Max Depth</td><td class="statscell">Average SAC</td><td class="statscell">Min SAC</td><td class="statscell">Max SAC</td><td class="statscell">Average Temp</td><td class="statscell">Min Temp</td><td class="statscell">Max Temp</td>';
	res += '</tr>';
	res += getStatsRows();
	res += '</table>';
	return res;
}

function getStatsRows(){
	var res = "";
	for(var i = 0; i < divestat.length ; i++) {
	res += '<tr><td class="statscell">'+divestat[i].YEAR+'</td><td class="statscell">'+divestat[i].DIVES+'</td><td class="statscell">'+divestat[i].TOTAL_TIME+'</td><td class="statscell">'+divestat[i].AVERAGE_TIME+'</td><td class="statscell">'+divestat[i].SHORTEST_TIME+'</td><td class="statscell">'+divestat[i].LONGEST_TIME+'</td><td class="statscell">'+divestat[i].AVG_DEPTH+'</td><td class="statscell">'+divestat[i].MIN_DEPTH+'</td><td class="statscell">'+divestat[i].MAX_DEPTH+'</td><td class="statscell">'+divestat[i].AVG_SAC+'</td><td class="statscell">'+divestat[i].MIN_SAC+'</td><td class="statscell">'+divestat[i].MAX_SAC+'</td><td class="statscell">'+divestat[i].AVG_TEMP+'</td><td class="statscell">'+divestat[i].MIN_TEMP+'</td><td class="statscell">'+divestat[i].MAX_TEMP+'</td></tr>';
	}
	return res;
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

function lastNonZero()
{
	for(var i = items[dive_id].samples.length-1; i >= 0; i--){
		if(items[dive_id].samples[i][2] !== 0)
			return items[dive_id].samples[i][2];
	}
}

/**
*Return the HTML string for a dive weight entry in the table.
*/
function get_weight_HTML(weight)
{
	return '<tr><td class="Cyl">' + gram_to_km(weight.weight) + ' kg ' + '</td><td class="Cyl">' + weight.description + '</td></tr>';
}

/**
*Return HTML table of weights of a dive.
*/
function get_weights_HTML(dive)
{
	var result = "";
	result += '<table><tr><td class="words">Weight</td><td class="words">Type</td></tr>';
	for (var i in dive.Weights) {
		result += get_weight_HTML(dive.Weights[i]);
	}
	result += '</table>';
	return result;
}

/**
*Return the HTML string for a dive cylinder entry in the table.
*/
function get_cylinder_HTML(cylinder)
{
	var cSPressure = cylinder.SPressure;
	var cEPressure = cylinder.EPressure;

	if (cSPressure === "--") {
		cSPressure = mbar_to_bar(items[dive_id].samples[0][2]) + " bar";
	}

	if (cEPressure === "--") {
		var nonZeroCEPressure = lastNonZero();
		cEPressure = mbar_to_bar(nonZeroCEPressure) + " bar";
	}

	return '<tr><td class="Cyl">' + cylinder.Type + '</td><td class="Cyl">' + cylinder.Size + '</td><td class="Cyl">' + cylinder.WPressure + '</td>' + '<td class="Cyl">' + cSPressure + '</td><td class="Cyl">' + cEPressure + '</td><td class="Cyl">' + cylinder.O2 + '</td></tr>';
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

function get_event_value(event)
{
	if (event.type == 11 || event.type == 25) { // gas change
		var he = event.value >> 16;
		var o2 = event.value & 0xffff;
		return 'He: ' + he + ' - O2: ' + o2;
	}
	if (event.type == 23) { // heading
		return event.value;
	}
	return '-';
}

/**
Return the HTML string for a bookmark entry in the table.
*/
function get_bookmark_HTML(event)
{
	return '<tr><td class="Cyl">' + event.name + '</td><td class="Cyl">' + int_to_time(event.time) + '</td><td class="Cyl">' + get_event_value(event) + '</td></tr>';
}

/**
*Return HTML table of bookmarks of a dive.
*/
function get_bookmarks_HTML(dive)
{
	if (dive.events <= 0)
		return "";
	var result = "";
	result += '<h2 class="det_hed">Events</h2><table><tr><td class="words">Name</td><td class="words">Time</td><td class="words">Value</td></tr>';
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
	       '</td><td class="words">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Time: </td><td>' + dive.time +
	       '</td><td class="words">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Location: </td><td>' + '<a onclick=\"Search_list_Modules(\'' + dive.location + '\', {location:true, divemaster:false, buddy:false, notes:false, tags:false,})\">' +
	       dive.location + '</a>' +
	       '</td></tr></table><table><tr><td class="words">Rating:</td><td>' + putRating(dive.rating) +
	       '</td><td class="words">&nbsp;&nbsp;&nbsp;Visibilty:</td><td>' + putRating(dive.visibility) +
	       '</td></tr></table>' +
	       '<table><tr><td class="words">Air temp: </td><td>' + dive.temperature.air +
	       '</td><td class="words">&nbsp;&nbsp;&nbsp;&nbsp;Water temp: </td><td>' + dive.temperature.water +
	       '</td></tr></table><table><tr><td class="words">DiveMaster: </td><td>' + dive.divemaster +
	       '</td></tr><tr><td class="words"><p>Buddy: </p></td><td>' + dive.buddy +
	       '</td></tr><tr><td class="words">Suit: </td><td>' + dive.suit +
	       '</td></tr><tr><td class="words">Tags: </td><td>' + putTags(dive.tags) +
	       '</td></tr></table><div style="margin:10px;"><p class="words">Notes: </p>' + dive.notes + '</div>';
};

/**
*Return HTML dive status data
*/
function get_status_HTML(dive)
{
	return '<h2 class="det_hed">Dive Status</h2><table><tr><td class="words">Sac: </td><td>' + ml_to_litre(dive.sac) +
	       ' l/min' + '</td><td class="words">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Otu: </td><td>' + dive.otu +
	       '</td><td class="words">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Cns: </td><td>' + dive.cns +
	       '</td></tr></table>';
};

function get_dive_photos(dive)
{
	if (dive.photos.length <= 0) {
		document.getElementById("divephotos").style.display = 'none';
		return "";
	}
	var slider = "";
	document.getElementById("divephotos").style.display = 'block';
	for (var i = 0; i < dive.photos.length; i++) {
		slider += '<img src="'+location.pathname +
			'_files/photos/'+dive.photos[i].filename+'" alt="" height="240" width="240">';
	}
	return slider;
}

function prev_photo()
{
	var temp = items[dive_id].photos[0];
	var i;
	for (i = 0; i < items[dive_id].photos.length - 1; i++) {
		items[dive_id].photos[i] = items[dive_id].photos[i + 1]
	}
	items[dive_id].photos[i] = temp;
	document.getElementById("slider").innerHTML = get_dive_photos(items[dive_id]);
}

function next_photo()
{
	var temp = items[dive_id].photos[items[dive_id].photos.length - 1];
	var i;
	for (i = items[dive_id].photos.length - 1; i > 0; i--) {
		items[dive_id].photos[i] = items[dive_id].photos[i - 1]
	}
	items[dive_id].photos[0] = temp;
	document.getElementById("slider").innerHTML = get_dive_photos(items[dive_id]);
}

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

function gram_to_km(gram)
{
	return gram / 1000;
}

function ml_to_litre(ml)
{
	return ml / (1000);
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
	var depthData = new Array();
	var pressureData = new Array();
	var eventsData = new Array();
	var temperatureData = new Array();
	var last = 0;
	for (var i = 0; i < items[dive_id].samples.length; i++) {
		depthData.push([
			items[dive_id].samples[i][0] / 60,
			-1 * mm_to_meter(items[dive_id].samples[i][1])
		]);
		if (items[dive_id].samples[i][2] !== 0) {
			pressureData.push([
				items[dive_id].samples[i][0] / 60,
				mbar_to_bar(items[dive_id].samples[i][2])
			]);
		}
		if (items[dive_id].samples[i][3] !== 0) {
			temperatureData.push([
				items[dive_id].samples[i][0] / 60,
				mkelvin_to_C(items[dive_id].samples[i][3])
			]);
			last = items[dive_id].samples[i][3];
		} else {
			if (last !== 0) {
				temperatureData.push([
					items[dive_id].samples[i][0] / 60,
					mkelvin_to_C(last)
				]);
			}
		}
	}
	for (var i = 0; i < items[dive_id].events.length; i++) {
		eventsData.push([
			items[dive_id].events[i].time / 60,
			0
		]);
	}
	if (plot1)
	{
		$('chart1').unbind();
		plot1.destroy();
	}
	plot1 = $.jqplot('chart1', [
					depthData,
					pressureData,
					eventsData,
					temperatureData
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
						 yaxis : 'yaxis'
					 },
					 {
						 color : 'rgba(44, 190, 160, 0.7)',
						 showMarker : false,
						 rendererOptions : {
							 smooth : true
						 },
						 yaxis : 'y2axis'
					 },
					 {
						  showLine:false,
						  markerOptions: { size: 10, style:"o" },
						  pointLabels: { show:false }
					 },
					 {
						  showLine:true,
						  showMarker : false,
						  pointLabels: { show:false },
						  yaxis : 'y3axis'
					 }
				 ],
				 axes : {
					 xaxis : {
						 pad : 1.0,
						 tickRenderer : $.jqplot.CanvasAxisTickRenderer,
						 tickOptions : {
							 showGridline : false,
							 formatString : '%i'
						 },
						 label:'Time (min)'
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
						 min : 0,
						 tickRenderer : $.jqplot.CanvasAxisTickRenderer,
						 tickOptions : {
							 formatString : '%ibar'
						 },
						 pad : 3.05
					 },
					 y3axis : {
						padMax : 3.05,
						max : 50,
						tickOptions: {
							showGridline: false,
							showMark: false,
							showLabel: false,
							shadow: false,
							formatString : '%i C'
						}
					 }
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
	document.getElementById("dive_equipments").innerHTML += get_weights_HTML(items[dive_id]);
	document.getElementById("bookmarks").innerHTML = get_bookmarks_HTML(items[dive_id]);
	document.getElementById("divestats").innerHTML = get_status_HTML(items[dive_id]);
	document.getElementById("slider").innerHTML = get_dive_photos(items[dive_id]);
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
	if (dive_id < items.length - 1) {
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

window.onresize = function(event) {
      plot1.replot( { resetAxes: true } );
};
