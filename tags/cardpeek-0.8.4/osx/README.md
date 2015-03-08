Experimental MacOSX Cardpeek bundle
===================================

Context
-------

In order to integrate Cardpeek in OSX, we need to generate an application bundle (i.e. cardpeek.app).
To this end:

1. We modified the autoconf files to allow this specific approach. 

2. We compile cardpeek through the objective-c program `cardpeek-osx.m` instead of the normal `cardpeek.c`.

3. We use gtk-mac-bundler (with the pacth below), with the adequate bundle, icns and plist files.

Caveats
-------

This has gone through very limited testing.
Currently Cardpeek does not have a menu is OSX and does not have the quit shortcut.

Patching gtk-mac-bundler
------------------------
 
Below between markdown triple quotes is the patch to apply to gtk-mac-bundler in order to make this work. 

```
diff --git a/bundler/bundler.py b/bundler/bundler.py
index 8061ac1..d9b16af 100644
--- a/bundler/bundler.py
+++ b/bundler/bundler.py
@@ -247,7 +247,7 @@ class Bundler:
                 relative_dest = self.project.evaluate_path(Path.source[m.end():])
                 dest = self.project.get_bundle_path("Contents/Resources", relative_dest)
             else:
-                print "Invalid bundle file, missing or invalid 'dest' property: " + Path.dest
+                print "Invalid bundle file, missing or invalid 'dest' property: " + source
                 sys.exit(1)
 
         (dest_parent, dest_tail) = os.path.split(dest)
@@ -302,7 +302,7 @@ class Bundler:
                         dir_util.copy_tree (str(globbed_source), str(dest),
                                             preserve_mode=1,
                                             preserve_times=1,
-                                            preserve_symlinks=1,
+                                            preserve_symlinks=0,
                                             update=1,
                                             verbose=1,
                                             dry_run=0)
@@ -612,7 +612,7 @@ class Bundler:
         self.copy_plist()
 
         # Note: could move this to xml file...
-        self.copy_path(Path("${prefix}/lib/charset.alias"))
+        #self.copy_path(Path("${prefix}/lib/charset.alias"))
 
         # Main binary
         path = self.project.get_main_binary()
```
