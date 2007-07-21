

function _getPrompt ()
{
    var winWatcherSvc = Components.classes["@mozilla.org/embedcomp/window-watcher;1"]
                                  .getService(Components.interfaces.nsIWindowWatcher);
    return winWatcherSvc.getNewPrompter (window);
}


function promptReplaceWindowManager ()
{
    if (getBoolPref ("compzilla.replace_existing_wm", false))
	return true;

    const nsIPrompt = Components.interfaces.nsIPrompt;
    const buttonFlags = 
        nsIPrompt.BUTTON_TITLE_IS_STRING * nsIPrompt.BUTTON_POS_0 + 
        nsIPrompt.BUTTON_TITLE_CANCEL * nsIPrompt.BUTTON_POS_1 + 
        nsIPrompt.BUTTON_POS_1_DEFAULT;

    //var dialogText = GetStringFromName ("replaceWindowManagerText");
    //var dialogTitle = GetStringFromName ("replaceWindowManagerTitle");

    var dialogTitle = "Replace Window Manager";
    var dialogText = "A desktop window manager is already running.\n" +
                     "Would you like to replace it with Pyro Desktop?";
    var neverAgainText = "Always perform this check when starting";
    var neverAgainVal = { value: true };

    // returns 0 for yes, 1 for no.
    var result = _getPrompt().confirmEx (dialogTitle,
                                         dialogText,
                                         buttonFlags,
                                         "Replace", null, null,
                                         neverAgainText, neverAgainVal);

    if (!neverAgainVal.value) {
        var prefs = GetPrefs ();
        prefs.setBoolPref ("compzilla.replace_existing_wm", true);
    }

    return !result;
}


function promptSetDefaultWindowManager ()
{
    try {
        var gconfSvc = Components.classes['@mozilla.org/gnome-gconf-service;1']
                                 .getService(Components.interfaces.nsIGConfService);
    } catch (e) {
    }
    if (!gconfSvc)
        return;

    const buttonFlags = Components.interfaces.nsIPrompt.STD_YES_NO_BUTTONS;
    const defaultWMKey = "/desktop/gnome/applications/window_manager/default";

    var existingWM = "";
    try {
        existingWM = gconfSvc.getString (defaultWMKey);
    } catch (e) {
    }

    if (existingWM == "" || existingWM.indexOf ("compzilla") == -1) {    
        //var dialogText = GetStringFromName ("setDefaultWindowManagerText");
        //var dialogTitle = GetStringFromName ("setDefaultWindowManagerTitle");

        var dialogTitle  = "Set Default Window Manager";
        var dialogText = "Pyro Desktop is not set as your default window manager.\n" +
                         "Use it by default for future sessions?";

        // returns 0 for yes, 1 for no.
        var result = _getPrompt().confirmEx (dialogTitle,
                                             dialogText,
                                             buttonFlags,
                                             null, null, null,
                                             null, {});

        if (result == 0) {
            try {
                gconfSvc.setString (defaultWMKey, 
                                    "/home/agraveley/run-compzilla.sh");
            } catch (e) {
                alert ("Error calling gconfSvc.setString: " + e);
            }
        }
    }
}
