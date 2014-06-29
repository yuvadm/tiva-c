//*****************************************************************************
//
// slidemenuwidget.c - A sliding menu drawing widget.
//
// Copyright (c) 2011-2014 Texas Instruments Incorporated.  All rights reserved.
// Software License Agreement
// 
// Texas Instruments (TI) is supplying this software for use solely and
// exclusively on TI's microcontroller products. The software is owned by
// TI and/or its suppliers, and is protected under applicable copyright
// laws. You may not combine this software with "viral" open-source
// software in order to form a larger program.
// 
// THIS SOFTWARE IS PROVIDED "AS IS" AND WITH ALL FAULTS.
// NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT
// NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. TI SHALL NOT, UNDER ANY
// CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
// DAMAGES, FOR ANY REASON WHATSOEVER.
// 
// This is part of revision 2.1.0.12573 of the DK-TM4C123G Firmware Package.
//
//*****************************************************************************

#include <stdbool.h>
#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "utils/uartstdio.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "slidemenuwidget.h"

//*****************************************************************************
//
//! \addtogroup slidemenuwidget_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
// This is a custom widget for drawing a menu system on the display.  The
// widget presents the menus using a "sliding" animation.  The menu items
// are shown in a vertical list, and as the user scrolls through the list
// of menu items, the menu slides up and down the display.  When a menu item
// is selected to descend in the menu tree, the widget slides the old menu
// off the to left while the new menu slides in from the right.  Likewise,
// going up in the menu tree, the higher level menu slides back onto the
// screen from the left.
//
// Additional structures are provided to implement a menu, and menu items.
// Each menu contains menu items, and each menu item can have a child menu.
// These structures can be used to build a menu tree.  The menu widget will
// show one menu at any given time, the menu that is displayed on the screen.
//
// In addition to child menus, any menu item can have instead a child widget.
// If this is used, then when the user selects a menu item, a new widget can
// be activated to perform some function.  When the function of the child
// widget completes, then the widget slides back off the screen (to the right)
// and the parent menu is displayed again.
//
// A given menu can have menu items that are individually selectable or
// multiple-selectable.  For individually selectable menu items, the item
// is selected by leaving the menu with the focus on the selected item.  For
// example navigating down to a submenu with choices A, B and C, and then
// navigating until the focus is on item B will cause item B to be selected.
// The menu will remember that item B was selected even when navigating away
// from that menu.
//
// If a menu is configured to be multiple-selectable, then each menu item has
// a check box that is checked by pressing the select button.  When the item
// is selected the box will show an X.  Any or all or none can be selected in
// this way.  When a menu is configured to be multi-selectable, the menu items
// cannot have any child menus or widgets.
//
// The menu widget provides some visual clues to the user about how to
// navigate the menu tree.  Whenever a menu item has a child menu or child
// widget, then a small right arrow is shown on the right side of the menu
// item that has the focus.  This tells the user to press the "right" button
// to descend to the next menu or widget.  When it is possible to go up a
// level in the menu tree (when showing a child menu), a small left arrow
// will be shown on the menu item with the focus.  This is an indication to the
// user that they should press the "left" button.
//
// This widget is meant to work with key/button presses.  It expects there
// to be up/down/left/right and select buttons.  The widget will need to be
// modified in order to work with a pointer input.
//
// In order to perform the sliding animation, the menu widget requires that
// it be provided with two off-screen displays.  The menu widget renders the
// two menus (the old and the new) into the two buffers, and then repeatedly
// paints both to the physical display while adjusting the coordinates as
// appropriate.  This will cause the menus to appear animated and move across
// the display.  When the menus are being animated, the menu widget is taking
// all the non-interrupt processor time in order to draw the buffers to the
// display.  This operation occurs in response to the widget processing of the
// key/button events and will occur in the thread that calls
// WidgetMessageQueueProcess().  The programmer should be aware of this
// processing burden when designing an application that uses the sliding
// menu widget.
//
//*****************************************************************************

//*****************************************************************************
//
// A graphics image of a small right arrow icon.
//
//*****************************************************************************
const uint8_t g_ui8RtArrow[] =
{
    IMAGE_FMT_1BPP_UNCOMP,
    4, 0,
    8, 0,

    0x80,
    0xC0,
    0xE0,
    0xF0,
    0xE0,
    0xC0,
    0x80,
    0
};

//*****************************************************************************
//
// A graphics image of a small left arrow icon.
//
//*****************************************************************************
const uint8_t g_ui8LtArrow[] =
{
    IMAGE_FMT_1BPP_UNCOMP,
    4, 0,
    8, 0,

    0x10,
    0x30,
    0x70,
    0xF0,
    0x70,
    0x30,
    0x10,
    0
};

//*****************************************************************************
//
// A graphics image of a small unchecked box icon.
//
//*****************************************************************************
const uint8_t g_ui8Unchecked[] =
{
    IMAGE_FMT_1BPP_UNCOMP,
    7, 0,
    8, 0,

    0xFE,
    0x82,
    0x82,
    0x82,
    0x82,
    0x82,
    0xFE,
    0
};

//*****************************************************************************
//
// A graphics image of a small checked box icon.
//
//*****************************************************************************
const uint8_t g_ui8Checked[] =
{
    IMAGE_FMT_1BPP_UNCOMP,
    7, 0,
    8, 0,

    0xFE,
    0xC6,
    0xAA,
    0x92,
    0xAA,
    0xC6,
    0xFE,
    0
};

