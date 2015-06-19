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
	change_sort_col('1');
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
		divelist.innerHTML += '<ul id="' + itemsToShow[i] + '" onclick="toggleExpansion(event, this)"></ul>';
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
		divelist.innerHTML += '<ul id="' + indexes[i] + '" onclick="toggleExpansion(event, this)"></ul>';
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
	page.innerHTML = (start + 1) + ' to ' + (end + 1) + ' of ' + (itemsToShow.length) + ' ' + translate.dives;
}

/**
*Expand all dives in the view.
*/
function expandAll()
{
	if (tripsShown) {
		for (var i = 0 ; i < trips.length ; i++) {
			if (trips[i].expanded === false)
				expand_trip(i);
		}
	} else {
		for (var i = start; i < start + sizeofpage; i++) {
			if (i >= itemsToShow.length)
				break;
			unexpand(document.getElementById(itemsToShow[i]));
			items[itemsToShow[i]].expanded = false;
		}
	}
}

/**
*Collapse all dives in the view.
*/
function collapseAll()
{
	if (tripsShown) {
		for (var i = 0 ; i < trips.length ; i++) {
			if (trips[i].expanded === true)
				unexpand_trip(i);
		}
	} else {
		for (var i = start; i < start + sizeofpage; i++) {
			if (i >= itemsToShow.length)
				break;
			expand(document.getElementById(itemsToShow[i]));
			items[itemsToShow[i]].expanded = true;
		}
	}
}

function setNumberOfDives(e)
{
	var value = e.options[e.selectedIndex].value;
	sizeofpage = parseInt(value, 10);
	var end = start + sizeofpage - 1;
	viewInPage();
}

function toggleExpansion(e, ul)
{
	if (e.target.localName === "a" ) {
		return;
	}
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
	       '<div class="item">' + dive.dive_duration + '</div>' +
	       '<div class="item">' + put_depth_unit(dive.maxdepth) + " " + depth_unit + '</div></div>';
};

