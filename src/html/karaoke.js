// This is HTML-unescaped logged in name; should only be used in JSON!
var loggedName = null;

// Currently opened tab
var currentTab = null;

// Handling back button
var backLetter = null;
var backToLetter = false;

// For confirmation dialog
var confirmDialogCallback = null;



// https://stackoverflow.com/questions/6234773/can-i-escape-html-special-chars-in-javascript
function escapeHtml(unsafe) {
    return unsafe
         .replace(/&/g, "&amp;")
         .replace(/</g, "&lt;")
         .replace(/>/g, "&gt;")
         .replace(/"/g, "&quot;")
         .replace(/'/g, "&#039;");
}

// AJAX runner
function runAPI( url, params, cfunc )
{
    var xhttp = new XMLHttpRequest();

    xhttp.onreadystatechange = function()
    {
        if ( xhttp.readyState == 4 )
        {
            if ( xhttp.status == 200)
            {
                if ( document.getElementById( "error" ) != null )
                    document.getElementById( "error" ).style.display = "none";
                
                cfunc(xhttp);
            }
            else
            {
                if ( document.getElementById( "error" ) != null )
                {
                    document.getElementById( "error" ).style.display = "block";
                    document.getElementById( "error" ).innerHTML = "Error sending request to the server";
                }
            }
        }
    };

    xhttp.open("POST", url, true);
    xhttp.setRequestHeader( "Content-Type", "application/json; charset=UTF-8" );
    xhttp.send( JSON.stringify( params ) );
}

// This function is called if a button is pressed in dialog. value is true if "yes" and false if "no"
function confirmDialogButton( value )
{
    // Hide it first
    document.getElementById( "confirmdialog" ).style.display = "none";
    
    if ( value )
        confirmDialogCallback();

    confirmDialogCallback = null;
}

// This function shows/popups the confirmation dialog 
function confirmDialog( text, callback )
{
    confirmDialogCallback = callback;

    document.getElementById( "confirmdialogtext" ).innerHTML = text;
    document.getElementById( "confirmdialog" ).style.display = "block";
}


// This callback is reused both for browse and search
function listSongs( xhttp )
{
    var obj = JSON.parse( xhttp.responseText );

    // Show the back link if this is browser
    if ( typeof obj["type"] != 'undefined' )
    {
        document.getElementById( "browseback" ).innerHTML = "Back to '" + backLetter + "'";
        document.getElementById( "browseback" ).style.display = "block";
        backToLetter = true;
    }
   
    if ( obj["results"] )
        obj = obj["results"]
       
    if ( obj.length > 0 )
    {
        var list = "";
        
        for ( var i = 0; i < obj.length; i++ )
        {
            list += "<div class='songentry' onclick='addsong( " + obj[i].id + ", \"" + obj[i].title + "\");'>"
                + "<div class='artist'>" + obj[i].artist + "</div>"
                + "<div class='title'>" + obj[i].title + "</div>"
                + "<div class='type'>" + obj[i].type + "</div>";
                
            if ( typeof obj[i].language != 'undefined' )
                list += "<div class='lang'>" + obj[i].language + "</div>";
            
            list += "</div>";
        }
        
        document.getElementById( currentTab + "data" ).innerHTML = list;
    }
    else
    {
        document.getElementById( currentTab + "data" ).innerHTML = "Nothing found";
    }
}

function search()
{
    runAPI( '/api/search', { query : document.getElementById("song").value }, listSongs );
}

// This is used in browser for letters or artists
function listArtists( xhttp )
{
    var obj = JSON.parse( xhttp.responseText );
    var list = "";
    
    if ( obj.results.length > 0 )
    {
        if ( obj.type == "initials" )
        {
            // This means we only received initials
            callback = "listArtists";
            document.getElementById( "browseback" ).style.display = "none";
        }
        else
        {
            callback = "listSongs";
        
            // This means we received artist names
            document.getElementById( "browseback" ).innerHTML = "Back";
            document.getElementById( "browseback" ).style.display = "block";
            backToLetter = false;
        }
        
        for ( var i = 0; i < obj.results.length; i++ )
        {
            list += "<div class=\"letter\" onClick=\"browseArtist( '" + obj.results[i] + "', " + callback + " );\">" + obj.results[i] + "</div>";
        }
        
        document.getElementById( currentTab + "data" ).innerHTML = list;
    }
    else
    {
        document.getElementById( currentTab + "data" ).innerHTML = "No such artist";
    }
}

function browseArtist( artist, callback )
{
    if ( artist.length == 1 )
        backLetter = artist;
    
    runAPI( '/api/browse', { artist: artist }, callback );
}        

function browse()
{
    backLetter = null;
    runAPI( '/api/browse', {}, listArtists );
}        

function browseback()
{
    if ( backToLetter == false )
        backLetter = null;
        
    if ( backLetter != null )
        runAPI( '/api/browse', { artist : backLetter }, listArtists );
    else
        browse();
}

function snackbar( message ) {
    // Get the snackbar DIV
    var x = document.getElementById("snackbar")

    // Add the "show" class to DIV
    x.innerHTML = message;
    x.className = "show";

    // After 3 seconds, remove the show class from DIV
    setTimeout(function(){ x.className = x.className.replace("show", ""); }, 3000);
} 

function addsongSucceed( xhttp )
{
    var data = JSON.parse( xhttp.responseText );

    if ( data["result"] == 1 )
    {
        snackbar( "<b>" + data["title"] + "</b> by <b>" + data["artist"] + "</b> is successfully enqueued" )
    }
    else
    {
        document.getElementById( "error" ).style.display = "block";
        document.getElementById( "error" ).innerHTML = "Failed to queue the song: " + data.reason;
    }

    document.getElementById("song").value = ""
}


function addsong( id, title )
{
    confirmDialog( "Queue the song \"" + title + "\" ?", function() { runAPI( '/api/addsong', { id : id }, addsongSucceed ); } );
}

// Proceeds the authentication status
function checkLoginResult( xhttp )
{
    var data = JSON.parse( xhttp.responseText );

    // Here we can have three situations:
    // 1. We are logged in successfully (data.loggedin is true)
    // 2. We tried to log in but challenge was required (data.source = "login" and data.challenge is generated
    // 3. We didn't try to log in
    if ( data.loggedin )
    {
        // We are logged in; redirect to main page if we are not there
        if ( data.source == "login" )
        {
            window.location = "index.html";
            return;
        }

        loggedName = data.name;
        document.getElementById("hello").innerHTML = "Hello, <b>" + escapeHtml( loggedName ) + "</b>!";
    }
    else if ( data.challenge )
    {
        // Is this re-entry of challenge?
        if ( typeof document.getElementById("code").challenge !== 'undefined' )
        {
            document.getElementById("code").classList.add("invalid");

            setTimeout(function(){ document.getElementById("code").classList.remove('invalid'); },500);
        }
        else
        {
            // This instance is secured, so the code needs to be entered. Store the code and show the challenge entry field
            document.getElementById("code").challenge = data.challenge;
            document.getElementById("entername").style.transition = "opacity 500ms";
            document.getElementById("entername").style.opacity = 0;
        
            // Hide the element fully once the transition is done, and show the access code element
            setTimeout( function() { 
                document.getElementById("entername").style.height = 0; 
                document.getElementById("entercode").style.display = 'block'; 
                }, 300 );
        }
    }
    else
    {
        // We are not logged in. Redirect to the login page if we're not there
        window.location = "login.html";
    }
}

// Remembers the user name so it could be logged in
// Check if we are logged in already; get the name if we are
function checkLogin()
{
    runAPI( '/api/auth/status', { source : "check" }, checkLoginResult );
}

function login()
{
    var params = { name : document.getElementById("name").value, source : "login" };
    
    var code = document.getElementById("code");
    
    if ( typeof code.challenge !== 'undefined' )
        params.code = md5( code.challenge + code.value );
    
    runAPI( '/api/auth/login', params, checkLoginResult );
}

// Logs the current user out
function logout()
{
    runAPI( '/api/auth/logout', {}, checkLoginResult );
}        


function w3_open()
{
    document.getElementById("mySidebar").style.display = "block";
    document.getElementById("myOverlay").style.display = "block";
}

function w3_close()
{
    document.getElementById("mySidebar").style.display = "none";
    document.getElementById("myOverlay").style.display = "none";
}

// Opens a specific tab activity (either on click or via menu)
function openTab(activity)
{
    var i;
    var x = document.getElementsByClassName("activity");

    for ( var i = 0; i < x.length; i++)
    {
       x[i].style.display = "none";
    }
    
    x = document.getElementsByClassName("menuentry");
    for ( var i = 0; i < x.length; i++) {
       x[i].className = x[i].className.replace(" w3-red", "");
    }

    document.getElementById(activity).style.display = "block";
    document.getElementById("menu_" + activity).className += " w3-red";
    
    currentTab = activity;
}

// Handles the response and updates the singer queue list
function receivedSingerQueue( xhttp )
{
    var obj = JSON.parse( xhttp.responseText );
    var list = "";

    for ( var i = 0; i < obj.length; i++ )
    {
        if ( typeof obj[i].removable !== 'undefined' )
            list += "<div class='queueentry-ours'><a href='#' onclick='removeQueue(" + obj[i].id + ",\"" + obj[i].title + "\")'><i class='fa fa-remove'></i></a>";
        else
            list += "<div class='queueentry'>";
        
        list += "<div class='qnumber'>" + (i + 1) + "</div>"
            + "<div class='qsinger'>" + obj[i].singer + "</div>"
            + "<div class='qtitle'>" + obj[i].title + "</div>"
            + "</div>";
    }
    
    if ( list === "" )
        list = "Nobody added a song :(";
        
    document.getElementById( "queuedata" ).innerHTML = list;
    
    if ( currentTab === 'queue' )
        setTimeout( function() { updateSingerQueue() }, 2000 );
}

// Removes a song from the singer queue
function removeQueue( id, song )
{
    confirmDialog( "Remove \"" + song + "\" from the queue?", function() { runAPI( '/api/queue/remove', { id : id }, receivedSingerQueue ); } );
}

// Initiates the update
function updateSingerQueue()
{
    runAPI( '/api/queue/list', {}, receivedSingerQueue );
}


// Called when sound control info has been received from Web server
function soundControlUpdateReceived( xhttp )
{
    var obj = JSON.parse( xhttp.responseText );
    
    if ( obj.state == "playing" || obj.state == "paused" )
    {
        // Change the playpause button to "play" if paused. Otherwise to 'paused
        if ( obj.state == "paused" )
        {
            document.getElementById( "soundstatus" ).innerHTML = "Paused: " + obj.song;
            document.getElementById( "btnplaypause" ).innerHTML = '<i class="fa fa-play"></i>';
        }
        else
        {
            document.getElementById( "btnplaypause" ).innerHTML = '<i class="fa fa-pause"></i>';
            document.getElementById( "soundstatus" ).innerHTML = "Playing: " + obj.song;
        }
    
        document.getElementById( "controlstatus" ).innerHTML = obj.pos + " | " + obj.duration;

        // Set the values
        document.getElementById( "value_volume" ).innerHTML = obj.volume + "%";
        
        if ( obj.delay == 0 )
            document.getElementById( "value_delay" ).innerHTML = "None";
        else
            document.getElementById( "value_delay" ).innerHTML = (obj.delay / 1000) + "s";

        // Pitch is 0-100; 50 is normal with step 5
        if ( obj.pitch === 'disabled' )
            document.getElementById( "value_pitch" ).innerHTML = "---";
        else
            document.getElementById( "value_pitch" ).innerHTML = (obj.pitch - 50) / 5;

        if ( obj.tempo === 'disabled' )
            document.getElementById( "value_tempo" ).innerHTML = "---";
        else
            document.getElementById( "value_tempo" ).innerHTML = obj.tempo + "%";
    }
    else
    {
        // We are stopped
        document.getElementById( "controlstatus" ).innerHTML = "Play controls:";
        document.getElementById( "value_volume" ).innerHTML = "---";
        document.getElementById( "value_delay" ).innerHTML = "---";
        document.getElementById( "value_pitch" ).innerHTML = "---";
        document.getElementById( "value_tempo" ).innerHTML = "---";
        
        document.getElementById( "btnplaypause" ).innerHTML = '<i class="fa fa-play"></i>';
        document.getElementById( "soundstatus" ).innerHTML = "Player stopped";
    }

    if ( currentTab === 'control' )
        setTimeout( function() { soundControlUpdate() }, 500 );
}

function soundControlUpdate()
{
    runAPI( '/api/control/status', {}, soundControlUpdateReceived );
}

// Is called when media control buttons (play, pause, stop etc) are clicked
function controlAction( type )
{
    runAPI( '/api/control/action', { 'a' : type }, soundControlUpdateReceived );
}

// Is called when a control such as volume, pitch etc gets changed
function controlAdjust( type, val )
{
    runAPI( '/api/control/adjust', { 'a' : type, 'v' : val  }, soundControlUpdateReceived );
}
