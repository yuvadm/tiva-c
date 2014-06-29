<!-- Copyright (c) 2013-2014 Texas Instruments Incorporated.  All rights reserved. -->

window.onload = function()
{
    document.getElementById('about').onclick = loadAbout;
    document.getElementById('overview').onclick = loadOverview;
    document.getElementById('block').onclick = loadBlock;
    document.getElementById('io_http').onclick = loadIOHttp;
}

function loadAbout()
{
    loadPage("about.htm");
    return false;
}

function loadOverview()
{
    loadPage("overview.htm");
    return false;
}

function loadBlock()
{
    loadPage("block.htm");
    return false;
}

function loadIOHttp()
{
    loadPage("io_http.htm");
    ledstateGet();
    speedGet();
    return false;
}

function SetFormDefaults()
{
    document.iocontrol.LEDOn.checked = ls;
    document.iocontrol.speed_percent.value = sp;
}

function toggle_led()
{
    var req = false;

    function ToggleComplete()
    {
        if(req.readyState == 4)
        {
            if(req.status == 200)
            {
                document.getElementById("ledstate").innerHTML = "<div>" +
                                                  req.responseText + "</div>";
            }
        }
    }

    if(window.XMLHttpRequest)
    {
        req = new XMLHttpRequest();
    }
    else if(window.ActiveXObject)
    {
        req = new ActiveXObject("Microsoft.XMLHTTP");
    }
    if(req)
    {
        req.open("GET", "/cgi-bin/toggle_led?id" + Math.random(), true);
        req.onreadystatechange = ToggleComplete;
        req.send(null);
    }
}

function speedSet()
{
    var req = false;
    var speed_txt = document.getElementById("speed_percent");
    function speedComplete()
    {
        if(req.readyState == 4)
        {
            if(req.status == 200)
            {
                document.getElementById("current_speed").innerHTML =
                                        "<div>" + req.responseText + "</div>";
            }
        }
    }
    if(window.XMLHttpRequest)
    {
        req = new XMLHttpRequest();
    }
    else if(window.ActiveXObject)
    {
        req = new ActiveXObject("Microsoft.XMLHTTP");
    }
    if(req)
    {
        req.open("GET", "/cgi-bin/set_speed?percent=" + speed_txt.value +
                        "&id" + Math.random(), true);
        req.onreadystatechange = speedComplete;
        req.send(null);
    }
}

function ledstateGet()
{
    var led = false;
    function ledComplete()
    {
        if(led.readyState == 4)
        {
            if(led.status == 200)
            {
                document.getElementById("ledstate").innerHTML = "<div>" +
                                                  led.responseText + "</div>";
            }
        }
    }

    if(window.XMLHttpRequest)
    {
        led = new XMLHttpRequest();
    }
        else if(window.ActiveXObject)
    {
        led = new ActiveXObject("Microsoft.XMLHTTP");
    }
    if(led)
    {
        led.open("GET", "/ledstate?id=" + Math.random(), true);
        led.onreadystatechange = ledComplete;
        led.send(null);
    }
}

function speedGet()
{
    var req = false;
    function speedReturned()
    {
        if(req.readyState == 4)
        {
            if(req.status == 200)
            {
                document.getElementById("current_speed").innerHTML = "<div>" + req.responseText + "</div>";
            }
        }
    }

    if(window.XMLHttpRequest)
    {
        req = new XMLHttpRequest();
    }
        else if(window.ActiveXObject)
    {
        req = new ActiveXObject("Microsoft.XMLHTTP");
    }
    if(req)
    {
        req.open("GET", "/get_speed?id=" + Math.random(), true);
        req.onreadystatechange = speedReturned;
        req.send(null);
    }
}

function loadPage(page)
{
    if(window.XMLHttpRequest)
    {
        xmlhttp = new XMLHttpRequest();
    }
    else
    {
        xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");
    }

    xmlhttp.open("GET", page, true);
    xmlhttp.setRequestHeader("Content-type",
                             "application/x-www-form-urlencoded");
    xmlhttp.send();

    xmlhttp.onreadystatechange = function ()
    {
        if((xmlhttp.readyState == 4) && (xmlhttp.status == 200))
        {
            document.getElementById("content").innerHTML = xmlhttp.responseText;
        }
    }
}
