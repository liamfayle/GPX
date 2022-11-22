'use strict'


//DB
const Database = require('better-sqlite3');

// C library API
const ffi = require('ffi-napi');

// Express App (Routes)
const express = require("express");
const app     = express();
const path    = require("path");
const fileUpload = require('express-fileupload');
const bp = require('body-parser');



app.use(fileUpload());
app.use(express.static(path.join(__dirname+'/uploads')));

// Minimization
const fs = require('fs');
const JavaScriptObfuscator = require('javascript-obfuscator');

// Important, pass in port as in `npm run dev 1234`, do not change
const portNum = process.argv[2];

// Send HTML at root, do not change
app.get('/',function(req,res){
  res.sendFile(path.join(__dirname+'/public/index.html'));
});

// Send Style, do not change
app.get('/style.css',function(req,res){
  //Feel free to change the contents of style.css to prettify your Web app
  res.sendFile(path.join(__dirname+'/public/style.css'));
});

// Send obfuscated JS, do not change
app.get('/index.js',function(req,res){
  fs.readFile(path.join(__dirname+'/public/index.js'), 'utf8', function(err, contents) {
    const minimizedContents = JavaScriptObfuscator.obfuscate(contents, {compact: true, controlFlowFlattening: true});
    res.contentType('application/javascript');
    res.send(minimizedContents._obfuscatedCode);
  });
});

app.get('/failed',function(req,res){
  res.send("failed");
});

function checkFileName(name) {
  if (fs.existsSync("uploads/" + name)) {
    return true;
  } else {
    return false;
  }
}

//Respond to GET requests for files in the uploads/ directory
app.get('/uploads/:name', function(req , res){
  fs.stat('uploads/' + req.params.name, function(err, stat) {
    if(err == null) {
      res.sendFile(path.join(__dirname+'/uploads/' + req.params.name));
    } else {
      console.log('Error in file downloading route: '+err);
      res.send('');
    }
  });
});

//******************** Your code goes here ******************** 
//setup shared lib
let lib = ffi.Library('./libgpxparser.so', {
  'getValidLogData': ['string', ['string', 'string']],
  'getValidViewData': ['string', ['string', 'string']],
  'getValidRouteOtherData': ['string', ['string', 'string', 'int']],
  'getValidTrackOtherData': ['string', ['string', 'string', 'int']],
  'updateRouteName': ['string', ['string', 'string', 'string', 'int']],
  'updateTrackName': ['int', ['string', 'string', 'string', 'int']],
  'isValidDoc': ['bool', ['string', 'string']],
  'createDocForm': ['bool', ['string', 'double', 'string']],
  'addRouteForm': ['bool', ['string', 'string', 'string']],
  'addWptRoute': ['bool', ['string', 'string', 'string', 'double', 'double']],
  'getPathBetween': ['string', ['string', 'double', 'double', 'double', 'double', 'double']],
  'jsonRoutes': ['string', ['string', 'string']],
  'routeToWaypoint': ['string', ['string', 'string', 'int', 'int']],
});


async function uploadSelected(file) {
  await file.mv('uploads/' + file.name, function(err) {
    if (err) {
      console.log(err);
    }
  });


}


//Respond to POST requests that upload files to uploads/ directory
app.post('/upload', function(req, res) {
  if(!req.files) {
    return res.send("");
  }
 
  let uploadFile = req.files.uploadFile;

  //check if file already exists
  let returned = checkFileName(uploadFile.name);
  if (returned) {
    return res.send("<script>alert('Error: File with that name already exists.')</script>");
  }

  if (path.extname(uploadFile.name) != ".gpx") {
    return res.send("");
  }
 
  // Use the mv() method to place the file somewhere on your server
  uploadSelected(uploadFile);

  //var isValid = lib.isValidDoc("uploads/" + uploadFile.name, "parser/gpx.xsd");


  return res.send("<script>alert('Uploaded File \"" + uploadFile.name + "\" Sucessfully.');</script>");

});


app.get('/getPaths/:sLat/:sLon/:eLat/:eLon/:tol/:validFiles', function(req, res) {
  
  var paths = "[";

  var files = req.params.validFiles.split(",");

  var i;
  for (i = 0; i < files.length; i++) {

    var temp = lib.getPathBetween("uploads/" + files[i], req.params.sLat, req.params.sLon, req.params.eLat, req.params.eLon, req.params.tol);
    if (temp != "*"){
      paths = paths + temp + ",";
    }
  }

  paths = paths.slice(0, -1);
  paths = paths + "]";

  //console.log(paths);
  res.send(paths);

});


