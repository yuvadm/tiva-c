<!-- Copyright (c) 2013-2014 Texas Instruments Incorporated.  All rights reserved. -->

window.onload = function()
{
    document.getElementById('about').onclick = loadAbout;
    document.getElementById('overview').onclick = loadOverview;
    document.getElementById('block').onclick = loadBlock;
    document.getElementById('io_http').onclick = loadIOHttp;

    loadPage("about.htm");
}
