// Put all onload AJAX calls here, and event listeners
jQuery(document).ready(function() {

    //at start
    $("#logTable").hide(0);
    $("#viewTable").hide(0);
    $("#otherData").hide(0);
    $("#pathTable").hide(0);
    $("#creategpx").trigger("reset");
    $("#addRouteForm").trigger("reset");
    $("#getPathsForm").trigger("reset");
    $("#loginDb").trigger("reset");

    //db hides
    $("#dbFuncs").hide(0); //hide data must check if logged in first
    $("#logoutDiv").hide(0); //shown only when logged in
    $("#loginDbDiv").hide(0); //shown only when logged out
    $("#qRes").hide(0); //result of query table show only when query is successful
    $("#qResP").hide(0);
    $("#qResP2").hide(0);

    $("#o5").hide(0);
    $("#o4").hide(0);
    $("#o3").hide(0);
    $("#o2").hide(0);


    var loadedFile = "/failed";
    jQuery.ajax({
        async: false,
        type: 'get',            //Request type
        dataType: 'json',       //Data type 
        url: '/uploads',   //The server endpoint we are connecting to
        data: {},
        success: function (data) {

           if (data.length <= 0) {
                console.log("No files, hide table, show message.")
                isFiles = false;
           } else {
                $("#logTable").show(0);
                $("#noFiles").hide(0);
                isFiles = true;
                loadedFile = populateLogTable(data);
                buildDropdown(data);
                var loadedFile2 = document.getElementById('addRoute').value;
                var form = document.getElementById('filenameR');
                form.value = loadedFile2;
                var selected88 = document.getElementById("selectQ").value;
                updateSelect(selected88);
                console.log(data);
           }

        },
        fail: function(error) {
            isFiles = false;
            console.log(error); 
        }
    });

    jQuery.ajax({
        async: false,
        type: 'get',            //Request type
        //dataType: 'string',       //Data type
        url: loadedFile,  //The server endpoint we are connecting to
        data: {},
        success: function (data) {
            console.log(data);
            $("#viewTable").show(0);
            updateTableView(data);
        },
        fail: function(error) {
            console.log(error); 
        }
    });


    $("#select").change(function() {
        var loadedFile = "updateForm/" + document.getElementById('select').value;

        $("#otherData").hide(0);
        //$("#noFiles2").hide(0);
        
        jQuery.ajax({
            async: false,
            type: 'get',            //Request type
            //dataType: 'string',       //Data type 
            url: loadedFile,  //The server endpoint we are connecting to
            data: {},
            success: function (data) {
                console.log(data);
                purgeViewTable();
                updateTableView(data);
            },
            fail: function(error) {
                console.log(error); 
            }
        });

    });


    $("#getPathsForm").submit(function(e) {

        e.preventDefault();

        purgePathTable();

        if (checkPath()) {

            var sLat = document.getElementById("startLat").value;
            var sLon = document.getElementById("startLon").value;
            var eLat = document.getElementById("endLat").value;
            var eLon = document.getElementById("endLon").value;
            var tol = document.getElementById("tolerance").value;

            jQuery.ajax({
                type: 'get',            //Request type
                url: "/getPaths/" + sLat + "/" + sLon + "/" + eLat + "/" + eLon + "/" + tol + "/" + validFiles,  //The server endpoint we are connecting to
                data: {},
                success: function (data) {
                    //var arr = JSON.parse(data);
                    purgePathTable();
                    updatePathTable(data);
                },
                fail: function(error) {
                    console.log(error); 
                }
            });
        }
    });

    $("#addRoute").change(function() {
        var loadedFile = document.getElementById('addRoute').value;
        var form = document.getElementById('filenameR');
        form.value = loadedFile;
    });

    $('#loginDbBut').click(function(e) { //login

        e.preventDefault();

        if (checkLogin()) {

            var user = document.getElementById("dbInput");
            var pass = document.getElementById("dbInput2");
            var name = document.getElementById("dbInput3");
            
            jQuery.ajax({
                type: 'get',            //Request type
                url: "/loginDb/" + user.value + "/" + pass.value + "/" + name.value,
                data: {},
                success: function (data) {
                    if (data == "Failed to login.") {
                        alert("Login data was invalid. Try again.");
                    } else {
                        alert("Logged in successfully.");
                        alert("Database has\n" + data[1] + " Files\n" + data[2] + " Routes\n" + data[3] + " Points");
                        $("#loginDbDiv").hide(0);
                        var p = document.getElementById("loggedName");
                        p.innerHTML = "Logged in as: " + data[0];
                        $("#dbFuncs").show(0); 
                        $("#logoutDiv").show(0); 
                    }
                },
                fail: function(error) {
                    console.log(error);
                }
            });
        }
    });

    $('#logoutDb').click(function(e) {//logout

        e.preventDefault();
            
        jQuery.ajax({
            type: 'get',            //Request type
            url: "/logout",
            data: {},
            success: function (data) {
                if (data == "good") { //successful logout
                    alert("Succesfully logged out.");
                    $("#dbFuncs").hide(0); 
                    $("#logoutDiv").hide(0); 
                    $("#qRes").hide(0); 
                    $("#qResP").hide(0);
                    $("#qResP2").hide(0);
                    $("#loginDbDiv").show(0);
                    //setTimeout(rel, 500);
                } else {
                    alert("Unknown error. Failed to logout.");
                    setTimeout(rel, 500);
                }
            },
            fail: function(error) {
                console.log(error);
            }
        });
    });

    $('#dbStatus').click(function(e) {//get status

        e.preventDefault();
            
        jQuery.ajax({
            type: 'get',            //Request type
            url: "/dbStatus",
            data: {},
            success: function (data) {
                if (data != "fail") { //successful logout
                    alert("Database has\n" + data[0] + " Files\n" + data[1] + " Routes\n" + data[2] + " Points");
                } else {
                    alert("Failed to retrieve database status.");
                }
            },
            fail: function(error) {
                console.log(error);
            }
        });
    });


    $('#clearData').click(function(e) {//get status

        e.preventDefault();
            
        jQuery.ajax({
            type: 'get',            //Request type
            url: "/clearData",
            data: {},
            success: function (data) {
                if (data != "fail") { //successful logout
                    alert("All table data cleared successfully.");
                    alert("Database has\n" + data[0] + " Files\n" + data[1] + " Routes\n" + data[2] + " Points");
                } else {
                    alert("Failed to clear table data.");
                }
            },
            fail: function(error) {
                console.log(error);
            }
        });
    });

    $('#storeFiles').click(function(e) {//get status

        e.preventDefault();
            
        if (checkFiles()) {
            jQuery.ajax({
                type: 'post',            //Request type
                url: "/storeFiles",
                data: {files:validFiles},
                success: function (data) {
                    if (data != "fail") { //successful logout
                        alert("All files have been successfully stored.");
                        alert("Database has\n" + data[0] + " Files\n" + data[1] + " Routes\n" + data[2] + " Points");
                    } else {
                        alert("Failed to store files.");
                    }
                },
                fail: function(error) {
                    console.log(error);
                }
            });
        }
    });

    //handle logged in /logged out display
    jQuery.ajax({
        type: 'get',
        url: "/auth",
        data: {},
        success: function (data) {
            if (data == "out") { //not logged in

                $("#dbFuncs").hide(0); 
                $("#logoutDiv").hide(0); 
                $("#qRes").hide(0); 
                $("#qResP").hide(0);
                $("#qResP2").hide(0);
                $("#loginDbDiv").show(0); 

            } else { //logged in

                $("#loginDbDiv").hide(0);

                var p = document.getElementById("loggedName");
                p.innerHTML = "Logged in as: " + data;

                $("#dbFuncs").show(0); 
                $("#logoutDiv").show(0); 

            }
        },
        fail: function(error) {
            alert("Could not receive logged in status.");
            console.log(error); 
        }
    });

    $("#selectQ").change(function(e) {

        var selected88 = document.getElementById("selectQ").value;

        updateSelect(selected88);

    });

    $("#submitSort").click(function(e) {

        e.preventDefault();

        var sort = document.getElementById("sortRby").value;

        if (sort.indexOf("name") != -1) {
            sort = "name";
        } else {
            sort = "len";
        }

        jQuery.ajax({
            type: 'get',
            url: "/o1/" + sort,
            data: {},
            success: function (data) {
                alert("Displaying Query Data Table.");
                purgeRouteTable();
                createRouteTable(data);
                $("#qRes").show(0);
            },
            fail: function(error) {
                alert("Could not display query");
                console.log(error); 
            }
        });

    });


    $("#selectQ").change(function(e) {

        purgeRouteTable();
        purgePointTable();
        purgePointTable2();
        $("#qResP").hide(0);
        $("#qResP2").hide(0);
        $("#qRes").hide(0);

    });


    $("#submitSortFile").click(function(e) {

        e.preventDefault();

        var sort = document.getElementById("sortRby2").value;

        if (sort.indexOf("name") != -1) {
            sort = "name";
        } else {
            sort = "len";
        }

        var file = document.getElementById("selFile").value;

        jQuery.ajax({
            type: 'get',
            url: "/o2/" + sort + "/" + file,
            data: {},
            success: function (data) {
                if (data != "fail") {
                    alert("Displaying Query Data Table.");
                    purgeRouteTable();
                    createRouteTable(data);
                    $("#qRes").show(0);
                } else {
                    alert("Could not display query. Most likely selected file has not been stored in database.");
                }
            },
            fail: function(error) {
                alert("Could not display query. Most likely error connecting.");
                console.log(error); 
            }
        });

    });


    $("#selFile2").change(function(e) {

        var menu = document.getElementById("selRoute");

        var len = menu.options.length;

        for (i = len-1; i>=0; i--) {
            menu.options[i] = null;
        }
        
        var sellied = $("#selFile2").find(":selected").text();
        var table = document.getElementById("logTable");
        for (i = 1; i < table.rows.length; i++) {
            var row = table.rows[i];
            if (row.cells[0].innerText == sellied) {
                var k;
                for (k = 0; k < row.cells[4].innerText; k++) {
                    var option = document.createElement("option");
                    option.text = "Route " + (k+1);
                    menu.add(option, k);
                }
            }
        }

    });

    $("#submitPointRoute").click(function(e) {

        e.preventDefault();

        var file = document.getElementById("selFile2").value;
        var menu = document.getElementById("selRoute");

        num = menu.value.split(" ").pop();
        num--;

        if (menu.options.length > 0) {
            jQuery.ajax({
                type: 'get',
                url: "/o3/" + file + "/" + num,
                data: {},
                success: function (data) {
                    if (data != "fail") {
                        alert("Displaying Query Data Table.");
                        purgePointTable();
                        createPointTable(data);
                        $("#qResP").show(0);
                    } else {
                        alert("Could not display query. Most likely selected file has not been stored in database OR selected route was created without being logged in.");
                    }
                },
                fail: function(error) {
                    alert("Could not display query. Most likely error connecting.");
                    console.log(error); 
                }
            });
        } else {
            alert("No routes to search.")
        }

    });

    $("#submitPointFile").click(function(e) {

        e.preventDefault();

        var sort = document.getElementById("sortRby3").value;

        if (sort.indexOf("name") != -1) {
            sort = "name";
        } else {
            sort = "len";
        }

        var file = document.getElementById("selFile3").value;

        jQuery.ajax({
            type: 'get',
            url: "/o4/" + sort + "/" + file,
            data: {},
            success: function (data) {
                if (data != "fail") {
                    alert("Displaying Query Data Table.");
                    purgePointTable2();
                    createPointTable2(data);
                    $("#qResP2").show(0);
                } else {
                    alert("Could not display query. Most likely selected file has not been stored in database.");
                }
            },
            fail: function(error) {
                alert("Could not display query. Most likely error connecting.");
                console.log(error); 
            }
        });

    });


    $("#submitN").click(function(e) {

        e.preventDefault();

        var sort = document.getElementById("sortRby4").value;
        var sort2 = document.getElementById("sortRby5").value;


        if (sort.indexOf("long") != -1) {
            sort = "long";
        } else {
            sort = "short";
        }

        if (sort2.indexOf("name") != -1) {
            sort2 = "name";
        } else {
            sort2 = "len";
        }

        var num = document.getElementById("numIn").value;
        var file = document.getElementById("selFile4").value;

        if (!isNaN(num) && num > 0) {
            jQuery.ajax({
                type: 'get',
                url: "/o5/" + sort + "/" + file + "/" + num + "/" + sort2,
                data: {},
                success: function (data) {
                    if (data != "fail") {
                        alert("Displaying Query Data Table.");
                        purgeRouteTable();
                        createRouteTable(data);
                        $("#qRes").show(0);
                    } else {
                        alert("Could not display query. Most likely selected file has not been stored in database OR route data was added while logged out.");
                    }
                },
                fail: function(error) {
                    alert("Could not display query. Most likely error connecting.");
                    console.log(error); 
                }
            });
        } else {
            alert("N must be a positive integer.")
        }

    });

});


