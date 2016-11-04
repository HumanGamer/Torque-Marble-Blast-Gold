//!autoload In-game script editor
//!module scripteditor
//-------------------------------------------------------------
// TGE Ingame Script Editor
// Davis Ray Sickmon, Jr (midryder@midnightryder.com)
//-------------------------------------------------------------
//-------------------------------------------------------------
// An integrated Script Editor for Torque.  Based on one night's
// hacking.  This is not a final product sort of deal - instead,
// it's something I'll add features to from time to time.  As
// it stands now it's mostly functional for short hacking runs
// from within Torque.  Long runs would probably make you miss
// VI / EMACS / TribalIDE / etc. :-)
// Installation:  Put this script in /common/editor
// In fps/client/init.cs (or mod of choice, like
// starter.fps/client/init.cs) add the following at the bottom of
// function initClient():
// exec("commmon/editor/ScriptEditor.cs");
// Run your game, and press ALT-F11 - you're in!
// Things to know - there's no cut & paste mouse context menu.
// However, ctrl+c, v, and x all work just fine.
//-------------------------------------------------------------


// We've got to have some way to call up the editor no matter where
// we are at.
GlobalActionMap.bind(keyboard, "alt f11", ScriptEditorInit);
GlobalActionMap.bind(keyboard, "control z", SEUndo);


//-------------------------------------------------------------
// Initalize the editor by calling ScriptEditorInit() - it
// does all the inital setup, obviously.
//-------------------------------------------------------------


