
// Disable session-restore prompt
pref("browser.sessionstore.resume_from_crash", false);

// Autostart compzilla when the first browser window opens
// FIXME: Once we stabalize, this should default to true
pref("compzilla.autostart", false);

// If another window manager is running, just replace it instead of asking
pref("compzilla.replace_existing_wm", false);