function purgeRouteTable() {

    var table = document.getElementById("qRes");
    var numRow = table.rows.length;

    var i;
    for (i = 1; i < numRow; i++) {

        table.deleteRow(1);

    }

}

function purgePointTable() {

    var table = document.getElementById("qResP");
    var numRow = table.rows.length;

    var i;
    for (i = 1; i < numRow; i++) {

        table.deleteRow(1);

    }

}

function purgePointTable2() {

    var table = document.getElementById("qResP2");
    var numRow = table.rows.length;

    var i;
    for (i = 1; i < numRow; i++) {

        table.deleteRow(1);

    }

}

function createPointTable2(passed) {

    var table = document.getElementById("qResP2");
    var data = passed.info;
    var names = passed.name;
    var k;
    var nCount = 1;
    var rowCount = 1;
    
    for (k = 0; k < data.length; k++) {

        var namerr = names[k].route_name;
        var len = names[k].route_len;

        if (namerr == "NULL" && data[k].length > 0) {
            namerr = "Unnamed Route " + nCount;
            nCount++;
        }

        var i;
        for (i = 0; i < data[k].length; i++) {
            
            var row = table.insertRow(rowCount);

            var ind = row.insertCell(0);
            var name = row.insertCell(1);
            var lat = row.insertCell(2);
            var lon = row.insertCell(3);
            var id = row.insertCell(4);
            var rId = row.insertCell(5);
            var rName = row.insertCell(6);
            var rLen = row.insertCell(7);

            ind.innerHTML = data[k][i].point_index;
            name.innerHTML = data[k][i].point_name;
            lat.innerHTML = data[k][i].latitude;
            lon.innerHTML = data[k][i].longitude;
            id.innerHTML = data[k][i].point_id;
            rId.innerHTML = data[k][i].route_id;

            rName.innerHTML = namerr;
            rLen.innerHTML = len;

            rowCount++;

        }

    }

}