function getExpanded(dive)
{
	var res = '<table><tr><td class="words">' + translate.Date + ': </td><td>' + dive.date +
		  '</td><td class="words">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;' + translate.Time + ': </td><td>' + dive.time +
		  '</td><td class="words">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;' + translate.Location + ': </td><td>' + '<a onclick=\"Search_list_Modules(\'' + dive.location + '\', {location:true, divemaster:false, buddy:false, notes:false, tags:false,})\">' + dive.location + '</a>' + getDiveCoor(dive) +
		  '</td></tr></table><table><tr><td class="words">' + translate.Rating + ':</td><td>' + putRating(dive.rating) +
		  '</td><td class="words">&nbsp;&nbsp;&nbsp;' + translate.Visibility + ':</td><td>' + putRating(dive.visibility) +
		  '</td></tr></table>' +
		  '<table><tr><td class="words">' + translate.Air_Temp + ': </td><td>' + dive.temperature.air +
		  '</td><td class="words">&nbsp;&nbsp;&nbsp;&nbsp;' + translate.Water_Temp + ': </td><td>' + dive.temperature.water +
		  '</td></tr></table><table><tr><td class="words">' + translate.Max_Depth + ': </td><td>' + put_depth_unit(dive.maxdepth) + " " + depth_unit + '</td></tr><tr><td class="words">' + translate.Duration + ': </td><td>' + dive.dive_duration +
		  '</td></tr><tr><td class="words">' + translate.DiveMaster + ': </td><td>' + dive.divemaster +
		  '</td></tr><tr><td class="words"><p>' + translate.Buddy + ': </p></td><td>' + dive.buddy +
		  '</td></tr><tr><td class="words">' + translate.Suit + ': </td><td>' + dive.suit +
		  '</td></tr><tr><td class="words">' + translate.Tags + ': </td><td>' + putTags(dive.tags) +
		  '</td></tr></table><div style="margin:10px;"><p class="words">' + translate.Notes + ': </p>' + dive.notes + '</div>';
	if (settings.listOnly === '0') {
		res += '<center><a onclick="showDiveDetails(' + dive.number + ')">' + translate.Show_more_details + '</a></center>';
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
var duration = true;
var depth = true;
var locat = true;

/*
This variable keep the state of the col.
which is sorted upon it.
*/
var sort_based_on = '1'; // sorting is based on number by default.

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
	case '4': //Duration
		if (duration) {
			sort_it(sortOn, cmpDurDes);
		} else {
			sort_it(sortOn, cmpDurAsc);
		}
		break;
	case '5': //Max Depth
		if (depth) {
			sort_it(sortOn, cmpDepthDes);
		} else {
			sort_it(sortOn, cmpDepthAsc);
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
	case '4': //Duration
		duration = 1 - duration;
		break;
	case '5': //depth
		depth = 1 - depth;
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

function cmpNumAsc(j, iSmaller)
{
	return settings.subsurfaceNumbers === '0' ? items[j].number < items[iSmaller].number : items[j].subsurface_number < items[iSmaller].subsurface_number;
}

function cmpNumDes(j, iSmaller)
{
	return settings.subsurfaceNumbers === '0' ? items[j].number > items[iSmaller].number : items[j].subsurface_number > items[iSmaller].subsurface_number;
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

function cmpDurAsc(j, iSmaller)
{
	return items[j].duration < items[iSmaller].duration;
}

function cmpDurDes(j, iSmaller)
{
	return items[j].duration > items[iSmaller].duration;
}

function cmpDepthAsc(j, iSmaller)
{
	return items[j].maxdepth < items[iSmaller].maxdepth;
}

function cmpDepthDes(j, iSmaller)
{
	return items[j].maxdepth > items[iSmaller].maxdepth;
}

function sort_it(sortOn, function_)
{
	var res = new Array();
	var visited = new Array(itemsToShow.length);
	for (var j = 0; j < itemsToShow.length; j++) {
		visited[j] = false;
	}

	var iSmaller;
	for (var i = 0; i < itemsToShow.length; i++) {
		for (var j = 0; j < itemsToShow.length; j++)
			if (visited[j] === false)
				iSmaller = j;
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
		if (this.contains(another_set.keys[i])) {
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
	set_search_dropdown(searchOptions);
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
		set_search_dropdown(user_search_preference);
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
		document.getElementById('diveListPanel').style.display = 'block';
		document.getElementById('diveStat').style.display = 'none';
	} else {
		document.getElementById('diveListPanel').style.display = 'none';
		document.getElementById('diveStat').style.display = 'block';
		statsShows = true;
		showStats();
	}
}

function showStats()
{
	document.getElementById('diveStatsData').innerHTML = '';
	document.getElementById('diveStatsData').innerHTML += getDiveStats();
}

function getDiveStats()
{
	var res = "";
	res += '<table><tr id="stats_header">';
	res += '<td class="statscell">' + translate.Year + '</td><td class="statscell">#</td><td class="statscell">' + translate.Total_Time + '</td><td class="statscell">' + translate.Average_Time + '</td><td class="statscell">' + translate.Shortest_Time + '</td><td class="statscell">' + translate.Longest_Time + '</td><td class="statscell">' + translate.Average_Depth + '</td><td class="statscell">' + translate.Min_Depth + '</td><td class="statscell">' + translate.Max_Depth + '</td><td class="statscell">' + translate.Average_SAC + '</td><td class="statscell">' + translate.Min_SAC + '</td><td class="statscell">' + translate.Max_SAC + '</td><td class="statscell">' + translate.Average_Temp + '</td><td class="statscell">' + translate.Min_Temp + '</td><td class="statscell">' + translate.Max_Temp + '</td>';
	res += '</tr>';
	res += getStatsRows();
	res += '</table>';
	return res;
}

function getStatsRows()
{
	var res = "";
	for (var i = 0; i < divestat.length; i++) {
		res += '<tr onmouseout="stats_row_unhighlight(this)" onmouseover="stats_row_highlight(this)" class="stats_row"><td class="statscell">' + divestat[i].YEAR + '</td><td class="statscell">' + divestat[i].DIVES + '</td><td class="statscell">' + divestat[i].TOTAL_TIME + '</td><td class="statscell">' + divestat[i].AVERAGE_TIME + '</td><td class="statscell">' + divestat[i].SHORTEST_TIME + '</td><td class="statscell">' + divestat[i].LONGEST_TIME + '</td><td class="statscell">' + divestat[i].AVG_DEPTH + '</td><td class="statscell">' + divestat[i].MIN_DEPTH + '</td><td class="statscell">' + divestat[i].MAX_DEPTH + '</td><td class="statscell">' + divestat[i].AVG_SAC + '</td><td class="statscell">' + divestat[i].MIN_SAC + '</td><td class="statscell">' + divestat[i].MAX_SAC + '</td><td class="statscell">' + divestat[i].AVG_TEMP + '</td><td class="statscell">' + divestat[i].MIN_TEMP + '</td><td class="statscell">' + divestat[i].MAX_TEMP + '</td></tr>';
	}
	return res;
}

function stats_row_highlight(row)
{
	row.style.backgroundColor = "rgba(125,125,125,0.7)";
}

function stats_row_unhighlight(row)
{
	row.style.backgroundColor = "rgba(125,125,125,0.3)";
}

//trips

var tripsShown;
var HEADER_COLOR;
var BACKGROUND_COLOR;

function getDefaultColor(){
	var header = document.getElementById('header');
	var button = document.getElementById('no_dives_selector');
	HEADER_COLOR = document.defaultView.getComputedStyle(header).getPropertyValue('background-color');
	BACKGROUND_COLOR = document.defaultView.getComputedStyle(button).getPropertyValue('background-color');
}

/**
*This is the main function called to show/hide trips
*/
function toggleTrips()
{
	var trip_button = document.getElementById('trip_button');
	var controller = document.getElementById('controller');
	var no_dives_selector = document.getElementById('no_dives_selector');
	if (tripsShown) {
		tripsShown = false;
		trip_button.style.backgroundColor = BACKGROUND_COLOR;
		viewInPage();
		controller.style.display = '';
		no_dives_selector.style.display = '';
	} else {
		showtrips();
		trip_button.style.backgroundColor = HEADER_COLOR;
		tripsShown = true;
		dives_controller.style.display = 'none';
		no_dives_selector.style.display = 'none';
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
		d.innerHTML += '<ul id="' + trips[trip].dives[j].number + '" onclick="toggleExpansion(event, this)" onmouseover="highlight(this)"' +
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

function firstNonZero()
{
	for(var i = 0; i <= items[dive_id].samples.length-1; i++){
		if(items[dive_id].samples[i][2] !== 0)
			return items[dive_id].samples[i][2];
	}
}

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
	return '<tr><td class="Cyl">' + weight.weight + '</td><td class="Cyl">' + weight.description + '</td></tr>';
}

/**
*Return HTML table of weights of a dive.
*/
function get_weights_HTML(dive)
{
	if (!dive.Weights.length)
		return "";

	var result = "";
	result += '<table><tr><td class="words">' + translate.Weight + '</td><td class="words">' + translate.Type + '</td></tr>';
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
		var nonZeroCSPressure = firstNonZero();
		cSPressure = Math.round(put_pressure_unit(nonZeroCSPressure)).toFixed(1) + " " + pressure_unit;
	}

	if (cEPressure === "--") {
		var nonZeroCEPressure = lastNonZero();
		cEPressure = Math.round(put_pressure_unit(nonZeroCEPressure)).toFixed(1) + " " + pressure_unit;
	}

	var pressure_string = cylinder.O2;
	if (cylinder.O2 !== "Air") {
		pressure_string = 'O2: ' + cylinder.O2;
	}
	if (cylinder.He && cylinder.He !== "0.0%") {
		pressure_string += ' / He: ' + cylinder.He;
	}
	return '<tr><td class="Cyl">' + cylinder.Type + '</td><td class="Cyl">' + cylinder.Size + '</td><td class="Cyl">' + cylinder.WPressure + '</td>' + '<td class="Cyl">' + cSPressure + '</td><td class="Cyl">' + cEPressure + '</td><td class="Cyl">' + pressure_string + '</td></tr>';
}

/**
*Return HTML table of cylinders of a dive.
*/
function get_cylinders_HTML(dive)
{
	if (!dive.Cylinders.length)
		return "";

	var result = "";
	result += '<h2 class="det_hed">' + translate.Dive_equipment + '</h2><table><tr><td class="words">' + translate.Type + '</td><td class="words">' + translate.Size + '</td><td class="words">' + translate.Work_Pressure + '</td><td class="words">' + translate.Start_Pressure + '</td><td class="words">' + translate.End_Pressure + '</td><td class="words">'+translate.Gas+'</td></tr>';
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
		return 'O2: ' + o2 + ' / He: ' + he;
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
	if (!dive.events || dive.events <= 0)
		return "";
	var result = "";
	result += '<h2 class="det_hed">' + translate.Events + '</h2><table><tr><td class="words">' + translate.Name + '</td><td class="words">' + translate.Time + '</td><td class="words">' + translate.Value + '</td></tr>';
	for (var i in dive.events) {
		result += get_bookmark_HTML(dive.events[i]);
	}
	result += '</table>';
	return result;
}

function getDiveCoorString(coordinates){
	res = "";
	lat = coordinates.lat;
	lon = coordinates.lon;
	res += float_to_deg(lat) + ' , ' + float_to_deg(lon);
	return res;
}

function getDiveCoor(dive)
{
	if (!dive.coordinates)
		return "";
	return '<td class="words">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;' + translate.Coordinates + ': </td><td>' + '<a href="http://maps.google.com/maps?t=k&q=loc:' + dive.coordinates.lat + ',' + dive.coordinates.lon + '" target="_blank">' + getDiveCoorString(dive.coordinates) + '</a></td>';
}

/**
*Return HTML main data of a dive
*/
function get_dive_HTML(dive)
{
	var res = '<h2 class="det_hed">' + translate.Dive_information + '</h2><table><tr><td class="words">' + translate.Date + ': </td><td>' + dive.date +
		  '</td><td class="words">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;' + translate.Time + ': </td><td>' + dive.time +
		  '</td><td class="words">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;' + translate.Location + ': </td><td>' + '<a onclick=\"Search_list_Modules(\'' + dive.location + '\', {location:true, divemaster:false, buddy:false, notes:false, tags:false,})\">' + dive.location + '</a></td>' + getDiveCoor(dive) +
		  '</tr></table><table><tr><td class="words">' + translate.Rating + ':</td><td>' + putRating(dive.rating) +
		  '</td><td class="words">&nbsp;&nbsp;&nbsp;' + translate.Visibility + ':</td><td>' + putRating(dive.visibility) +
		  '</td></tr></table>' +
		  '<table><tr><td class="words">' + translate.Air_Temp + ': </td><td>' + dive.temperature.air +
		  '</td><td class="words">&nbsp;&nbsp;&nbsp;&nbsp;' + translate.Water_Temp + ': </td><td>' + dive.temperature.water +
		  '</td></tr></table><table><tr><td class="words">' + translate.Max_Depth + ': </td><td>' + put_depth_unit(dive.maxdepth) + " " + depth_unit + '</td></tr><tr><td class="words">' + translate.Duration + ': </td><td>' + dive.dive_duration +
		  '</td></tr><tr><td class="words">' + translate.DiveMaster + ': </td><td>' + dive.divemaster +
		  '</td></tr><tr><td class="words"><p>' + translate.Buddy + ': </p></td><td>' + dive.buddy +
		  '</td></tr><tr><td class="words">' + translate.Suit + ': </td><td>' + dive.suit +
		  '</td></tr><tr><td class="words">' + translate.Tags + ': </td><td>' + putTags(dive.tags) +
		  '</td></tr></table>'+ put_divecomputer_details(dive.divecomputers) +'<div style="margin:10px;"><p class="words">' + translate.Notes + ': </p>' + dive.notes + '</div>';
	return res;
};

function put_divecomputer_details(dc)
{
	if (dc.length <= 0)
		return;

	var res = '';
	res += '<p style="margin:10px;" class="words">Divecomputer:</p>';
	for (var i =0; i < dc.length; i++) {
		res += '<table>';
		res += '<tr><td>Model : </td><td>' + dc[i].model + '</td></tr>';
		if (dc[i].deviceid)
			res += '<tr><td>Device ID : </td><td>' + dc[i].deviceid + '</td></tr>';
		if (dc[i].diveid)
			res += '<tr><td>Dive ID : </td><td>' + dc[i].diveid + '</td></tr>';
		res += '</table><br>';
	}
	return res;
}

/**
*Return HTML dive status data
*/
function get_status_HTML(dive)
{
	return '<h2 class="det_hed">' + translate.Dive_Status + '</h2><table><tr><td class="words">SAC: </td><td>' + put_volume_unit(dive.sac).toFixed(2) + ' ' + volume_unit +
	       '/min' + '</td><td class="words">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;OTU: </td><td>' + dive.otu +
	       '</td><td class="words">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;CNS: </td><td>' + dive.cns + '</td></tr></table>';
};

/**
* Dive Photo View
*/

var success;
var error;
var missing_ids;

function show_loaded_photos()
{
	//dive photos in the current dive
	var dive = items[dive_id];
	var slider = "";
	document.getElementById("divephotos").style.display = 'block';
	for (var i = 0; i < dive.photos.length; i++) {
		slider += '<img src="' + location.pathname + '_files/photos/' + dive.photos[i].filename + '" alt="" height="240" width="240">';
	}
	document.getElementById("slider").innerHTML = slider;
}

function remove_missing_photos()
{
	//dive photos in the current dive
	var dive = items[dive_id];
	var actually_removed = 0;

	console.log(missing_ids.length);
	for(i = 0; i < missing_ids.length; i++){
		dive.photos.splice(missing_ids[i] - actually_removed, 1);
		actually_removed++;
	}
}

function check_loaded_photos()
{
	//hide the photos div if all photos are missing
	if (error >= items[dive_id].photos.length) {
		document.getElementById("divephotos").style.display = 'none';
		return;
	}

	//remove missing photos from the list
	remove_missing_photos();

	//view all remaining photos
	show_loaded_photos();
}

function check_loading_finished()
{
	//if all images wasn't loaded yet
	if (success + error < items[dive_id].photos.length)
		return;

	check_loaded_photos();
}

function load_photo(url, i) {
	var img = new Image();
	img.onload = function() {
		success++;
		check_loading_finished();
	};
	img.onerror = function() {
		error++;
		missing_ids.push(i);
		check_loading_finished();
	};
	img.src = url;
}

function load_all_photos(dive)
{
	//init
	success = 0;
	error = 0;
	missing_ids = new Array();

	//exit if no photos in this dive.
	if (!dive.photos || dive.photos.length <= 0) {
		document.getElementById("divephotos").style.display = 'none';
		return "";
	}

	for (var i = 0; i < dive.photos.length; i++) {
		load_photo(location.pathname + '_files/photos/' + dive.photos[i].filename, i);
	}
}

function prev_photo()
{
	var temp = items[dive_id].photos[0];
	var i;
	for (i = 0; i < items[dive_id].photos.length - 1; i++) {
		items[dive_id].photos[i] = items[dive_id].photos[i + 1]
	}
	items[dive_id].photos[i] = temp;
	show_loaded_photos();
}

function next_photo()
{
	var temp = items[dive_id].photos[items[dive_id].photos.length - 1];
	var i;
	for (i = items[dive_id].photos.length - 1; i > 0; i--) {
		items[dive_id].photos[i] = items[dive_id].photos[i - 1]
	}
	items[dive_id].photos[0] = temp;
	show_loaded_photos();
}

/**
* Helper functions
*/
function mkelvin_to_C(mkelvin)
{
	return (mkelvin - ZERO_C_IN_MKELVIN) / 1000.0;
}

function mkelvin_to_F(mkelvin)
{
	return mkelvin * 9 / 5000.0 - 459.670;
}

function mbar_to_bar(mbar)
{
	return mbar / 1000;
}

function mbar_to_psi(mbar)
{
	return (mbar * 0.0145037738);
}

function mm_to_meter(mm)
{
	return mm / (1000);
}

function mm_to_feet(mm)
{
	return mm * 0.00328084;
}

function gram_to_km(gram)
{
	return (gram / 1000).toFixed(1);
}

function ml_to_litre(ml)
{
	return ml / (1000);
}

function ml_to_cuft(ml)
{
	return ml / 28316.8466;
}

function format_two_digit(n)
{
	return n > 9 ? "" + n : "0" + n;
}

function int_to_time(n)
{
	return Math.floor((n) / 60) + ":" + format_two_digit((n) % (60)) + " min";
}

function float_to_deg(flt){
	var deg = 0 | flt;
	flt = (flt < 0 ? flt =- flt : flt);
        var min = 0 | flt % 1 * 60;
        var sec = (0 | flt * 60 % 1 * 6000) / 100;
        return deg + "&deg; " + min + "' " + sec + "\"";
}

var depth_unit = "m";
var temperature_unit = "C";
var pressure_unit = "bar";
var volume_unit = "l";

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
			-1 * put_depth_unit(items[dive_id].samples[i][1])
		]);
		if (items[dive_id].samples[i][2] !== 0) {
			pressureData.push([
				items[dive_id].samples[i][0] / 60,
				put_pressure_unit(items[dive_id].samples[i][2])
			]);
		}
		if (items[dive_id].samples[i][3] !== 0) {
			temperatureData.push([
				items[dive_id].samples[i][0] / 60,
				put_temperature_unit(items[dive_id].samples[i][3])
			]);
			last = items[dive_id].samples[i][3];
		} else {
			if (last !== 0) {
				temperatureData.push([
					items[dive_id].samples[i][0] / 60,
					put_temperature_unit(last)
				]);
			}
		}
	}
	if (items[dive_id].events) {
		for (var i = 0; i < items[dive_id].events.length; i++) {
			eventsData.push([
				items[dive_id].events[i].time / 60,
				0
			]);
		}
	}
	if (plot1) {
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
					 tooltipLocation: null,
					 tooltipContentEditor: function(str, seriesIndex, pointIndex, jqPlot) {
						if(seriesIndex===2)
						return items[dive_id].events[pointIndex].name;
						else
						return str.replace(",", " : ");
					 }
				 },
				 seriesDefaults : {
					 shadowAlpha : 0.1,
					 shadowDepth : 2,
					 fillToZero : true
				 },
				 series :[
					 {
						 color : 'rgba(75,98,98,.6)',
						 negativeColor : 'rgba(75,98,98,.6)',
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
							 formatString : '%imin'
						 },
						 label:'Time (min)'
					 },
					 yaxis : {
						 max : 0,
						 tickRenderer : $.jqplot.CanvasAxisTickRenderer,
						 tickOptions : {
						 formatter: function(format, value) { return -1 * value + " " + depth_unit;},
							 formatString : '%.2fm'
						 },
						 pad : 2.05
					 },
					 y2axis : {
						 min : 0,
						 tickRenderer : $.jqplot.CanvasAxisTickRenderer,
						 tickOptions : {
							 formatString : '%i '+pressure_unit
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
							formatString : '%i ' + temperature_unit
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
	document.getElementById("dive_equipment").innerHTML = get_cylinders_HTML(items[dive_id]);
	document.getElementById("dive_equipment").innerHTML += get_weights_HTML(items[dive_id]);
	document.getElementById("bookmarks").innerHTML = get_bookmarks_HTML(items[dive_id]);
	document.getElementById("divestats").innerHTML = get_status_HTML(items[dive_id]);
	load_all_photos(items[dive_id]);
	setDiveTitle(items[dive_id]);

	//hide the list of dives and show the canvas.
	document.getElementById("diveListPanel").style.display = 'none';
	document.getElementById("divePanel").style.display = 'block';
	canvas_draw();
	scrollToTheTop();
}

function setDiveTitle(dive)
{
	document.getElementById("dive_no").innerHTML = translate.Dive_No + (settings.subsurfaceNumbers === '0' ?
		dive.number + 1 : dive.subsurface_number);
	document.getElementById("dive_location").innerHTML = dive.location;
}

/**
*Show the list view and hide the detailed list view.
*this function have to clear any data saved by showDiveDetails
*/
function unshowDiveDetails(dive)
{
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

function scrollToTheTop()
{
	window.scrollTo(0, 0);
}

window.onresize = function(event)
{
	if (plot1)
		plot1.replot({
			resetAxes : false
		});
};

function translate_page()
{
	document.getElementById("number_header").innerHTML = translate.Number;
	document.getElementById("date_header").innerHTML = translate.Date;
	document.getElementById("time_header").innerHTML = translate.Time;
	document.getElementById("location_header").innerHTML = translate.Location;
	document.getElementById("duration_header").innerHTML = translate.Duration;
	document.getElementById("maxdepth_header").innerHTML = translate.Max_Depth;
	document.getElementById("adv_srch_sp").innerHTML = translate.Advanced_Search;
	document.getElementById("expnd_all_btn").innerHTML = translate.Expand_All;
	document.getElementById("claps_all_btn").innerHTML = translate.Collapse_All;
	document.getElementById("trip_button").innerHTML = translate.trips;
	document.getElementById("stats_button").innerHTML = translate.Statistics;
	document.getElementById("div_profil_lbl").innerHTML = translate.Dive_profile;
	document.getElementById("bk_to_ls_lbl").innerHTML = translate.Back_to_List;
	document.getElementById("bk_to_ls_lbl2").innerHTML = translate.Back_to_List;
}

function set_units()
{
	if (settings.unit_system == "Imperial") {
		depth_unit = "ft";
		temperature_unit = "F";
		pressure_unit = "psi";
		volume_unit = "cuft";
	} else if (settings.unit_system == "Meteric") {
		depth_unit = "m";
		temperature_unit = "C";
		pressure_unit = "bar";
		volume_unit = "l";
	} else if (settings.unit_system == "Personalize") {
		depth_unit = settings.units.depth == "METER"?"m":"ft";
		pressure_unit = settings.units.pressure == "BAR"?"bar":"psi";
		temperature_unit = settings.units.temperature == "CELSIUS"?"C":"F";
		volume_unit = settings.units.volume == "CUFT"?"cuft":"l";
	}
	// meteric by default
}

function put_volume_unit(ml)
{
	if (settings.unit_system == "Imperial")
		return ml_to_cuft(ml);
	else if (settings.unit_system == "Personalize" && settings.units.volume == "CUFT")
		return ml_to_cuft(ml);
	return ml_to_litre(ml);
}

function put_weight_unit(grams)
{
	if (settings.unit_system == "Imperial")
		return grams_to_lbs(ml);
	else if (settings.unit_system == "Personalize" && settings.units.weight == "LBS")
		return grams_to_lbs(ml);
	return grams_to_kgs(ml);
}

function put_temperature_unit(mkelvin)
{
	if (settings.unit_system == "Imperial")
		return mkelvin_to_F(mkelvin);
	else if (settings.unit_system == "Personalize" && settings.units.temperature == "FAHRENHEIT")
		return mkelvin_to_F(mkelvin);
	return mkelvin_to_C(mkelvin);
}

function put_depth_unit(mm)
{
	if (settings.unit_system == "Imperial")
		return mm_to_feet(mm).toFixed(1);
	else if (settings.unit_system == "Personalize" && settings.units.depth == "FEET")
		return mm_to_feet(mm).toFixed(1);
	return mm_to_meter(mm);
}

function put_pressure_unit(mbar)
{
	if (settings.unit_system == "Imperial")
		return mbar_to_psi(mbar);
	else if (settings.unit_system == "Personalize" && settings.units.pressure == "PSI")
		return mbar_to_psi(mbar);
	return mbar_to_bar(mbar);
}