function ScriptEditorInit()
{
    if(!isObject(GuiSEText)) new GuiControlProfile (GuiSEText)
    {
        fontType = ($platform $= "macos") ? "Courier New" : "Lucida Console";
        fontSize = 12;
     			   fontColorLink = "255 96 96";
        fontColorLinkHL = "0 0 255";

        fillColor = "255 255 255";
        fillColorHL = "128 128 128";

        fontColor = "0 0 0";
        fontColorHL = "0 64 0";
        fontColorNA = "128 128 128";

        autoSizeWidth = true;
        autoSizeHeight = true;
        tab = true;
        canKeyFocus = true;
    };

   Canvas.cursorOn();
   Canvas.setCursor(DefaultCursor);
   // Is there already an instance of the editor running?
   if($scriptEditor > 0)
   {
       $scriptEditor.bringToFront($scriptEditor);
       return;
   }
   // Welcome to the Twilight Zone.  Yep, why bother with a
   // .gui file when we can just build it all in code!
   // I could just hunt for the project window later, but, it's
   // easier just to keep track of it when we want to shut down.
   $scriptEditor = new GuiWindowCtrl(SEProjectWindow) {
      profile = "GuiWindowProfile";
      horizSizing = "right";
      vertSizing = "bottom";
      position = "0 0";
      extent = "250 550";
      minExtent = "8 2";
      visible = "1";
      text = "SEProject Window";
      maxLength = "255";
      resizeWidth = "1";
      resizeHeight = "1";
      canMove = "1";
      canClose = "1";
      canMinimize = "1";
      canMaximize = "1";
      minSize = "50 50";
      closeCommand = "ScriptEditorDestroy();";

     // File listing
      new GuiScrollCtrl() {
         profile = "GuiScrollProfile";
         horizSizing = "width";
         vertSizing = "height";
         position = "1 300";
         extent = "247 251";
         minExtent = "8 2";
         visible = "1";
         willFirstRespond = "0";
         hScrollBar = "dynamic";
         vScrollBar = "dynamic";
         constantThumbHeight = "0";
         childMargin = "0 0";

         new GuiTextListCtrl(SEProjectListing) {
            profile = "GuiSEText";
            horizSizing = "width";
            vertSizing = "height";
            position = "2 2";
            extent = "177 2";
            minExtent = "8 2";
            visible = "1";
            enumerate = "1";
            resizeCell = "1";
            columns = "0";
            fitParentWidth = "0";
            clipColumnText = "0";
         };
      };

						
    // Directory Filter Listing
      new GuiScrollCtrl() {
         profile = "GuiScrollProfile";
         horizSizing = "width";
         vertSizing = "height";
         position = "1 125";
         extent = "247 180";
         minExtent = "8 2";
         visible = "1";
         willFirstRespond = "0";
         hScrollBar = "dynamic";
         vScrollBar = "dynamic";
         constantThumbHeight = "0";
         childMargin = "0 0";

         new GuiTextListCtrl(SEDirectoryFilter) {
            profile = "GuiSEText";
            horizSizing = "width";
            vertSizing = "height";
            position = "2 2";
            extent = "177 2";
            minExtent = "8 2";
            visible = "1";
            enumerate = "1";
            resizeCell = "1";
            columns = "2";
            fitParentWidth = "0";
            clipColumnText = "0";
         };
    };
    
    // File filter listing
      new GuiScrollCtrl() {
         profile = "GuiScrollProfile";
         horizSizing = "width";
         vertSizing = "height";
         position = "1 45";
         extent = "247 80";
         minExtent = "8 2";
         visible = "1";
         willFirstRespond = "0";
         hScrollBar = "dynamic";
         vScrollBar = "dynamic";
         constantThumbHeight = "0";
         childMargin = "0 0";

         new GuiTextListCtrl(SEListingFilter) {
            profile = "GuiSEText";
            horizSizing = "width";
            vertSizing = "height";
            position = "2 2";
            extent = "177 2";
            minExtent = "8 2";
            visible = "1";
            enumerate = "1";
            resizeCell = "1";
            columns = "2";
            fitParentWidth = "0";
            clipColumnText = "0";
         };
    };
      new GuiButtonCtrl(SEPWLoadButton) {
         profile = "GuiButtonProfile";
         horizSizing = "right";
         vertSizing = "bottom";
         position = "3 24";
         extent = "80 21";
         minExtent = "8 2";
         visible = "1";
         command = "SELoadProjectFromList();";
         text = "Load";
         groupNum = "-1";
         buttonType = "PushButton";
      };
      new GuiButtonCtrl(SEPWExecuteButton) {
         profile = "GuiButtonProfile";
         horizSizing = "right";
         vertSizing = "bottom";
         position = "105 24";
         extent = "89 21";
         minExtent = "8 2";
         visible = "1";
         command = "SEExecScriptFromList();";
         text = "Execute";
         groupNum = "-1";
         buttonType = "PushButton";
      };
   };
   // snag the current gui from the canvas, and add our script editor to it.
   canvas.getcontent().add($scriptEditor);
   // Populate the project filter list
   SEListingFIlter.addRow(0, "*.*", "All Files");
   SEListingFIlter.addRow(1, "*.cs", "Torque Script");
   SEListingFIlter.addRow(2, "*.gui", "GUI Scripts");
   SEListingFIlter.addRow(3, "*.mis", "Mission Scripts");
   SEListingFIlter.addRow(4, "*.jpg", "JPG Graphics");
   SEListingFIlter.addRow(5, "*.png", "PNG  Graphics");
   SEListingFIlter.addRow(6, "*.gif", "GIF Graphics");
   SEListingFIlter.addRow(7, "*.bmp", "Bitmap Graphics");
   SEListingFIlter.addRow(8, "*.wav", "WAV Audio");
   SEListingFIlter.addRow(9, "*.ogg", "OGG Audio");
   SEListingFilter.sort(0);
   SEListingFilter.setSelectedRow(0);
   SEListingFilter.scrollVisible(0);
   SELoadProjectListing("*.*");
   SELoadDirectoryTree();
   
}

//-------------------------------------------------------------
//  Called on close - deletes all of the editor windows,
//  and then deletes the editor it's self.
//-------------------------------------------------------------
function ScriptEditorDestroy()
{
    for(%i=0; %i < $SE_activeWindows; %i++)
    {
        $SE_Windows[%i].delete();
    }
    $scriptEditor.delete();
    $scriptEditor = 0;
    $SE_ActiveWindows = 0;
}