function createPointTable(data) {

    var table = document.getElementById("qResP");

    var i;
    for (i = 0; i < data.length; i++) {

        var row = table.insertRow(i+1);

        var ind = row.insertCell(0);
        var name = row.insertCell(1);
        var lat = row.insertCell(2);
        var lon = row.insertCell(3);
        var id = row.insertCell(4);
        var rId = row.insertCell(5);

        ind.innerHTML = data[i].point_index;
        name.innerHTML = data[i].point_name;
        lat.innerHTML = data[i].latitude;
        lon.innerHTML = data[i].longitude;
        id.innerHTML = data[i].point_id;
        rId.innerHTML = data[i].route_id;

    }


}


function createRouteTable(data) {

    var table = document.getElementById("qRes");

    var i;
    for (i = 0; i < data.length; i++) {

        var row = table.insertRow(i+1);

        var name = row.insertCell(0);
        var len = row.insertCell(1);
        var rId = row.insertCell(2);
        var gId = row.insertCell(3);

        name.innerHTML = data[i].route_name;
        len.innerHTML = data[i].route_len;
        rId.innerHTML = data[i].route_id;
        gId.innerHTML = data[i].gpx_id;

    }

}


function updateSelect(sel) {

    if (sel.indexOf('1') != -1) {
        $("#o5").hide(0);
        $("#o4").hide(0);
        $("#o3").hide(0);
        $("#o2").hide(0);
        $("#o1").show(0);
        //get all routes displayed in table
    } else if (sel.indexOf('2') != -1) {
        $("#o5").hide(0);
        $("#o4").hide(0);
        $("#o3").hide(0);
        $("#o1").hide(0);
        var i;
        var menu = document.getElementById("selFile");
        menu.remove(0);
        for (i = 0; i < validFiles.length; i++) {
            var option = document.createElement("option");
            option.text = validFiles[i];
            menu.add(option, i);
        }
        $("#o2").show(0);
    } else if (sel.indexOf('3') != -1) {
        $("#o5").hide(0);
        $("#o4").hide(0);
        $("#o2").hide(0);
        $("#o1").hide(0);
        var i;
        var menu = document.getElementById("selFile2");
        menu.remove(0);
        for (i = 0; i < validFiles.length; i++) {
            var option = document.createElement("option");
            option.text = validFiles[i];
            menu.add(option, i);
        }
        //sel route
        var sellied = $("#selFile2").find(":selected").text();
        var table = document.getElementById("logTable");
        var menu = document.getElementById("selRoute");
        menu.remove(0);
        for (i = 1; i < table.rows.length; i++) {
            var row = table.rows[i];
            if (row.cells[0].innerText == sellied) {
                var k;
                for (k = 0; k < row.cells[4].innerText; k++) {
                    var option = document.createElement("option");
                    option.text = "Route " + (k+1);
                    menu.add(option, k);
                }
            }
        }
        $("#o3").show(0);
    } else if (sel.indexOf('4') != -1) {
        $("#o5").hide(0);
        $("#o3").hide(0);
        $("#o2").hide(0);
        $("#o1").hide(0);
        var i;
        var menu = document.getElementById("selFile3");
        menu.remove(0);
        for (i = 0; i < validFiles.length; i++) {
            var option = document.createElement("option");
            option.text = validFiles[i];
            menu.add(option, i);
        }
        $("#o4").show(0);
    } else if (sel.indexOf('5') != -1) {
        $("#o4").hide(0);
        $("#o3").hide(0);
        $("#o2").hide(0);
        $("#o1").hide(0);
        var i;
        var menu = document.getElementById("selFile4");
        menu.remove(0);
        for (i = 0; i < validFiles.length; i++) {
            var option = document.createElement("option");
            option.text = validFiles[i];
            menu.add(option, i);
        }
        $("#o5").show(0);
    } else {
        console.log("critical error in updateSelect()");
    }

}