app.post('/createGpx', function(req, res) {

  if (req.body.gpxName.length == 0) {
    return res.send("");
  }

  if (req.body.gpxCre.length == 0) {
    return res.send("");
  }

  if (req.body.gpxVer.length == 0) {
    return res.send("");
  }

  if (isNaN(req.body.gpxVer)) {
    return res.send("");
  }
  
  let returned = checkFileName(req.body.gpxName);
  if (returned) {
    return res.send("<script>alert('Error: File with that name already exists.')</script>");
  }

  if (path.extname(req.body.gpxName) != ".gpx") {
    return res.send("");
  }

  if (req.body.gpxName.indexOf("/") > -1) {
    return res.send("");
  }

  let s = lib.createDocForm("uploads/" + req.body.gpxName, req.body.gpxVer, req.body.gpxCre);

  if (s == true) {
    console.log("created gpx file");
    return res.send("<script>alert('Successfully created gpx file \"" + req.body.gpxName + "\".');</script>");
  }
 
  console.log("failed to create gpx file");
  return res.send("<script>alert('Failed to create gpx file (invalid parameters).');</script>");
  

});


//get names of files in uploads folder
app.get('/uploads', function(req , res){
  fs.readdir('uploads/', function(err, items) {
    if(err != null) {
      console.log('Error getting file names or no .gpx files exist');
      return res.send('');
    } else {
      items.forEach(function(file, index, object) {
        if (path.extname(file) != ".gpx") {
          object.splice(index, index+1);
        }
      });
      if (items.length == 1) {
        if (path.extname(items[0]) != ".gpx") {
          items.pop();
        }
      }

      //try to validate and remove from uploads if fail
      items.forEach(function(file, index, object) {
        let s = lib.getValidLogData("uploads/" + file, "parser/gpx.xsd");
        object[index] = s;
        if (s == "failed") {
          fs.unlink("uploads/" + file, (err) => {
            if (err) {
              return;
            }
          })
          //return res.send("<script>alert('The uploaded file was not valid gpx (removed from server).')</script>");
        }
      });

      console.log('Sent log table data succesfully.');
      res.send(items);
    }
  });
});


app.get('/updateForm/:file', function(req , res){
  var filename = "uploads/" + req.params.file;
  let s = lib.getValidViewData(filename, "parser/gpx.xsd");
  console.log('Sent view table data succesfully.');
  res.send(s);
});


app.get('/updateOtherRouteData/:file/:num', function(req , res){
  let s = lib.getValidRouteOtherData("uploads/" + req.params.file, "parser/gpx.xsd", req.params.num);
  console.log("Send other gpx data successfully.");
  res.send(s);
});

app.get('/updateOtherTrackData/:file/:num', function(req , res){
  let s = lib.getValidTrackOtherData("uploads/" + req.params.file, "parser/gpx.xsd", req.params.num);
  console.log("Send other gpx data successfully.");
  res.send(s);
});


app.post('/updateTrackName/:file/:num', function(req , res){
  let s = lib.updateTrackName("uploads/" + req.params.file, "parser/gpx.xsd", req.body.name, req.params.num);
  if (s == -1) {
    console.log("failed to update track name");
    res.send("<script>alert('Failed to update track name.')</script>");
  } else {
    console.log("success to update track name");
    res.send("<script>alert('Track name updated.')</script>");
  }
});


//DATABSE STUFF
const validUser = "admin";
const validPass = "password";
const databaseName = "gpxDB";

function loginManager(username, password, name) {
  if (username != validUser || password != validPass || name != databaseName) {
    throw "Invalid Login";
  }
}



app.use(bp.json());
app.use(bp.urlencoded({ extended: true }));

var user;
var pass;
var name;
var auth;
var db = null;

