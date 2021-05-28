var units  = ['b', 'KiB', 'MiB', 'GiB', 'TiB', 'PiB'];
var rates  = ['b/s', 'KiB/s', 'MiB/s', 'GiB/s', 'TiB/s', 'PiB/s'];
var shortrates  = ['b', 'K', 'M', 'G', 'T', 'P'];

function elapsedstr(elapsed) {
    if(elapsed < 60)
        return elapsed + ' seconds ago';

    elapsed /= 60
    if(elapsed < 60)
        return elapsed.toFixed(0) + ' minutes ago';

    return (elapsed / 60).toFixed(0) + ' hours ago';
}

function autosize(value) {
	var temp = value / 1024;
	var unitidx = 2;

	if(temp > 4096) {
		temp /= 1024;
		unitidx = 3;
	}

	return temp.toFixed(2) + ' ' + units[unitidx];
}

//
// return a value prefixed by zero if < 10
//
function zerolead(value) {
	return (value < 10) ? '0' + value : value;
}

//
// compute a scaled size with adapted prefix
//
function size(value, total) {
	uindex = 1;

	pc = Math.floor((value / total) * 100);

	for(; value > 1024; value /= 1024)
		uindex++;

	text = value.toFixed(2) + ' ' + units[uindex] + (total ? ' (' + pc + ' %)' : '');

	return $('<span>', {style: percentstyle(pc)}).html(text);
}

function rate(value) {
	value = value / 1024;
	uindex = 1;

	for(; value > 1024; value /= 1024)
		uindex++;

	return value.toFixed(2) + ' ' + rates[uindex];
}

var graph_items = 0;
var graph_calls = [];
var graph_read = [];
var graph_write = [];
var graph_calls_plot = null;
var graph_read_plot = null;
var graph_write_plot = null;

var backlog = null;

function fetch_stats(data) {
    $.get("/zdbfs-stats", parse_stats);
}

function graph_init() {
    for(var i = 0; i < 60; i++) {
        graph_calls.push([new Date() - (i * 1000), null]);
        graph_read.push([new Date() - (i * 1000), null]);
        graph_write.push([new Date() - (i * 1000), null]);
    }

    graph_items = graph_calls.length;

    graph_calls_plot = $.plot("#chart-calls", [graph_calls], {
        series: { color: '#18ab67' },
        xaxis: { mode: "time", timezone: "browser", ticks: 3 },
        yaxis: { min: 0, },
    });

    graph_read_plot = $.plot("#chart-read", [graph_read], {
        series: { color: '#1e90ff' },
        xaxis: { mode: "time", timezone: "browser", ticks: 3 },
        yaxis: { min: 0, },
    });

    graph_write_plot = $.plot("#chart-write", [graph_write], {
        series: { color: '#f86565' },
        xaxis: { mode: "time", timezone: "browser", ticks: 3 },
        yaxis: { min: 0, },
    });
}

function mb(size) {
    return parseInt(((size) / (1024 * 1024)).toFixed(0)).toLocaleString();
}

function parse_stats(data) {
    json = JSON.parse(data);

    // set original backlog
    if(backlog == null)
        backlog = json;

    callsps = (json["syscall"]["fuse"] - backlog["syscall"]["fuse"]);

    $(".values-calls").html(json["syscall"]["fuse"].toLocaleString());
    $(".values-calls-per-sec").html(callsps.toLocaleString());

    $(".values-cache-hit").html(json["cache"]["hit"].toLocaleString());
    $(".values-cache-miss").html(json["cache"]["miss"].toLocaleString());
    $(".values-cache-flush-linear").html(json["cache"]["linear_flush"].toLocaleString());

    $(".values-inodes").html(json["metadata"]["entries"].toLocaleString());
    $(".values-blocks").html(json["blocks"]["entries"].toLocaleString());
    $(".values-transit").html(json["temporary"]["entries"].toLocaleString());

    $(".values-read-blks").html((json["syscall"]["read"] - backlog["syscall"]["read"]).toLocaleString());
    $(".values-write-blks").html((json["syscall"]["write"] - backlog["syscall"]["write"]).toLocaleString());
    $(".values-read").html(mb(json["transfert"]["read_bytes"]));
    $(".values-write").html(mb(json["transfert"]["write_bytes"]));

    readspeed = json["transfert"]["read_bytes"] - backlog["transfert"]["read_bytes"];
    writespeed = json["transfert"]["write_bytes"] - backlog["transfert"]["write_bytes"];

    $(".values-read-speed").html(mb(readspeed));
    $(".values-write-speed").html(mb(writespeed));

    $(".values-virt-meta-size").html(mb(json["metadata"]["data_virtual_size_bytes"]));
    $(".values-phy-meta-size").html(mb(json["metadata"]["data_physical_size_bytes"]));
    $(".values-virt-data-size").html(mb(json["blocks"]["data_virtual_size_bytes"]));
    $(".values-phy-data-size").html(mb(json["blocks"]["data_physical_size_bytes"]));

    var meta_over = (json["metadata"]["data_physical_size_bytes"] / json["metadata"]["data_virtual_size_bytes"]) - 1;
    var data_over = (json["blocks"]["data_physical_size_bytes"] / json["blocks"]["data_virtual_size_bytes"]) - 1;
    $(".values-meta-overhead").html((meta_over * 100).toFixed(0) + "%");
    $(".values-data-overhead").html((data_over * 100).toFixed(0) + "%");

    var overhead = json["metadata"]["data_physical_size_bytes"] - json["metadata"]["data_virtual_size_bytes"];
    overhead += json["blocks"]["data_physical_size_bytes"] - json["blocks"]["data_virtual_size_bytes"];

    $(".values-total-overhead").html(mb(overhead));

    var keys = Object.keys(json["syscall"]);
    for(var i in keys) {
        var key = keys[i];
        $(".syscall-" + key).html(json["syscall"][key].toLocaleString());
        $(".syscall-" + key).addClass("inactive")
            .removeClass("badge-warning")
            .removeClass("badge-danger");

        var diff = json["syscall"][key] - backlog["syscall"][key];

        if(diff > 0)
            $(".syscall-" + key).removeClass("inactive");

        if(diff > 800)
            $(".syscall-" + key).addClass("badge-danger");

        else if(diff > 100)
            $(".syscall-" + key).addClass("badge-warning");
    }

    // graph
    if(graph_items == 0)
        graph_init();

    graph_calls = graph_calls.slice(1);
    graph_calls.push([new Date(), callsps]);

    graph_read = graph_read.slice(1);
    graph_read.push([new Date(), readspeed / (1024 * 1024)]);

    graph_write = graph_write.slice(1);
    graph_write.push([new Date(), writespeed / (1024 * 1024)]);

    graph_calls_plot.setData([graph_calls]);
    graph_calls_plot.setupGrid();
    graph_calls_plot.draw();

    graph_read_plot.setData([graph_read]);
    graph_read_plot.setupGrid();
    graph_read_plot.draw();

    graph_write_plot.setData([graph_write]);
    graph_write_plot.setupGrid();
    graph_write_plot.draw();

    // update backlog
    backlog = json;

    setTimeout(fetch_stats, 1000);
}

function connect() {
    $('#disconnected').hide();
    fetch_stats();
}

$(document).ready(function() {
    connect();
});