//*****************************************************************************
//
//! Draws the current menu into a drawing context, off-screen buffer.
//!
//! \param psMenuWidget points at the SlideMenuWidget being processed.
//! \param psContext points to the context where all drawing should be done.
//! \param i32OffsetY is the Y offset for drawing the menu.
//!
//! This function renders a menu (set of menu items), into a drawing context.
//! It assumes that the drawing context is an off-screen buffer, and that
//! the entire buffer belongs to this widget.  The vertical position of the
//! menu can be adjusted by using the parameter i32OffsetY.  This value can be
//! positive or negative and can cause the menu to be rendered above or below
//! the normal position in the display.
//!
//! \return None.
//
//*****************************************************************************
void
SlideMenuDraw(tSlideMenuWidget *psMenuWidget, tContext *psContext,
              int32_t i32OffsetY)
{
    tSlideMenu *psMenu;
    uint32_t ui32Idx;
    tRectangle sRect;

    //
    // Check the arguments
    //
    ASSERT(psMenuWidget);
    ASSERT(psContext);

    //
    // Set the foreground color for the rectangle fill to match what we want
    // as the menu background.
    //
    GrContextForegroundSet(psContext, psMenuWidget->ui32ColorBackground);
    GrRectFill(psContext, &psContext->sClipRegion);

    //
    // Get the current menu that is being displayed
    //
    psMenu = psMenuWidget->psSlideMenu;

    //
    // Set the foreground to the color we want for the menu item boundaries
    // and text color, text font.
    //
    GrContextForegroundSet(psContext, psMenuWidget->ui32ColorForeground);
    GrContextFontSet(psContext, psMenuWidget->psFont);

    //
    // Set the rectangle bounds for the first menu item.
    // The starting Y value is calculated based on which menu item is currently
    // centered.  Y coordinates are subtracted to find the Y start location
    // of the first menu item, which could even be off the display.
    //
    // Set the X coords of the menu item to the extents of the display
    //
    sRect.i16XMin = 0;
    sRect.i16XMax = psContext->sClipRegion.i16XMax;

    //
    // Find the Y coordinate of the centered menu item
    //
    sRect.i16YMin = (psContext->psDisplay->ui16Height / 2) -
                    (psMenuWidget->ui32MenuItemHeight / 2);

    //
    // Adjust to find Y coordinate of first menu item
    //
    sRect.i16YMin -= psMenu->ui32CenterIndex * psMenuWidget->ui32MenuItemHeight;

    //
    // Now adjust for the offset that was passed in by caller.  This allows
    // for drawing menu items above or below the main display.
    //
    sRect.i16YMin += i32OffsetY;

    //
    // Find the ending Y coordinate of first menu item
    //
    sRect.i16YMax = sRect.i16YMin + psMenuWidget->ui32MenuItemHeight - 1;

    //
    // Start the index at the first menu item.  It is possible that this
    // menu item is off the display.
    //
    ui32Idx = 0;

    //
    // Loop through all menu items, drawing on the display.  Note that some
    // may not be on the screen, but they will be clipped.
    //
    while(ui32Idx < psMenu->ui32Items)
    {
        //
        // If this index is the one that is highlighted, then change the
        // background
        //
        if(ui32Idx == psMenu->ui32FocusIndex)
        {
            //
            // Set the foreground to the highlight color, and fill the
            // rectangle of the background of this menu item.
            //
            GrContextForegroundSet(psContext, psMenuWidget->ui32ColorHighlight);
            GrRectFill(psContext, &sRect);

            //
            // Set the new foreground to the normal foreground color, and
            // set the background to the highlight color.  This is so
            // remaining drawing operations will have the correct background
            // and foreground colors for this highlighted menu item cell.
            //
            GrContextForegroundSet(psContext, psMenuWidget->ui32ColorForeground);
            GrContextBackgroundSet(psContext, psMenuWidget->ui32ColorHighlight);

            //
            // If this menu has a parent, then draw a left arrow icon on the
            // focused menu item.
            //
            if(psMenu->psParent)
            {
                GrImageDraw(psContext, g_ui8LtArrow, sRect.i16XMin + 4,
                            sRect.i16YMin +
                            (psMenuWidget->ui32MenuItemHeight / 2) - 4);
            }

            //
            // If this menu has a child menu or child widget, then draw a
            // right arrow icon on the focused menu item.
            //
            if(psMenu->psSlideMenuItems[ui32Idx].psChildMenu ||
               psMenu->psSlideMenuItems[ui32Idx].psChildWidget)
            {
                GrImageDraw(psContext, g_ui8RtArrow, sRect.i16XMax - 8,
                            sRect.i16YMin +
                            (psMenuWidget->ui32MenuItemHeight / 2) - 4);
            }
        }

        //
        // Otherwise this is a normal, non-highlighted menu item cell,
        // so set the normal background color.
        //
        else
        {
            GrContextBackgroundSet(psContext, psMenuWidget->ui32ColorBackground);
        }

        //
        // If the current menu is multi-selectable, then draw a checkbox on
        // the menu item.  Draw a checked or unchecked box depending on whether
        // the item has been selected.
        //
        if(psMenu->bMultiSelectable)
        {
            if(psMenu->ui32SelectedFlags & (1 << ui32Idx))
            {
                GrImageDraw(psContext, g_ui8Checked, sRect.i16XMax - 12,
                            sRect.i16YMin +
                            (psMenuWidget->ui32MenuItemHeight / 2) - 4);
            }
            else
            {
                GrImageDraw(psContext, g_ui8Unchecked, sRect.i16XMax - 12,
                            sRect.i16YMin +
                            (psMenuWidget->ui32MenuItemHeight / 2) - 4);
            }

        }

        //
        // Draw the rectangle representing the menu item
        //
        GrRectDraw(psContext, &sRect);

        //
        // Draw the text for this menu item in the middle of the menu item
        // rectangle (cell).
        //
        GrStringDrawCentered(psContext,
                             psMenu->psSlideMenuItems[ui32Idx].pcText,
                             -1,
                             psMenuWidget->sBase.psDisplay->ui16Width / 2,
                             sRect.i16YMin + \
                             (psMenuWidget->ui32MenuItemHeight / 2) - 1, 0);

        //
        // Advance to the next menu item, and update the menu item rectangle
        // bounds to the next position
        //
        ui32Idx++;
        sRect.i16YMin += psMenuWidget->ui32MenuItemHeight;
        sRect.i16YMax += psMenuWidget->ui32MenuItemHeight;

        //
        // Note that this may attempt to render menu items that run off the
        // bottom of the drawing area, but these will just be clipped and a
        // little bit of processing time is wasted.
        //
    }
}