app.get('/loginDb/:user/:pass/:name', async function(req , res){

  let db;
  var data = new Array(4);
  try {
    //create connection
    loginManager(req.params.user, req.params.pass, req.params.name);

    //set logged in vals so dont need to keep asking
    user = req.params.user;
    pass = req.params.pass;
    name = req.params.name;
    auth = true;
    data[0] = user;

    db = new Database(name + '.db', {});

    //setup tables if they do not exist
    db.prepare("create table if not exists FILE (gpx_id INTEGER NOT NULL PRIMARY KEY, file_name varchar(60) not null, ver decimal(2,1) not null, creator varchar(256) not null )").run();
    db.prepare("create table if not exists ROUTE (route_id INTEGER NOT NULL PRIMARY KEY, route_name varchar(256), route_len float(15,7) not null, gpx_id int not null, foreign key(gpx_id) references FILE(gpx_id) on delete cascade on update cascade )").run();
    db.prepare("create table if not exists POINT (point_id INTEGER NOT NULL PRIMARY KEY, point_index int not null, latitude decimal(11,7) not null, longitude decimal(11,7) not null, point_name varchar(256), route_id int not null, foreign key(route_id) references ROUTE(route_id) on delete cascade on update cascade )").run();

    //get row counts
    data[1] = db.prepare("select * from `FILE`").all().length;
    data[2] = db.prepare("select * from `ROUTE`").all().length;
    data[3] = db.prepare("select * from `POINT`").all().length;

    return res.send(data);
  } catch(e) {
    console.log("Query error: "+e);
    return res.send("Failed to login.");
  } finally {
    db.close();
  }
});


app.get('/auth', function(req , res){
  if (auth == false || auth === undefined) {
    return res.send("out");
  } else {
    return res.send(user);
  }
});


app.get('/dbStatus', async function(req , res){
  let db;
  var data = new Array(3);
  try {
    //create connection
    db = new Database(name + '.db', {});

    //get row counts
    data[0] = db.prepare("select * from `FILE`").all().length;
    data[1] = db.prepare("select * from `ROUTE`").all().length;
    data[2] = db.prepare("select * from `POINT`").all().length;

    return res.send(data);
  } catch(e) {
    console.log("Query error: "+e);
    return res.send("fail");
  } finally {
    db.close();
  }
});


app.get('/clearData', async function(req , res){
  let db;
  var data = new Array(3);
  try {
    //create connection
    db = new Database(name + '.db', {});

    //delete data
    db.prepare("delete from `FILE`").run();
    db.prepare("delete from `ROUTE`").run();
    db.prepare("delete from `POINT`").run();

    try {
      db.prepare("update sqlite_sequence set seq=1 where name='FILE'").run();
      db.prepare("update sqlite_sequence set seq=1 where name='ROUTE'").run();
      db.prepare("update sqlite_sequence set seq=1 where name='POINT'").run();
    } catch (e) {
      console.log("no sqlitesequnce table");
    }

    //get row counts
    data[0] = db.prepare("select * from `FILE`").all().length;
    data[1] = db.prepare("select * from `ROUTE`").all().length;
    data[2] = db.prepare("select * from `POINT`").all().length;

    return res.send(data);
  } catch(e) {
    console.log("Query error: "+e);
    return res.send("fail");
  } finally {
    db.close();
  }
});


app.post('/storeFiles', async function(req , res){

  var files = req.body.files;

  let db;
  var data = new Array(3);
  try {
    
    //create connection
    db = new Database(name + '.db', {});

    //store files
    var i;
    for (i = 0; i < files.length; i++) {

      let s = lib.getValidLogData("uploads/" + files[i], "parser/gpx.xsd");
      var info = JSON.parse(s);

      var rows = db.prepare("select * from `FILE`").all();
      
      var k;
      var check = false;
      for (k = 0; k < rows.length; k++) {
        if (rows[k].file_name == files[i]) {
          check = true;
          break;
        }
      }

      if (check == false) {
        db.prepare("insert into FILE (file_name, ver, creator) values ('" + files[i] + "', '" + info['version'] + "', '" + info['creator'] + "')").run();

        let r = lib.jsonRoutes("uploads/" + files[i], "parser/gpx.xsd");
        info = JSON.parse(r);
        
        var j;
        for (j = 0; j < info.length; j++) {

          var w;
          
          if (info[j]['name'] == "None") {
            w = lib.routeToWaypoint("uploads/" + files[i], "", info[j]['len'], j);
            info[j]['name'] = "NULL";
          } else {
            w = lib.routeToWaypoint("uploads/" + files[i], info[j]['name'], info[j]['len'], j);
          }

          console.log(w);

          rows = db.prepare("select * from `FILE`").all();
          var id;
          for (k = 0; k < rows.length; k++) {
            if (rows[k].file_name == files[i]) {
              id = rows[k].gpx_id;
              break;
            }
          }

          var a = db.prepare("insert into ROUTE (route_name, route_len, gpx_id) values ('" + info[j]['name'] + "', '" + info[j]['len'] + "', '" + id + "')").run();
          
          
          var wpt = JSON.parse(w);
          var z;

          var idR = a['lastInsertRowid'];


          for (z = 0; z < wpt.length; z++) {

            if (wpt[z].name == "") {
              wpt[z].name = "NULL";
            }

            db.prepare("insert into POINT (point_index, latitude, longitude, point_name, route_id) values ('" + wpt[z].index + "', '" + wpt[z].lat + "', '" + wpt[z].lon + "', '" + wpt[z].name + "', '" + idR + "')").run();

          }

        }

      }

    }

    //get row counts
    data[0] = db.prepare("select * from `FILE`").all().length;
    data[1] = db.prepare("select * from `ROUTE`").all().length;
    data[2] = db.prepare("select * from `POINT`").all().length;

    return res.send(data);
  } catch(e) {
    console.log("Query error: "+e);
    return res.send("fail");
  } finally {
    db.close();
  }
});