function checkLogin() {

    var user = document.getElementById("dbInput");
    var pass = document.getElementById("dbInput2");
    var name = document.getElementById("dbInput3");

    if (user.value.length == 0) {
        alert("Username cannot be empty");
        return false;
    }

    if (pass.value.length == 0) {
        alert("Password cannot be empty");
        return false;
    }

    if (name.value.length == 0) {
        alert("Name cannot be empty");
        return false;
    }

    return true;

}

function rel() {

    window.location.replace(window.location.href.split("#")[0]);
  
}

var isFiles = false;
var toLoad = "";
var validFiles = new Array();

//populate log table on startup with data array containing gpx filenames
function populateLogTable(data) {

    var i;
    var loaded = "";
    var first = true;
    var table = document.getElementById("logTable");
    var skipped = 0;
    for (i = 0; i < data.length; i++) {

        if (data[i] == "failed") {
            skipped++;
            continue;
        }

        data[i] = data[i].split("\n").join(" "); //checkfunc
        var obj = JSON.parse(data[i]);

        if (first == true) {
            loaded = "updateForm/" + obj.filename.split("uploads/").pop();
            first = false;
        }

        var row = table.insertRow(i+1-skipped);

        var name = row.insertCell(0);
        var version = row.insertCell(1);
        var creator = row.insertCell(2);
        var wpt = row.insertCell(3);
        var rte = row.insertCell(4);
        var trk = row.insertCell(5);

        validFiles.push(obj.filename.split("uploads/").pop());

        name.innerHTML = '<a href="' + obj.filename + '">' + obj.filename.split("uploads/").pop() + '</a>';

        //not implemented yet
        version.innerHTML = obj.version;
        creator.innerHTML = obj.creator;
        wpt.innerHTML = obj.numWaypoints;
        rte.innerHTML = obj.numRoutes;
        trk.innerHTML = obj.numTracks;

    }

    if (skipped == i) {
        $("#logTable").hide(0);
        $("#noFiles").show(0);
        isFiles = false;
        loaded = "failed";
    }

    return loaded;

}