//-------------------------------------------------------------
//  Loops through and adds all the files for the project in here
//  I was going to have it cull certain filetypes - .dso's, etc.
//  However, I decided against it at the moment.  Sometimes it's
//  useful to see if a mission has been run (by seeing the lighting
//  file) or see if a script has compiled.  Later I may add
//  a filtering mechanism to the project window - IE, just show
//  .cs files, etc.
//-------------------------------------------------------------
function SELoadProjectListing(%fileSpec)
{
    SEProjectListing.clear();
    %i = 0;
     for(%file = findFirstFile(%fileSpec); %file !$= ""; %file = findNextFile(%filespec))
    {
        SEProjectListing.addRow(%i++, %file);
    }
    SEProjectListing.sort(0);
    SEProjectListing.setSelectedRow(0);
    SEProjectListing.scrollVisible(0);
}

//-------------------------------------------------------------
//  Finds the directory structure for the project.
//-------------------------------------------------------------
function SELoadDirectoryTree()
{ 
    SEDirectoryFilter.clear();
    SEDirectoryFilter.addRow(1, "/");
    %i = 0;
    %fileSpec = "*.*";
    for(%file = findFirstFile(%fileSpec); %file !$= ""; %file = findNextFile(%filespec))
    {
        // I'm sure there's a really slick way of returning all 
        // Directories in a project.  Until someone tells me what
        // that way is, you'll have to put up with this! :-)
        %base = filepath(%file);
        %addLine = 1;
        for(%ii = 0; %ii < (%i+1); %ii++)
        {
            if(%base $= SEDirectoryFilter.getRowTextById(%ii))
            {
               %addLine = 0;
            }
        }
        if(%addLine == 1)
            SEDirectoryFilter.addRow(%i++, %base);
    }
    SEDirectoryFilter.sort(0);
    SEDirectoryFilter.setSelectedRow(0);
    SEDirectoryFilter.scrollVisible(0);
}

//-------------------------------------------------------------
// Just handles selection of filters, and forces an update to the 
// listing of items in the project window.
//-------------------------------------------------------------
function SEListingFilter::OnSelect(%this, %id, %text)
{
     %dir = SEDirectoryFilter.getRowTextById(SEDirectoryFilter.getSelectedId());
     if(%dir $= "/")
     {
        %dir = "" @ %text;
     }
     else
     {
        %dir = %dir @ "/" @ %text;
     }
     SELoadProjectListing(%dir);
}

//-------------------------------------------------------------
// Handles showing files in just the subdirectory structure you
// ask for.
//-------------------------------------------------------------

function SEDirectoryFilter::OnSelect(%this, %id, %text)
{
    if(%text $= "/")
    {
        %text = "";
    }
    else
    {
        %text = %text @ "/";
    }
    %filter = %text @ SEListingFilter.getRowTextById(SEListingFilter.getSelectedId());
    SELoadProjectListing(%filter);
}

//-------------------------------------------------------------
// Triggered by the Exec button - executes the script that is
// currently selected in the Script Editor window.
//-------------------------------------------------------------
function SEExecScriptFromList()
{
    %id = SEProjectListing.getSelectedId();
    %script = getField(SEProjectListing.getRowTextById(%id), 0);
    exec(%script);
}