app.get('/logout', function(req , res){
  auth = false;
  user = "";
  pass = "";
  name = "";
  return res.send("good");
});


app.post('/updateRouteName/:file/:num', async function(req , res){
  let s = lib.updateRouteName("uploads/" + req.params.file, "parser/gpx.xsd", req.body.name, req.params.num);
  var file = req.params.file;
  var nameNew = req.body.name;
  if (s == "-1") {
    console.log("failed to update route name");
    res.send("<script>alert('Failed to update route name.')</script>");
  } else if (auth == true) {
    console.log("successfully update route name");
    //^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
  
    let db;
    var data = new Array(3);
    try {
      //create connection

      if (!auth) {
        res.send("<script>alert('Route name updated | Database logged out and will not be updated.');</script>");
      }

      db = new Database(name + '.db', {});

      var k;
      var rows = db.prepare("select * from `FILE`").all();
      var id = -1;
      for (k = 0; k < rows.length; k++) {
        if (rows[k].file_name == file) {
          id = rows[k].gpx_id;
          break;
        }
      }

      if (id != -1) {
        db.prepare("update ROUTE set route_name = '" + nameNew + "' where gpx_id = '" + id + "' and route_name = '" + s + "'").run();
      }

      //get row counts
      var rows = db.prepare("select * from `FILE`").all();
      data[0] = rows.length;
      rows = db.prepare("select * from `ROUTE`").all();
      data[1] = rows.length;
      rows = db.prepare("select * from `POINT`").all();
      data[2] = rows.length;

      if (id != -1) {
        var script = "<script>alert('Route name updated | Database has " + data[0] + " Files, " + data[1] + " Routes, " + data[2] + " Points.');</script>";
      } else {
        var script = "<script>alert('Route name updated | Database was not updated (File in which route name was changed did not exist in table FILE)');</script>";
      }
      
      res.send(script);
    } catch(e) {
      console.log("Query error: "+e);
    } finally {
      db.close();
    }
    //^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
  } else {
    res.send("<script>alert('Route name updated.');</script>");
  }
});