function buildDropdown(data) {

    var i;
    var menu = document.getElementById("select");
    var routeMenu = document.getElementById("addRoute");
    //var form = document.getElementById("")
    routeMenu.remove(0);
    menu.remove(0);
    var skipped = 0;
    var first = true;
    for (i = 0; i < data.length; i++) {

        if (data[i] == "failed") {
            skipped++;
            continue;
        }

        data[i] = data[i].split("\n").join(" "); //checkfunc
        var obj = JSON.parse(data[i]);

        var option = document.createElement("option");
        option.text = obj.filename.split("uploads/").pop();

        var option2 = document.createElement("option");
        option2.text = obj.filename.split("uploads/").pop();

        menu.add(option, i+1-skipped);
        routeMenu.add(option2, i+1-skipped);


    }

    if (skipped == i) {
        var option = document.createElement("option");
        option.text = "No Files";
        menu.add(option);

        var option2 = document.createElement("option");
        option2.text = "No Files";
        routeMenu.add(option2);
    }

}


function setToLoad(filename) {

    var url = '/updateForm/' + filename;
    
    toLoad = url;

}

function updateTableView(data) {

    if (data == "failed") {
        $("#viewTable").hide(0);
        return;
    }

    var r1, r2, t1, t2;

    var f1 = true;
    var f2 = true;

    var i;
    for (i = 0; i < data.length; i++) {

        if (data.charAt(i) == '[' && f1 == true) {
            f1 = false;
            r1 = i;
        }

        if (data.charAt(i) == ']' && f2 == true) {
            f2 = false;
            r2 = i;
        }

        if (data.charAt(i) == '[' && f1 == false) {
            t1 = i;
        }

        if (data.charAt(i) == ']' && f2 == false) {
            t2 = i;
        }

    }

    var rte = data.substr(r1, r2-r1+1);
    var trk = data.substr(t1, t2-t1+1);

    rte = rte.split("\n").join(" "); //checkfunc
    trk = trk.split("\n").join(" ");
    var rObj = JSON.parse(rte);
    var tObj = JSON.parse(trk);


    var table = document.getElementById("viewTable");
    var numRow = 1;
    
    for (i = 0; i < rObj.length; i++) {

        var row = table.insertRow(numRow);
        numRow++;

        var comp = row.insertCell(0);
        var name = row.insertCell(1);
        var numP = row.insertCell(2);
        var len = row.insertCell(3);
        var loop = row.insertCell(4);

        var link = "Route " + (i + 1);

        comp.innerHTML = "<a onclick='toggleTableRoute(" + (i+1) + ");' href='#'>" +  link + "</a>";

        //name.innerHTML = rObj[i].name;
        var file = document.getElementById("select").value;
        name.innerHTML = "<form enctype='multipart/form-data' target='dummy' method='post' onsubmit='getUpdatedVal()' id='routeName' action='updateRouteName/" + file + "/" + (i+1) + "'> <input name='name' type='text' id='name" + (i+1) + "' value='" + rObj[i].name + "'> </form>"

        numP.innerHTML = rObj[i].numPoints;
        len.innerHTML = rObj[i].len + "m";
        loop.innerHTML = rObj[i].loop;

    }

    var toAdd = i;

    for (i = 0; i < tObj.length; i++) {

        var row = table.insertRow(numRow);
        numRow++;

        var comp = row.insertCell(0);
        var name = row.insertCell(1);
        var numP = row.insertCell(2);
        var len = row.insertCell(3);
        var loop = row.insertCell(4);

        var link = "Track " + (i+1);

        comp.innerHTML = "<a onclick='toggleTableTrack(" + (i+1) + ");' href='#'>" +  link + "</a>";


        //name.innerHTML = tObj[i].name;
        var file = document.getElementById("select").value;
        name.innerHTML = "<form id='trackName' enctype='multipart/form-data' target='dummy' method='post' action='updateTrackName/" + file + "/" + (i+1) + "'> <input name='name' type='text' id='name" + (i+1) + "' value='" + tObj[i].name + "'> </form>"

        numP.innerHTML = tObj[i].points;
        len.innerHTML = tObj[i].len + "m";
        loop.innerHTML = tObj[i].loop;

    }

}