//*****************************************************************************
//
//! Paints a menu, menu items on a display.
//!
//! \param psWidget is a pointer to the slide menu widget to be drawn.
//!
//! This function draws the contents of a slide menu on the display.  This is
//! called in response to a \b WIDGET_MSG_PAINT message.
//!
//! \return None.
//
//*****************************************************************************
static void
SlideMenuPaint(tWidget *psWidget)
{
    tSlideMenuWidget *psMenuWidget;
    tContext sContext;

    //
    // Check the arguments.
    //
    ASSERT(psWidget);

    //
    // If this widget has a child widget, that means that the menu has
    // slid off the screen and the child widget is in control.  Therefore
    // there is nothing to paint here.  Just exit and the child widget will
    // be painted.
    //
    if(psWidget->psChild)
    {
        return;
    }

    //
    // Convert the generic widget pointer into a slide menu widget pointer,
    // and get a pointer to its context.
    //
    psMenuWidget = (tSlideMenuWidget *)psWidget;

    //
    // Initialize a context for the primary off-screen drawing buffer.
    // Clip region is set to entire display by default, which is what we want.
    //
    GrContextInit(&sContext, psMenuWidget->psDisplayA);

    //
    // Render the menu into the off-screen buffer, using normal vertical
    // position.
    //
    SlideMenuDraw(psMenuWidget, &sContext, 0);

    //
    // Initialize a drawing context for the display where the widget is to be
    // drawn.  This is the physical display, not an off-screen buffer.
    //
    GrContextInit(&sContext, psWidget->psDisplay);

    //
    // Initialize the clipping region on the physical display, based on the
    // extents of this widget.
    //
    GrContextClipRegionSet(&sContext, &(psWidget->sPosition));

    //
    // Now copy the rendered menu into the physical display. This will show
    // the menu on the display.
    //
    GrImageDraw(&sContext, psMenuWidget->psDisplayA->pvDisplayData,
                psWidget->sPosition.i16XMin, psWidget->sPosition.i16YMin);
}