//-------------------------------------------------------------
// Triggered by the Load button - loads the currently selected
// script in a new script editor window.
//-------------------------------------------------------------
function SELoadProjectFromList()
{
    // We have to keep track of how many of the windows we've instanced
    $SE_activeWindows++;
    // Snag the currently selected script
    %id = SEProjectListing.getSelectedId();
    %script = getField(SEProjectListing.getRowTextById(%id), 0);
    // Ok, let's check to make sure this isn't a picture.  If it is,
    // we've got something else we need to use instead.
    %extension = strlwr(getSubStr(%script, strLen(%script) -3, 3));
    if(%extension $= "jpg" || %extension $= "gif" || %extension $= "bmp" || %extension $= "tga" || %extension $= "png")
    {
        // Setup a new bitmap view window.
        %windowID = new GuiWindowCtrl() {
            profile = "GuiWindowProfile";
            horizSizing = "right";
            vertSizing = "bottom";
            position = "219 13";
            extent = "250 250";
            minExtent = "8 2";
            visible = "1";
            maxLength = "255";
            resizeWidth = "1";
            resizeHeight = "1";
            canMove = "1";
            canClose = "1";
            canMinimize = "1";
            canMaximize = "1";
            minSize = "50 50";
            text = %script;

            new GuiBitmapCtrl() {
                profile = "GuiDefaultProfile";
                horizSizing = "width";
                vertSizing = "height";
                position = "0 23";
                extent = "247 227";
                minExtent = "8 2";
                visible = "1";
                wrap = "0";
                bitmap = %script;
            };
       };
       canvas.getContent().add(%windowID);
       %windowID.closeCommand = %windowID @ ".delete();";
       $SE_windows[$SE_ActiveWindows] = %windowID;
       return;
    }
    
    %scriptFO = new FileObject();
    %scriptFO.openForRead(%script);
    %text = "";
    // Load it into memory
    while(!%scriptFO.isEOF())
        %text = %text @ %scriptFO.readLine() @ "\n";
    // Delete the file object
    %scriptFO.delete();
    // We also need a unique name for the textEdit control. Hack :-P
    %editTextName = "SE_DYNAMIC" @ %script @ "_TEXT_";
    // Another window created from script.
    %windowID = new GuiWindowCtrl() {
        profile = "GuiWindowProfile";
        horizSizing = "right";
        vertSizing = "bottom";
        position = "219 220";
        extent = "600 400";
        minExtent = "8 2";
        visible = "1";
        maxLength = "255";
        resizeWidth = "1";
        resizeHeight = "1";
        canMove = "1";
        canClose = "1";
        canMinimize = "1";
        canMaximize = "1";
        minSize = "50 50";
        text = %script;
    };
    canvas.getcontent().add(%windowID);
    %windowID.closeCommand = %windowID @ ".delete();";
    %scrollID = new GuiScrollCtrl() {
        profile = "GuiScrollProfile";
        horizSizing = "width";
        vertSizing = "height";
        position = "0 49";
        extent = "597 344";
        minExtent = "8 2";
        visible = "1";
        willFirstRespond = "0";
        hScrollBar = "dynamic";
        vScrollBar = "dynamic";
   		     constantThumbHeight = "0";
        childMargin = "0 0";
    };
    %windowID.add(%scrollID);
    %editID = new GuiMLTextEditCtrl(%editTextName) {
        profile = "GuiSEText";
        horizSizing = "width";
        vertSizing = "height";
        position = "2 2";
        extent = "577 364";
        minExtent = "8 2";
        visible = "1";
        lineSpacing = "2";
        allowColorChars = "1";
        maxChars = "-1";
    };
    %scrollID.add(%editID);
    %editID.setText(%text);
    %otherControls = new GuiButtonCtrl() {
        profile = "GuiButtonProfile";
        horizSizing = "right";
        vertSizing = "bottom";
        position = "2 24";
        extent = "70 25";
        minExtent = "8 2";
        visible = "1";
        text = "Save";
        command = "SESave(" @ %editID @ ");";
        groupNum = "-1";
        buttonType = "PushButton";
    };
    %windowID.add(%otherControls);
    %otherControls =   new GuiButtonCtrl() {
        profile = "GuiButtonProfile";
        horizSizing = "right";
        vertSizing = "bottom";
        position = "71 24";
        extent = "70 25";
        minExtent = "8 2";
        visible = "1";
        text = "Execute";
        command = "SEExec(" @ %editID @ ");";
        groupNum = "-1";
        buttonType = "PushButton";
    };
    %windowID.add(%otherControls);
    %otherControls =  new GuiButtonCtrl() {
        profile = "GuiButtonProfile";
        horizSizing = "right";
        vertSizing = "bottom";
        position = "141 24";
        extent = "70 25";
        minExtent = "8 2";
        visible = "1";
        text = "Revert";
        command = "SERevert(" @ %editID @ ");";
        groupNum = "-1";
        buttonType = "PushButton";
    };
    %windowID.add(%otherControls);
    %findTextID =  new GuiTextEditCtrl() {
        profile = "GuiTextEditProfile";
        horizSizing = "right";
        vertSizing = "bottom";
        position = "281 24";
        extent = "70 25";
        minExtent = "8 2";
        visible = "1";
        groupNum = "-1";
    };  
    %windowID.add(%findTextID);
    %otherControls =  new GuiButtonCtrl() {
        profile = "GuiButtonProfile";
        horizSizing = "right";
        vertSizing = "bottom";
        position = "211 24";
        extent = "70 25";
        minExtent = "8 2";
        visible = "1";
        text = "Find";
        command = "SEFindText(" @ %editID @ ", " @ %findTextID @ ");";
        groupNum = "-1";
        buttonType = "PushButton";
    };
    %windowID.add(%otherControls);	
     // Keep track of our windows.  We want to kill them later.
    $SE_windows[$SE_ActiveWindows] = %windowID;
}