function purgeViewTable() {

    var table = document.getElementById("viewTable");
    var numRow = table.rows.length;

    var i;
    for (i = 1; i < numRow; i++) {

        table.deleteRow(1);

    }

}


function purgeOtherTable() {

    var table = document.getElementById("otherData");
    var numRow = table.rows.length;

    var i;
    for (i = 0; i < numRow; i++) {

        table.deleteRow(0);

    }

    //$("#noFiles2").hide(0);
    $("#otherData").hide(0);
    //$("#title").hide(0);

}

function updateTableOther(data) {

    data = data.split("\n").join(" "); //remove \n chars

    try {
        var obj = JSON.parse(data);
    } catch (e) {
        console.log("Coulnd not parse gpx data json string: Likely newline char causing error");
        var obj = JSON.parse("[]");
    }

    var table = document.getElementById("otherData");

    var i;
    var name;
    var data;

    for (i = 0; i < obj.length; i++) {

        var row = table.insertRow(table.rows.length);

        row.style.width = "100%";

        name = row.insertCell(0);
        data = row.insertCell(1);

        name.innerHTML = "<b>" + obj[i].name;
        data.innerHTML = obj[i].data;

        name.style.width = "15rem";
        data.style.width = "40rem";

    }

    if (obj.length > 0) {
        console.log("displaying other gpx data");
        $("#otherData").show(0);
        alert("Displaying other gpx data.");
        //$("#title").show(0);
    } else {
        $("#otherData").hide(0);
        console.log("Empty gpx data array");
        //var p = document.getElementById("noFiles2");
        //p.innerHTML = "No Other GPX Data to Show";
        //$("#noFiles2").show(0);
        alert("No Other GPX Data");
    }

}

function toggleTableRoute(id) {

    var loadedFile = "updateOtherRouteData/" + document.getElementById('select').value + "/" + id;
    jQuery.ajax({
        type: 'get',            //Request type
        //dataType: 'string',       //Data type - we will use JSON for almost everything 
        url: loadedFile,  //The server endpoint we are connecting to
        data: {},
        success: function (data) {
            console.log(data);
            purgeOtherTable();
            updateTableOther(data);
        },
        fail: function(error) {
            console.log(error); 
        }
    });

}


