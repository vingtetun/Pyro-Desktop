

var winWatcherSvc = Components.classes["@mozilla.org/embedcomp/window-watcher;1"]
    .getService(Components.interfaces.nsIWindowWatcher);

try {
    var gconfSvc = Components.classes['@mozilla.org/gnome-gconf-service;1'].getService(
        Components.interfaces.nsIGConfService);
} catch(e) {
}


function _getPrompt ()
{
    return winWatcherSvc.getNewPrompter (window);
}


function promptReplaceWindowManager ()
{
    const buttonFlags = Components.interfaces.nsIPrompt.STD_YES_NO_BUTTONS;

    //var dialogText = GetStringFromName ("replaceWindowManagerText");
    //var dialogTitle = GetStringFromName ("replaceWindowManagerTitle");

    var dialogTitle = "Replace Window Manager";
    var dialogText = "A window manager is already running on this desktop.\n" +
                     "Would you like to replace it with Pyro?";

    // returns 0 for yes, 1 for no.
    var result = _getPrompt().confirmEx (dialogTitle,
                                         dialogText,
                                         buttonFlags,
                                         null, null, null,
                                         null, {});
    return !result;
}


function promptSetDefaultWindowManager ()
{
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
        var dialogText = "Pyro is not set as your default window manager.\n" +
                         "Would you like to make Pyro the default for future sessions?";

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