//*****************************************************************************
//
//! Performs the sliding menu operation, in response to the "down" button.
//!
//! \param psWidget is a pointer to the slide menu widget to move down.
//!
//! This function will respond to the "down" key/button event.  The down
//! button is used to select the next menu item down the list, and the effect
//! is that the menu itself slides up, leaving the highlighted menu item
//! in the middle of the screen.
//!
//! This function repeatedly draws the menu onto the display until the sliding
//! animation is finished and will not return to the caller until then.  This
//! function is usually called from the thread context of
//! WidgetMessageQueueProcess().
//!
//! \return Returns a non-zero value if the menu was moved or was not moved
//! because it is already at the last position.  If a child widget is active
//! then this function does nothing and returns a 0.
//
//*****************************************************************************
static int32_t
SlideMenuDown(tWidget *psWidget)
{
    tSlideMenuWidget *psMenuWidget;
    tSlideMenu *psMenu;
    tContext sContext;
    uint32_t ui32MenuHeight;
    uint32_t ui32Y;

    //
    // If this menu widget has a child widget, that means the child widget
    // is in control of the display, and there is nothing to do here.
    //
    if(psWidget->psChild)
    {
        return(0);
    }

    //
    // Get handy pointers to the menu widget, and the menu that is currently
    // displayed.
    //
    psMenuWidget = (tSlideMenuWidget *)psWidget;
    psMenu = psMenuWidget->psSlideMenu;

    //
    // If we are already at the end of the list of menu items, then there
    // is nothing else to do.
    //
    if(psMenu->ui32FocusIndex >= (psMenu->ui32Items - 1))
    {
        return(1);
    }

    //
    // Increment focus menu item.  This has the effect of selecting the next
    // menu item in the list.
    //
    psMenu->ui32FocusIndex++;

    //
    // Initialize a context for the primary off-screen drawing buffer.
    // Clip region is set to entire display by default, which is what we want.
    //
    GrContextInit(&sContext, psMenuWidget->psDisplayA);

    //
    // Render the menu into the off-screen buffer.  This will be the same
    // menu appearance as before, except the highlighted item has changed
    // to the next menu item down.
    //
    SlideMenuDraw(psMenuWidget, &sContext, 0);

    //
    // Draw a continuation of this menu in the second offscreen buffer.
    // This is the part of the menu that would be drawn if the display were
    // twice as tall.  We are effectively creating a virtual display that is
    // twice as tall as the physical display.
    //
    GrContextInit(&sContext, psMenuWidget->psDisplayB);
    SlideMenuDraw(psMenuWidget, &sContext, -1 *
                  (psMenuWidget->sBase.sPosition.i16YMax -
                  psMenuWidget->sBase.sPosition.i16YMin));

    //
    // Initialize a drawing context for the display where the widget is to be
    // drawn.  This is the physical display, not an off-screen buffer.
    //
    GrContextInit(&sContext, psWidget->psDisplay);

    //
    // Initialize the clipping region on the physical display, based on the
    // extents of this widget.
    //
    GrContextClipRegionSet(&sContext, &(psWidget->sPosition));

    //
    // Get the height of the displayed part of the menu.
    //
    ui32MenuHeight = psMenuWidget->psDisplayA->ui16Height;

    //
    // Now copy the rendered menu into the physical display
    //
    // Iterate over the Y displacement of one menu item cell.  This loop
    // will repeatedly draw both off screen buffers to the physical display,
    // adjusting the position of each by one pixel each time it is drawn.  Each
    // time the offset is changed so that both buffers are drawn one higher
    // than the previous time.  This will have the effect of "sliding" the
    // entire menu up by the height of one menu item cell.
    // The speed of the animation is controlled entirely by the speed of the
    // processor and the speed of the interface to the physical display.
    //
    for(ui32Y = 0; ui32Y <= psMenuWidget->ui32MenuItemHeight; ui32Y++)
    {
        GrImageDraw(&sContext, psMenuWidget->psDisplayA->pvDisplayData,
                    psWidget->sPosition.i16XMin,
                    psWidget->sPosition.i16YMin - ui32Y);
        GrImageDraw(&sContext, psMenuWidget->psDisplayB->pvDisplayData,
                    psWidget->sPosition.i16XMin,
                    psWidget->sPosition.i16YMin + ui32MenuHeight - ui32Y);
    }

    //
    // Increment centered menu item.  This will now match the menu item with
    // the focus.  When the menu is repainted again, the newly selected
    // menu item will be centered and highlighted.
    //
    psMenu->ui32CenterIndex = psMenu->ui32FocusIndex;

    //
    // Initialize a context for the primary off-screen drawing buffer.
    // Clip region is set to entire display by default, which is what we want.
    //
    GrContextInit(&sContext, psMenuWidget->psDisplayA);

    //
    // Render the menu into the off-screen buffer.  This will be the same
    // menu appearance as before, except the highlighted item has changed
    // to the next menu item down.  Now when a repaint occurs the menu
    // will be redrawn with the newly highlighted menu item.
    //
    SlideMenuDraw(psMenuWidget, &sContext, 0);

    //
    // Return indication that we handled the key event.
    //
    return(1);
}

//*****************************************************************************
//
//! Performs the sliding menu operation, in response to the "up" button.
//!
//! \param psWidget is a pointer to the slide menu widget to move up.
//!
//! This function will respond to the "up" key/button event.  The up
//! button is used to select the previous menu item down the list, and the
//! effect is that the menu itself slides down, leaving the highlighted menu
//! item in the middle of the screen.
//!
//! This function repeatedly draws the menu onto the display until the sliding
//! animation is finished and will not return to the caller until then.  This
//! function is usually called from the thread context of
//! WidgetMessageQueueProcess().
//!
//! \return Returns a non-zero value if the menu was moved or was not moved
//! because it is already at the first position.  If a child widget is active
//! then this function does nothing and returns a 0.
//
//*****************************************************************************
static int32_t
SlideMenuUp(tWidget *psWidget)
{
    tSlideMenuWidget *psMenuWidget;
    tSlideMenu *psMenu;
    tContext sContext;
    uint32_t ui32MenuHeight;
    uint32_t ui32Y;

    //
    // If this menu widget has a child widget, that means the child widget
    // is in control of the display, and there is nothing to do here.
    //
    if(psWidget->psChild)
    {
        return(0);
    }

    //
    // Get handy pointers to the menu widget, and the menu that is currently
    // displayed.
    //
    psMenuWidget = (tSlideMenuWidget *)psWidget;
    psMenu = psMenuWidget->psSlideMenu;

    //
    // If we are already at the start of the list of menu items, then there
    // is nothing else to do.
    //
    if(psMenu->ui32FocusIndex == 0)
    {
        return(1);
    }

    //
    // Decrement the focus menu item.  This has the effect of selecting the
    // previous menu item in the list.
    //
    psMenu->ui32FocusIndex--;

    //
    // Initialize a context for the primary off-screen drawing buffer.
    // Clip region is set to entire display by default, which is what we want.
    //
    GrContextInit(&sContext, psMenuWidget->psDisplayA);

    //
    // Render the menu into the off-screen buffer.  This will be the same
    // menu appearance as before, except the highlighted item has changed
    // to the previous menu item up.
    //
    SlideMenuDraw(psMenuWidget, &sContext, 0);

    //
    // Draw a continuation of this menu in the second offscreen buffer.
    // This is the part of the menu that would be drawn above this menu if the
    // display were twice as tall.  We are effectively creating a virtual
    // display that is twice as tall as the physical display.
    //
    GrContextInit(&sContext, psMenuWidget->psDisplayB);
    SlideMenuDraw(psMenuWidget, &sContext,
                  (psMenuWidget->sBase.sPosition.i16YMax -
                  psMenuWidget->sBase.sPosition.i16YMin));

    //
    // Initialize a drawing context for the display where the widget is to be
    // drawn.  This is the physical display, not an off-screen buffer.
    //
    GrContextInit(&sContext, psWidget->psDisplay);

    //
    // Initialize the clipping region on the physical display, based on the
    // extents of this widget.
    //
    GrContextClipRegionSet(&sContext, &(psWidget->sPosition));

    //
    // Get the height of the displayed part of the menu.
    //
    ui32MenuHeight = psMenuWidget->psDisplayA->ui16Height;

    //
    // Now copy the rendered menu into the physical display
    //
    // Iterate over the Y displacement of one menu item cell.  This loop
    // will repeatedly draw both off screen buffers to the physical display,
    // adjusting the position of each by one pixel each time it is drawn.  Each
    // time the offset is changed so that both buffers are drawn one lower
    // than the previous time.  This will have the effect of "sliding" the
    // entire menu down by the height of one menu item cell.
    // The speed of the animation is controlled entirely by the speed of the
    // processor and the speed of the interface to the physical display.
    //
    for(ui32Y = 0; ui32Y <= psMenuWidget->ui32MenuItemHeight; ui32Y++)
    {
        GrImageDraw(&sContext, psMenuWidget->psDisplayB->pvDisplayData,
                    psWidget->sPosition.i16XMin,
                    psWidget->sPosition.i16YMin + ui32Y - ui32MenuHeight);
        GrImageDraw(&sContext, psMenuWidget->psDisplayA->pvDisplayData,
                    psWidget->sPosition.i16XMin,
                    psWidget->sPosition.i16YMin + ui32Y);
    }

    //
    // Decrement the  centered menu item.  This will now match the menu item
    // with the focus.  When the menu is repainted again, the newly selected
    // menu item will be centered and highlighted.
    //
    psMenu->ui32CenterIndex = psMenu->ui32FocusIndex;

    //
    // Initialize a context for the primary off-screen drawing buffer.
    // Clip region is set to entire display by default, which is what we want.
    //
    GrContextInit(&sContext, psMenuWidget->psDisplayA);

    //
    // Render the menu into the off-screen buffer.  This will be the same
    // menu appearance as before, except the highlighted item has changed
    // to the next menu item up.  Now when a repaint occurs the menu
    // will be redrawn with the newly highlighted menu item.
    //
    SlideMenuDraw(psMenuWidget, &sContext, 0);

    //
    // Return indication that we handled the key event.
    //
    return(1);
}