//-------------------------------------------------------------
//  For those moments when something is helplessly screwed.
//  Reloads the file.
//-------------------------------------------------------------
function SERevert(%textControl)
{
    %filename = SEGetFilename(%textControl);
    %scriptFO = new FileObject();
    %scriptFO.openForRead(%filename);
    %text = "";
    while(!%scriptFO.isEOF())
        %text = %text @ %scriptFO.readLine() @ "\n";
    %textControl.setText(%text);
    %scriptFO.delete();
}

//-------------------------------------------------------------
//  Writes out the edited file, then exec()'s it.
//-------------------------------------------------------------
function SEExec(%textControl)
{
    SESave(%textControl);
    %filename = SEGetFilename(%textControl);
    exec(%fileName);
}

//-------------------------------------------------------------
//  Writes out the edited file.
//-------------------------------------------------------------
function SESave(%textControl)
{
    %filename = SEGetFilename(%textControl);
    %scriptFO = new FileObject();
    %scriptFO.openForWrite(%filename);
    %scriptFO.writeline(%textControl.getText());
    %scriptFO.delete();
}
//-------------------------------------------------------------
//  Just a quick little function that extracts the filename from
//  the text control's name, and returns it.
//-------------------------------------------------------------
function SEGetFileName(%textControl)
{
    %fileName = strreplace(%textControl.getName(), "SE_DYNAMIC", "");
    %fileName = strreplace(%fileName, "_TEXT_", "");
    return(%filename);
}


//-------------------------------------------------------------
//  Just wishfull thinking for the moment.
//-------------------------------------------------------------
function SEUndo()
{


}


//-------------------------------------------------------------
//  The beginings of 'Find'.  Right now, it sort of works -
//  if will find a value, BUT, it doesn't do highlighting of 
//  found entries.  In fact, there's no way to do it currently.
//  I'll have to add yet another function to the MLText controls.
//  To make this work as much as it did, I had to add a
//  .getCursorPosition command.  It also does not scroll to the 
//  item it find.  I figured out through testing that it find it, 
//  but there's not a dang thing I can do with it yet.
//-------------------------------------------------------------
function SEFindText(%textControl, %textFindControl)
{
   %textControl.setCursorPosition(strPos(%textControl.getText(), %textFindControl.getValue(), %textControl.getCursorPosition() + 1));
   %textControl.forceReflow();
}