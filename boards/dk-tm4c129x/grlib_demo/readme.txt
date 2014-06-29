Graphics Library Demonstration

This application provides a demonstration of the capabilities of the
TivaWare Graphics Library.  A series of panels show different features of
the library.  For each panel, the bottom provides a forward and back button
(when appropriate), along with a brief description of the contents of the
panel.

The first panel provides some introductory text and basic instructions for
operation of the application.

The second panel shows the available drawing primitives: lines, circles,
rectangles, strings, and images.

The third panel shows the canvas widget, which provides a general drawing
surface within the widget hierarchy.  A text, image, and application-drawn
canvas are displayed.

The fourth panel shows the check box widget, which provides a means of
toggling the state of an item.  Three check boxes are provided, with each
having a red ``LED'' to the right.  The state of the LED tracks the state
of the check box via an application callback.

The fifth panel shows the container widget, which provides a grouping
construct typically used for radio buttons.  Containers with a title, a
centered title, and no title are displayed.

The sixth panel shows the push button widget.  Two rows of push buttons
are provided; the appearance of each row is the same but the top row
does not utilize auto-repeat while the bottom row does. Each push button
has a red ``LED'' beneath it, which is toggled via an application callback
each time the push button is pressed. While holding down any of
auto-repeat buttons, the ``LED'' for that button should be toggled as long
as the button is being held down.

The seventh panel shows the radio button widget.  Two groups of radio
buttons are displayed, the first using text and the second using images for
the selection value.  Each radio button has a red ``LED'' to its right,
which tracks the selection state of the radio buttons via an application
callback.  Only one radio button from each group can be selected at a time,
though the radio buttons in each group operate independently.

The eighth and final panel shows the slider widget.  Six sliders
constructed using the various supported style options are shown.  The
slider value callback is used to update two widgets to reflect the values
reported by sliders.  A canvas widget near the top right of the display
tracks the value of the red and green image-based slider to its left and
the text of the grey slider on the left side of the panel is update to show
its own value.  The slider on the right is configured as an indicator
which tracks the state of the upper slider and ignores user input.

-------------------------------------------------------------------------------

Copyright (c) 2013-2014 Texas Instruments Incorporated.  All rights reserved.
Software License Agreement

Texas Instruments (TI) is supplying this software for use solely and
exclusively on TI's microcontroller products. The software is owned by
TI and/or its suppliers, and is protected under applicable copyright
laws. You may not combine this software with "viral" open-source
software in order to form a larger program.

THIS SOFTWARE IS PROVIDED "AS IS" AND WITH ALL FAULTS.
NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT
NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. TI SHALL NOT, UNDER ANY
CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
DAMAGES, FOR ANY REASON WHATSOEVER.

This is part of revision 2.1.0.12573 of the DK-TM4C129X Firmware Package.