//*****************************************************************************
//
//! Performs the sliding menu operation, in response to the "right" button.
//!
//! \param psWidget is a pointer to the slide menu widget to move to the right.
//!
//! This function will respond to the "right" key/button event.  The right
//! button is used to select the next menu level below the current menu item,
//! or a widget that is activated by the menu item.  The effect is that the
//! menu itself slides off to the left, and the new menu or widget slides in
//! from the right.
//!
//! This function repeatedly draws the menu onto the display until the sliding
//! animation is finished and will not return to the caller until then.  This
//! function is usually called from the thread context of
//! WidgetMessageQueueProcess().
//!
//! \return Returns a non-zero value if the menu was moved or was not moved
//! because it is already at the last position.  If a child widget is active
//! then this function does nothing and returns a 0.
//
//*****************************************************************************
static int32_t
SlideMenuRight(tWidget *psWidget)
{
    tSlideMenuWidget *psMenuWidget;
    tSlideMenu *psMenu;
    tSlideMenu *psChildMenu;
    tContext sContext;
    tWidget *psChildWidget;
    uint32_t ui32X;
    uint32_t ui32MenuWidth;

    //
    // If this menu widget has a child widget, that means the child widget
    // is in control of the display, and there is nothing to do here.
    //
    if(psWidget->psChild)
    {
        return(0);
    }

    //
    // Get handy pointers to the menu widget, and the current menu, and the
    // child menu and widget if they exist.
    //
    psMenuWidget = (tSlideMenuWidget *)psWidget;
    psMenu = psMenuWidget->psSlideMenu;
    psChildMenu = psMenu->psSlideMenuItems[psMenu->ui32FocusIndex].psChildMenu;
    psChildWidget = psMenu->psSlideMenuItems[psMenu->ui32FocusIndex].psChildWidget;

    //
    // Initialize a context for the secondary off-screen drawing buffer.
    // Clip region is set to entire display by default, which is what we want.
    //
    GrContextInit(&sContext, psMenuWidget->psDisplayB);

    //
    // Render the current menu into off-screen buffer B.  This
    // will be the same menu appearance as is already being shown.
    //
    SlideMenuDraw(psMenuWidget, &sContext, 0);

    //
    // Now set up context for drawing into off-screen buffer A
    //
    GrContextInit(&sContext, psMenuWidget->psDisplayA);

    //
    // Process child menu of this menu item
    //
    if(psChildMenu)
    {
        //
        // Switch the active menu for this SlideMenuWidget to be the child
        // menu
        //
        psMenuWidget->psSlideMenu = psChildMenu;

        //
        // Draw the new (child) menu into off-screen buffer A
        //
        SlideMenuDraw(psMenuWidget, &sContext, 0);
    }

    //
    // Process child widget of this menu item.  This only happens if there
    // is no child menu.
    //
    else if(psChildWidget)
    {
        //
        // Call the widget activated callback function.  This will notify
        // the application that a child widget has been activated by the
        // menu system.
        //
        if(psMenuWidget->pfnActive)
        {
            psMenuWidget->pfnActive(psChildWidget,
                                   &psMenu->psSlideMenuItems[psMenu->ui32FocusIndex],
                                   1);
        }

        //
        // Link the new child widget into this SlideMenuWidget so
        // it appears as a child to this widget.  Normally the menu widget
        // has no child widget.
        //
        psWidget->psChild = psChildWidget;
        psChildWidget->psParent = psWidget;

        //
        // Fill a rectangle with the new child widget background color.
        // This is done in off-screen buffer A.  When the menu slides off,
        // it will be replaced by a blank background that will then be
        // controlled by the new child widget.
        //
        GrContextForegroundSet(
            &sContext,
            psMenu->psSlideMenuItems[psMenu->ui32FocusIndex].ui32ChildWidgetColor);
        GrRectFill(&sContext, &sContext.sClipRegion);

        //
        // Request a repaint for the child widget so it can draw itself once
        // the menu slide is done.
        //
        WidgetPaint(psChildWidget);
    }

    //
    // There is no child menu or child widget, so there is nothing to change
    // on the display.
    //
    else
    {
        return(1);
    }

    //
    // Initialize a drawing context for the display where the widget is to be
    // drawn.  This is the physical display, not an off-screen buffer.
    //
    GrContextInit(&sContext, psWidget->psDisplay);

    //
    // Initialize the clipping region on the physical display, based on the
    // extents of this widget.
    //
    GrContextClipRegionSet(&sContext, &(psWidget->sPosition));

    //
    // Get the width of the menu widget which is used in calculations below
    //
    ui32MenuWidth = psMenuWidget->psDisplayA->ui16Width;

    //
    // The following loop draws the two off-screen buffers onto the physical
    // display using a right-to-left-wipe.  This will provide an appearance
    // of sliding to the left.  The new child menu, or child widget background
    // will slide in from the right.  The "old" menu is being held in
    // off-screen buffer B and the new one is in buffer A.  So when we are
    // done, the correct image will be in buffer A.
    //
    for(ui32X = 0; ui32X <= ui32MenuWidth; ui32X += 8)
    {
        GrImageDraw(&sContext, psMenuWidget->psDisplayB->pvDisplayData,
                    psWidget->sPosition.i16XMin - ui32X,
                    psWidget->sPosition.i16YMin);
        GrImageDraw(&sContext, psMenuWidget->psDisplayA->pvDisplayData,
                    psWidget->sPosition.i16XMin + ui32MenuWidth - ui32X,
                    psWidget->sPosition.i16YMin);
    }

    //
    // Return indication that we handled the key event.
    //
    return(1);
}