app.post('/addRoute', async function(req, res) {


  var numWpt = req.body.num;
  var filename = "uploads/" + req.body.filename;

  if (req.body.filename == "***") {
    return res.send("");
  }


  if (req.body.wptLat.length != (numWpt) && numWpt != "***") {
    return res.send("");
  }

  if (req.body.wptLon.length != (numWpt) && numWpt != "***") {
    return res.send("");
  }


  let r = await lib.addRouteForm(filename, "parser/gpx.xsd", req.body.routeName);

  if (r == false) {
    return res.send("<script>alert('[Route validation error] Failed to add route');</script>");
  }

  var i;
  for (i = 1; i < numWpt; i++) {


    if (req.body.wptName[i-1] == "..//..//") {
      req.body.wptName[i-1] = "";
    }

    if (req.body.wptLat[i-1] > 90 || req.body.wptLat[i-1] < -90) {
      return res.send("");
    }

    if (req.body.wptLon[i-1] >= 180 || req.body.wptLon[i-1] < -180) {
      return res.send("");
    }

    let w = await lib.addWptRoute(filename, "parser/gpx.xsd", req.body.wptName[i-1], req.body.wptLat[i-1], req.body.wptLon[i-1]);

    if (w == false) {
      return res.send("<script>alert('[Waypoint validation error] Failed to add waypoint to route.');</script>");
    }

  }

  //^^^^^^^^^^^^

  let x = await lib.jsonRoutes(filename, "parser/gpx.xsd");
  let dataArr = JSON.parse(x);

  let ourR = dataArr[dataArr.length - 1];

  let rtoW = await lib.routeToWaypoint(filename, ourR['name'], ourR['len'], -1);
  let pts = JSON.parse(rtoW);

  let db;
  var data = new Array(3);
  try {
    if (!auth) {
      res.send("<script>alert('Successfully added route. | Database logged out and will not be updated.');</script>");
    }
    //create connection
    db = new Database(name + '.db', {});

    //add

    var k;
    var rows = db.prepare("select * from `FILE`").all();
    var id = -1;
    for (k = 0; k < rows.length; k++) {
      if (rows[k].file_name == req.body.filename) {
        id = rows[k].gpx_id;
        break;
      }
    }

    if (id != -1) {

      if (ourR['name'] == "None") {
        ourR['name'] = "NULL";
      }

      var a = db.prepare("insert into ROUTE (route_name, route_len, gpx_id) values ('" + ourR['name'] + "', '" + ourR['len'] + "', '" + id + "')").run();

      var idR = a['lastInsertRowid'];
      
      var z;
      for (z = 0; z < pts.length; z++) {

        if (pts[z].name == "") {
          pts[z].name = "NULL";
        }

        db.prepare("insert into POINT (point_index, latitude, longitude, point_name, route_id) values ('" + pts[z].index + "', '" + pts[z].lat + "', '" + pts[z].lon + "', '" + pts[z].name + "', '" + idR + "')").run();

      }

    }

    //get row counts
    var rows =  db.prepare("select * from `FILE`").all();
    data[0] = rows.length;
    rows = db.prepare("select * from `ROUTE`").all();
    data[1] = rows.length;
    rows = db.prepare("select * from `POINT`").all();
    data[2] = rows.length;

    if (id != -1) {
      var script = "<script>alert('Successfully added route. | Database has " + data[0] + " Files, " + data[1] + " Routes, " + data[2] + " Points.');</script>";
    } else {
      var script = "<script>alert('Successfully added route. | Database was not updated (File which route was added to did not exist in table FILE)');</script>";
    }
    
    res.send(script);
  } catch(e) {
    console.log("Query error: "+e);
    //return res.send("fail");
  } finally {
    db.close();
  }

  //^^^^^^^^^^^

});


app.get('/o1/:sort', async function(req, res) {

  var sort = req.params.sort;

  let db;
  var data = new Array(3);
  try {
    //create connection
    db = new Database(name + '.db', {});
    var rows;

    if (sort == "len") {
      rows = db.prepare("select * from `ROUTE` order by `route_len`").all();
    } else {
      rows = db.prepare("select * from `ROUTE` order by `route_name`").all();
      rows.sort(function(a,b) {
        if (a.route_name == "NULL" && b.route_name != "NULL") {
          return 1;
        }
        if (a.route_name != "NULL" && b.route_name == "NULL") {
          return -1;
        }
        if (a.route_name < b.route_name) {
          return -1;
        }
        if (a.route_name > b.route_name) {
          return 1;
        }
        return 0;
      })
    }

    res.send(rows);
  } catch(e) {
    console.log("Query error: "+e);
    return res.send("fail");
  } finally {
    db.close();
  }

});


app.get('/o2/:sort/:file', async function(req, res) {

  var sort = req.params.sort;
  var file = req.params.file;

  let db;
  var data = new Array(3);
  try {
    //create connection
    db = new Database(name + '.db', {});

    var k;
    var rows = db.prepare("select * from `FILE`").all();
    var id = -1;
    for (k = 0; k < rows.length; k++) {
      if (rows[k].file_name == file) {
        id = rows[k].gpx_id;
        break;
      }
    }

    if (id != -1) {
      var rows;
      if (sort == "len") {
        rows = db.prepare("select * from `ROUTE` where `gpx_id` = '" + id + "' order by `route_len`").all();
      } else {
        rows =  db.prepare("select * from `ROUTE` where `gpx_id` = '" + id + "' order by `route_name`").all();
        rows.sort(function(a,b) {
          if (a.route_name == "NULL" && b.route_name != "NULL") {
            return 1;
          }
          if (a.route_name != "NULL" && b.route_name == "NULL") {
            return -1;
          }
          if (a.route_name < b.route_name) {
            return -1;
          }
          if (a.route_name > b.route_name) {
            return 1;
          }
          return 0;
        })
      }
      res.send(rows);
    } else {
      return res.send("fail");
    }
  } catch(e) {
    console.log("Query error: "+e);
    return res.send("fail");
  } finally {
    db.close();
  }

});


