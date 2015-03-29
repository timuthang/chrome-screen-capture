### Why does this extension require accessing all data on my computer? ###
It's because this extension uses an NPAPI plugin to save your screenshot to your computer. As long as you install an extension using an NPAPI plugin, Chrome will warn you like this.

### Why can't I sync this extension access multiple computers? ###
Based on the security concern, Chrome doesn't sync any extension which contains an NPAPI plugin.

### Why can't I capture region or whole page on new tabs, extension gallery pages or local HTML pages? ###
Because we can't inject content script in these pages, we can't capture region or whole page on them. If you want to capture an extension gallery page, you can start Chrome with the --allow-scripting-gallery flag.

### Why can't this extension capture Flash? ###
A Flash object has a property named “wmode”. The default value of wmode is “window”. To capture a Flash, its wmode property need to be “opaque”.

### Why can't I install this extension on Chrome OS? ###
This extension contains an NPAPI plugin, which is unsupported on Chrome OS.