//*****************************************************************************
//
//! Performs the sliding menu operation, in response to the "left" button.
//!
//! \param psWidget is a pointer to the slide menu widget to move to the left.
//!
//! This function will respond to the "left" key/button event.  The left
//! button is used to ascend to the next menu up in the menu tree.  The effect
//! is that the current menu, or active widget, slides off to the right, while
//! the parent menu slides in from the left.
//!
//! This function repeatedly draws the menu onto the display until the sliding
//! animation is finished and will not return to the caller until then.  This
//! function is usually called from the thread context of
//! WidgetMessageQueueProcess().
//!
//! \return Returns a non-zero value if the menu was moved or was not moved
//! because it is already at the last position.  If a child widget is active
//! then this function does nothing and returns a 0.
//
//*****************************************************************************
static int32_t
SlideMenuLeft(tWidget *psWidget)
{
    tSlideMenuWidget *psMenuWidget;
    tSlideMenu *psMenu;
    tSlideMenu *psParentMenu;
    tContext sContext;
    uint32_t ui32X;
    uint32_t ui32MenuWidth;

    //
    // Get handy pointers to the menu widget and active menu, and the parent
    // menu if there is one.
    //
    psMenuWidget = (tSlideMenuWidget *)psWidget;
    psMenu = psMenuWidget->psSlideMenu;
    psParentMenu = psMenu->psParent;

    //
    // Initialize a context for the primary off-screen drawing buffer.
    // Clip region is set to entire display by default, which is what we want.
    //
    GrContextInit(&sContext, psMenuWidget->psDisplayB);

    //
    // If this widget has a child, that means that the child widget is in
    // control, and we are requested to go back to the previous menu item.
    // Process the child widget.
    //
    if(psWidget->psChild)
    {
        //
        // Call the widget de-activated callback function.  This notifies the
        // application that the widget is being deactivated.
        //
        if(psMenuWidget->pfnActive)
        {
            psMenuWidget->pfnActive(psWidget->psChild,
                                   &psMenu->psSlideMenuItems[psMenu->ui32FocusIndex],
                                   0);
        }

        //
        // Unlink the child widget from the slide menu widget.  The menu
        // widget will now no longer have a child widget.
        //
        psWidget->psChild->psParent = 0;
        psWidget->psChild = 0;

        //
        // Fill a rectangle with the child widget background color.  This will
        // erase everything else that is shown on the widget but leave the
        // background, which will make the change visually less jarring.
        // This is done in off-screen buffer B, which is the buffer that is
        // going to be slid off the screen.
        //
        GrContextForegroundSet(
            &sContext,
            psMenu->psSlideMenuItems[psMenu->ui32FocusIndex].ui32ChildWidgetColor);
        GrRectFill(&sContext, &sContext.sClipRegion);
    }

    //
    // Otherwise there is not a child widget in control, so process the parent
    // menu, if there is one.
    //
    else if(psParentMenu)
    {
        //
        // Render the current menu into the off-screen buffer B.  This will be
        // the same menu appearance that is currently on the display.
        //
        SlideMenuDraw(psMenuWidget, &sContext, 0);

        //
        // Now switch the widget to the parent menu
        //
        psMenuWidget->psSlideMenu = psParentMenu;
    }

    //
    // Otherwise, we are already at the top level menu and there is nothing
    // else to do.
    //
    else
    {
        return(1);
    }

    //
    // Draw the new menu in the second offscreen buffer.  This is the menu
    // that will be on the display when the animation is over.
    //
    GrContextInit(&sContext, psMenuWidget->psDisplayA);
    SlideMenuDraw(psMenuWidget, &sContext, 0);

    //
    // Initialize a drawing context for the display where the widget is to be
    // drawn.  This is the physical display, not an off-screen buffer.
    //
    GrContextInit(&sContext, psWidget->psDisplay);

    //
    // Initialize the clipping region on the physical display, based on the
    // extents of this widget.
    //
    GrContextClipRegionSet(&sContext, &(psWidget->sPosition));

    //
    // Get the width of the menu widget.
    //
    ui32MenuWidth = psMenuWidget->psDisplayA->ui16Width;

    //
    // The following loop draws the two off-screen buffers onto the physical
    // display using a left-to-right.  This will provide an appearance
    // of sliding to the right.  The parent menu will slide in from the left.
    // The "old" child menu is being held in off-screen buffer B and the new
    // one is in buffer A.  So when we are done, the correct image will be in
    // buffer A.
    //
    for(ui32X = 0; ui32X <= ui32MenuWidth; ui32X += 8)
    {
        GrImageDraw(&sContext, psMenuWidget->psDisplayB->pvDisplayData,
                    psWidget->sPosition.i16XMin + ui32X,
                    psWidget->sPosition.i16YMin);
        GrImageDraw(&sContext, psMenuWidget->psDisplayA->pvDisplayData,
                    psWidget->sPosition.i16XMin + ui32X - ui32MenuWidth,
                    psWidget->sPosition.i16YMin);
    }

    //
    // Return indication that we handled the key event.
    //
    return(1);
}