app.get('/o3/:file/:num', async function(req, res) {

  var sort = req.params.sort;
  var file = req.params.file;
  var num = req.params.num;

  let db;
  var data = new Array(3);
  try {
    //create connection
    db = new Database(name + '.db', {});

    var k;
    var rows =  db.prepare("select * from `FILE`").all();
    var id = -1;
    for (k = 0; k < rows.length; k++) {
      if (rows[k].file_name == file) {
        id = rows[k].gpx_id;
        break;
      }
    }

    if (id != -1) {
      var rId = -1;
      var i;
      var rows =  db.prepare("select * from `ROUTE` where `gpx_id` = '" + id + "'").all();
      for (i = 0; i < rows.length; i++) {
        if (i == num) {
          rId = rows[i].route_id;
          break;
        }
      }
      if (rId != -1) {
        var rows =  db.prepare("select * from `POINT` where `route_id` = '" + rId + "' order by `point_index`").all();
        res.send(rows);
      } else {
        return res.send("fail");
      }
    } else {
      return res.send("fail");
    }
  } catch(e) {
    console.log("Query error: "+e);
    return res.send("fail");
  } finally {
    db.close();
  }

});


app.get('/o4/:sort/:file', async function(req, res) {

  var sort = req.params.sort;
  var file = req.params.file;


  let db;
  try {
    //create connection
    db = new Database(name + '.db', {});

    var k;
    var rows = db.prepare("select * from `FILE`").all();
    var id = -1;
    for (k = 0; k < rows.length; k++) {
      if (rows[k].file_name == file) {
        id = rows[k].gpx_id;
        break;
      }
    }

    if (id != -1) {
      
      if (sort == "name") {
        var rows = db.prepare("select * from `ROUTE` where `gpx_id` = '" + id + "' order by `route_name`").all();
        rows.sort(function(a,b) {
          if (a.route_name == "NULL" && b.route_name != "NULL") {
            return 1;
          }
          if (a.route_name != "NULL" && b.route_name == "NULL") {
            return -1;
          }
          if (a.route_name < b.route_name) {
            return -1;
          }
          if (a.route_name > b.route_name) {
            return 1;
          }
          return 0;
        })
      } else {
        var rows = db.prepare("select * from `ROUTE` where `gpx_id` = '" + id + "' order by `route_len`").all();
      }

      var data = rows;
      var info = new Array();
      var i;
      for (i = 0; i < data.length; i++) {
        var rows = db.prepare("select * from `POINT` where `route_id` = '" + data[i].route_id + "' order by `point_index`").all();
        info.push(rows);
      }

      res.send({info:info, name:data});

    } else {
      return res.send("fail");
    }
  } catch(e) {
    console.log("Query error: "+e);
    return res.send("fail");
  } finally {
    db.close();
  }

});


app.get('/o5/:sort/:file/:num/:final', async function(req, res) {

  var sort = req.params.sort;
  var file = req.params.file;
  var final = req.params.final;
  var num = req.params.num;

  let db;
  try {
    //create connection
    db = new Database(name + '.db', {});

    var k;
    var rows =  db.prepare("select * from `FILE`").all();
    var id = -1;
    for (k = 0; k < rows.length; k++) {
      if (rows[k].file_name == file) {
        id = rows[k].gpx_id;
        break;
      }
    }

    if (id != -1) {
      
      var data = new Array();
      var rows =  db.prepare("select * from `ROUTE` where `gpx_id` = '" + id + "' order by `route_len`").all();
      if (sort == "long") {
        var i;
        var count = 0;
        for (i = rows.length - 1; i>=0; i--) {
            if (count == num) {
              break;
            }
            data.push(rows[i]);
            count++;
        }
      } else {
        var i;
        var count = 0;
        for (i = 0; i < rows.length; i++) {
            if (count == num) {
              break;
            }
            data.push(rows[i]);
            count++;
        }
      }

      if (final == "name") {
        data.sort(function(a,b) {
          if (a.route_name < b.route_name) {
            return -1;
          }
          if (a.route_name > b.route_name) {
            return 1;
          }
          return 0;
        })
      } else {
        data.sort(function(a,b) {
          if (a.route_len < b.route_len) {
            return 1;
          }
          if (a.route_len > b.route_len) {
            return -1;
          }
          return 0;
        })
      }

      return res.send(data);
    } else {
      return res.send("fail");
    }
  } catch(e) {
    console.log("Query error: "+e);
    return res.send("fail");
  } finally {
    db.close();
  }

});

app.listen(portNum);
console.log('Running app at localhost: ' + portNum);