function toggleTableTrack(id) {

    var loadedFile = "updateOtherTrackData/" + document.getElementById('select').value + "/" + id;
    jQuery.ajax({
        type: 'get',            //Request type
        //dataType: 'string',       //Data type - we will use JSON for almost everything 
        url: loadedFile,  //The server endpoint we are connecting to
        data: {},
        success: function (data) {
            console.log(data);
            purgeOtherTable();
            updateTableOther(data);
        },
        fail: function(error) {
            console.log(error); 
        }
    });

}

var numWpt = 1;
var latt = false;
var long = false;
function onKey(code, clas) {

    //code 1 -> lon, 0 -> lat
    console.log(clas + " == " + numWpt);
    if (code == 0 && clas == numWpt) {
        latt = true;
    } else if (code == 1 && clas == numWpt) {
        long = true;
    }

    if (latt == true && long == true) {
        latt = false;
        long = false;

        numWpt++;

        var form = document.getElementById('numR');
        form.value = numWpt;

        var form = document.getElementById("addRouteForm");

        var inputName = document.createElement("input");
        var inputLat = document.createElement("input");
        var inputLon = document.createElement("input");

        inputName.type = "text";
        inputName.name = "wptName";
        inputName.id = "wptName";
        inputName.placeholder = "Waypoint Name (optional)";
        inputName.style.marginRight = "0.45%";
        inputName.value = "";

        inputLat.type = "text";
        inputLat.name = "wptLat";
        inputLat.id = "wptLat";
        inputLat.placeholder = "Latitude [-90, 90]";
        inputLat.setAttribute("onkeypress", "onKey(0, this.className)");
        inputLat.style.marginRight = "0.45%";
        inputLat.className = numWpt;

        inputLon.type = "text";
        inputLon.name = "wptLon";
        inputLon.id = "wptLon";
        inputLon.placeholder = "Longitude [-180, 180]";
        inputLon.setAttribute("onkeypress", "onKey(1, this.className)");
        inputLon.className = numWpt;

        form.appendChild(inputName);
        form.appendChild(inputLat);
        form.appendChild(inputLon);

    }

}


function checkForm() {

    var check = false;

    if (validFiles.length == 0) {
        alert("No files to search from. Try uploading one!");
        return false;
    }

    //var form = document.getElementById("addRouteForm");
    var i;


    if (numWpt == 1) {
        var name = document.getElementsByName("wptName");
        if (name[0].value.length == 0) {
            name[0].value = "..//..//";
        }
        setTimeout(rel, 500);
        return true;
    } 
    /*if (numWpt == 1 && latt == false) {
        alert("All latitudes (except last) must have value and be between -90 and 90.");
        return false;
    }
    if (numWpt == 1 && long == false) {
        alert("All longitudes (except last) must have value and be between -180 and 180.");
        return false;
    }*/

    if (numWpt > 1) {
        for (i = 1; i < numWpt; i++) {

            var lat = document.getElementsByName("wptLat");
            var lon = document.getElementsByName("wptLon");


            if (lat[i-1].value > 90.0 || lat[i-1].value < -90.0 || lat[i-1].length == 0 || isNaN(lat[i-1].value)) {
                alert("All latitudes (except last) must have value and be between -90 and 90.");
                return false;
            }

            if (lon[i-1].value >= 180.0 || lon[i-1].value < -180.0 || lon[i-1].length == 0 || isNaN(lon[i-1].value)) {
                alert("All longitudes (except last) must have value and be between -180 and 179.99.");
                return false;
            }

        }
    }

    if (numWpt > 1) {

        for (i=1; i < numWpt; i++) {

            var lat = document.getElementsByName("wptLat");
            var lon = document.getElementsByName("wptLon");

            if (lat[i-1].value.length == 0 || isNaN(lat[i-1].value)) {
                alert("All latitudes (except last) must be filled and contain a valid number.");
                return false;
            }

            if (lon[i-1].value.length == 0 || isNaN(lat[i-1].value)) {
                alert("All longitudes (except last) must be filled and contain a valid number.");
                return false;
            }

        }

        for (i = 1; i < numWpt; i++) {

            var name = document.getElementsByName("wptName");
            if (name[i-1].value.length == 0) {

                name[i-1].value = "..//..//";

            }
        }

    }

    setTimeout(rel, 500);
    return true;
}


function checkUpload() {

    var file = document.getElementById("uploadFile");

    if (file.value == "") {
        alert('Select a file to upload.');
        return false;
    }

    if (!file.value.endsWith(".gpx")) {
        alert('Error: Not a gpx file.');
        return false;
    }

    setTimeout(rel, 500);
    return true;

}