//*****************************************************************************
//
//! Handles menu selection, in response to the "select" button.
//!
//! \param psWidget is a pointer to the slide menu widget to use for a
//! select operation.
//!
//! This function will allow for checking or unchecking multi-selectable
//! menu items.  If the menu does not allow multiple selection, then it
//! treats it as a "right" button press.
//!
//! \return Returns a non-zero value if the key was handled.  Returns 0 if the
//! key was not handled.
//
//*****************************************************************************
static int32_t
SlideMenuClick(tWidget *psWidget)
{
    tSlideMenuWidget *psMenuWidget;
    tSlideMenu *psMenu;

    //
    // If a child widget is in control then there is nothing to do.
    //
    if(psWidget->psChild)
    {
        return(0);
    }

    //
    // Get handy pointers to the menu widget and current menu.
    //
    psMenuWidget = (tSlideMenuWidget *)psWidget;
    psMenu = psMenuWidget->psSlideMenu;

    //
    // Check to see if this menu allows multiple selection.
    //
    if(psMenu->bMultiSelectable)
    {
        //
        // Toggle the selection status of the currently highlighted menu
        // item, and then repaint it.
        //
        psMenu->ui32SelectedFlags ^= 1 << psMenu->ui32FocusIndex;
        SlideMenuPaint(psWidget);

        //
        // We are done so return indication that we handled the key event.
        //
        return(1);
    }

    //
    // Otherwise, treat the select button the same as a right button.
    //
    return(SlideMenuRight(psWidget));
}

//*****************************************************************************
//
//! Process key/button event to decide how to move the sliding menu.
//!
//! \param psWidget is a pointer to the slide menu widget to process.
//! \param ui32Msg is the message containing the key event.
//!
//! This function is used to specifically handle key events destined for the
//! slide menu widget.  It decides which menu movement function should be
//! called for each key event.
//!
//! \return Returns an indication if the key was handled.  Non-zero if the
//! key event was handled or else 0.
//
//*****************************************************************************
static int32_t
SlideMenuMove(tWidget *psWidget, uint32_t ui32Msg)
{
    //
    // Process the key event.
    //
    switch(ui32Msg)
    {
        //
        // User presses select button.
        //
        case WIDGET_MSG_KEY_SELECT:
        {
            return(SlideMenuClick(psWidget));
        }

        //
        // User presses up button.
        //
        case WIDGET_MSG_KEY_UP:
        {
            return(SlideMenuUp(psWidget));
        }

        //
        // User presses down button.
        //
        case WIDGET_MSG_KEY_DOWN:
        {
            return(SlideMenuDown(psWidget));
        }

        //
        // User presses left button.
        //
        case WIDGET_MSG_KEY_LEFT:
        {
            return(SlideMenuLeft(psWidget));
        }

        //
        // User presses right button.
        //
        case WIDGET_MSG_KEY_RIGHT:
        {
            return(SlideMenuRight(psWidget));
        }

        //
        // This is an unexpected event.  Return an indication that the event
        // was not handled.
        //
        default:
        {
            return(0);
        }
    }
}

//*****************************************************************************
//
//! Handles messages for a slide menu widget.
//!
//! \param psWidget is a pointer to the slide menu widget.
//! \param ui32Msg is the message.
//! \param ui32Param1 is the first parameter to the message.
//! \param ui32Param2 is the second parameter to the message.
//!
//! This function receives messages intended for this slide menu widget and
//! processes them accordingly.  The processing of the message varies based on
//! the message in question.
//!
//! Unrecognized messages are handled by calling WidgetDefaultMsgProc().
//!
//! \return Returns a value appropriate to the supplied message.
//
//*****************************************************************************
int32_t
SlideMenuMsgProc(tWidget *psWidget, uint32_t ui32Msg, uint32_t ui32Param1,
              uint32_t ui32Param2)
{
    //
    // Check the arguments.
    //
    ASSERT(psWidget);

    //
    // Determine which message is being sent.
    //
    switch(ui32Msg)
    {
        //
        // The widget paint request has been sent.
        //
        case WIDGET_MSG_PAINT:
        {
            //
            // Handle the widget paint request.
            //
            SlideMenuPaint(psWidget);

            //
            // Return one to indicate that the message was successfully
            // processed.
            //
            return(1);
        }

        //
        // A key event has been received.  By convention, this widget will
        // process the key events if ui32Param1 is set to this widget.
        // Otherwise a different widget has the "focus" for key events.
        //
        case WIDGET_MSG_KEY_SELECT:
        case WIDGET_MSG_KEY_UP:
        case WIDGET_MSG_KEY_DOWN:
        case WIDGET_MSG_KEY_LEFT:
        case WIDGET_MSG_KEY_RIGHT:
        {
            //
            // If this key event is for us, then process the event.
            //
            if((tWidget *)ui32Param1 == psWidget)
            {
                return(SlideMenuMove(psWidget, ui32Msg));
            }
        }

        //
        // An unknown request has been sent.  This widget does not handle
        // pointer events, so they get dumped here if they occur.
        //
        default:
        {
            //
            // Let the default message handler process this message.
            //
            return(WidgetDefaultMsgProc(psWidget, ui32Msg, ui32Param1,
                                        ui32Param2));
        }
    }
}

//*****************************************************************************
//
//! Initializes a slide menu widget.
//!
//! \param psWidget is a pointer to the slide menu widget to initialize.
//! \param psDisplay is a pointer to the display on which to draw the menu.
//! \param i32X is the X coordinate of the upper left corner of the canvas.
//! \param i32Y is the Y coordinate of the upper left corner of the canvas.
//! \param i32Width is the width of the canvas.
//! \param i32Height is the height of the canvas.
//! \param psDisplayOffA is one of two off-screen displays used for rendering.
//! \param psDisplayOffB is one of two off-screen displays used for rendering.
//! \param ui32ItemHeight is the height of a menu item
//! \param ui32Foreground is the foreground color used for menu item boundaries
//! and text.
//! \param ui32Background is the background color of a menu item.
//! \param ui32Highlight is the color of a highlighted menu item.
//! \param psFont is a pointer to the font that should be used for text.
//! \param psMenu is the initial menu to display
//!
//! This function initializes the caller provided slide menu widget.
//!
//! \return None.
//
//*****************************************************************************
void
SlideMenuInit(tSlideMenuWidget *psWidget, const tDisplay *psDisplay,
              int32_t i32X, int32_t i32Y, int32_t i32Width, int32_t i32Height,
              tDisplay *psDisplayOffA, tDisplay *psDisplayOffB,
              uint32_t ui32ItemHeight, uint32_t ui32Foreground,
              uint32_t ui32Background, uint32_t ui32Highlight,
              tFont *psFont, tSlideMenu *psMenu)
{
    uint32_t ui32Idx;

    //
    // Check the arguments.
    //
    ASSERT(psWidget);
    ASSERT(psDisplay);
    ASSERT(psDisplayOffA);
    ASSERT(psDisplayOffB);
    ASSERT(psFont);
    ASSERT(psMenu);

    //
    // Clear out the widget structure.
    //
    for(ui32Idx = 0; ui32Idx < sizeof(tSlideMenuWidget); ui32Idx += 4)
    {
        ((uint32_t *)psWidget)[ui32Idx / 4] = 0;
    }

    //
    // Set the size of the widget structure.
    //
    psWidget->sBase.i32Size = sizeof(tSlideMenuWidget);

    //
    // Mark this widget as fully disconnected.
    //
    psWidget->sBase.psParent = 0;
    psWidget->sBase.psNext = 0;
    psWidget->sBase.psChild = 0;

    //
    // Save the display pointer.
    //
    psWidget->sBase.psDisplay = psDisplay;

    //
    // Set the extents of the display area.
    //
    psWidget->sBase.sPosition.i16XMin = i32X;
    psWidget->sBase.sPosition.i16YMin = i32Y;
    psWidget->sBase.sPosition.i16XMax = i32X + i32Width - 1;
    psWidget->sBase.sPosition.i16YMax = i32Y + i32Height - 1;

    //
    // Initialize the widget fields
    //
    psWidget->psDisplayA = psDisplayOffA;
    psWidget->psDisplayB = psDisplayOffB;
    psWidget->ui32MenuItemHeight = ui32ItemHeight;
    psWidget->ui32ColorForeground = ui32Foreground;
    psWidget->ui32ColorBackground = ui32Background;
    psWidget->ui32ColorHighlight = ui32Highlight;
    psWidget->psFont = psFont;
    psWidget->psSlideMenu = psMenu;

    //
    // Use the slide menu message handler to process messages to this widget.
    //
    psWidget->sBase.pfnMsgProc = SlideMenuMsgProc;
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