function checkGpx() {

    var name = document.getElementsByName("gpxName")[0].value;
    var creator = document.getElementsByName("gpxCre")[0].value;
    var version = document.getElementsByName("gpxVer")[0].value;

    if (name.length == 0) {
        alert('Error: Name field cannot be left blank.');
        return false;
    }

    if (version.length == 0) {
        alert('Error: Version field cannot be left blank.');
        return false;
    }

    if (creator.length == 0) {
        alert('Error: Creator field cannot be left blank.');
        return false;
    }

    if (!name.endsWith(".gpx")) {
        alert('Error: Filename requires \".gpx\" extension.');
        return false;
    }

    if (isNaN(version)) {
        alert('Error: Version must be valid number.');
        return false;
    }

    

    if (name.indexOf("/") > -1) {
        alert('Error: Filename cannot be a custom directory.');
        return false;
    }

    setTimeout(rel, 500);
    return true;

}

function checkFiles() {
    if (validFiles.length == 0) {
        alert("No files on server.");
        return false;
    }
    return true;
}


function checkPath() {

    if (validFiles.length == 0) {
        alert("No files to search from. Try uploading one!");
        return false;
    }

    if (isNaN(document.getElementById("startLat").value) || document.getElementById("startLat").value.length == 0) {
        alert("All fields must contain valid numbers.");
        return false;
    }else if (isNaN(document.getElementById("startLon").value) || document.getElementById("startLon").value.length == 0) {
        alert("All fields must contain valid numbers.");
        return false;
    }else if (isNaN(document.getElementById("endLat").value) || document.getElementById("endLat").value.length == 0) {
        alert("All fields must contain valid numbers.");
        return false;
    }else if (isNaN(document.getElementById("endLon").value) || document.getElementById("endLon").value.length == 0) {
        alert("All fields must contain valid numbers.");
        return false;
    }else if (isNaN(document.getElementById("tolerance").value) || isNaN(document.getElementById("tolerance").value) < 0 || document.getElementById("tolerance").value.length == 0) {
        alert("All fields must contain valid numbers. Tolerance >= 0");
        return false;
    }

    if (document.getElementById("startLat").value > 90 || document.getElementById("startLat").value < -90) {
        alert("Latitude must be between -90 and 90.");
        return false;
    }
    if (document.getElementById("endLat").value > 90 || document.getElementById("endLat").value < -90) {
        alert("Latitude must be between -90 and 90.");
        return false;
    }

    if (document.getElementById("startLon").value > 180 || document.getElementById("startLon").value < -180) {
        alert("Longitude must be between -180 and 180.");
        return false;
    }
    if (document.getElementById("endLon").value > 180 || document.getElementById("endLon").value < -180) {
        alert("Longitude must be between -180 and 180.");
        return false;
    }

    //setTimeout(rel, 0);
    return true;

}


function purgePathTable() {

    var table = document.getElementById("pathTable");
    var numRow = table.rows.length;

    var i;
    for (i = 1; i < numRow; i++) {

        table.deleteRow(1);

    }

    //$("#noFiles2").hide(0);
    $("#pathTable").hide(0);
    //$("#title").hide(0);

}

function updatePathTable(data) {

    var arr;
    try {
        arr = JSON.parse(data);
    } catch(e) {
        alert('No Paths Found.');
        return;
    }

    if (arr.length == 0) {
        alert('No Paths Found.');
        return;
    }

    var table = document.getElementById("pathTable");
    var numRow = 1;
    
    //routes
    var skipped = 0;
    for (i = 0; i < arr.length; i++) {

        if (arr[i].type == "R") {

        var row = table.insertRow(numRow);
        numRow++;

        var comp = row.insertCell(0);
        var name = row.insertCell(1);
        var numP = row.insertCell(2);
        var len = row.insertCell(3);
        var loop = row.insertCell(4);

        var link = "Route " + (i + 1 - skipped);
        comp.innerHTML = link;

        name.innerHTML = arr[i].name;

        numP.innerHTML = arr[i].numPoints;
        len.innerHTML = arr[i].len + "m";
        loop.innerHTML = arr[i].loop;

        } else {
            skipped++;
        }

    }

    var numT = 0;
    for (i = 0; i < arr.length; i++) {

        if (arr[i].type == "T") {
            numT++;

        var row = table.insertRow(numRow);
        numRow++;

        var comp = row.insertCell(0);
        var name = row.insertCell(1);
        var numP = row.insertCell(2);
        var len = row.insertCell(3);
        var loop = row.insertCell(4);

        var link = "Track " + numT;
        comp.innerHTML = link;

        name.innerHTML = arr[i].name;

        numP.innerHTML = arr[i].points;
        len.innerHTML = arr[i].len + "m";
        loop.innerHTML = arr[i].loop;

        } 

    }

    $("#pathTable").show(0);

}

