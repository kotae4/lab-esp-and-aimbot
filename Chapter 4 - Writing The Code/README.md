# Chapter 4 - Writing The Code #

Finally, the moment we've all been waiting for: coding the thing! A lot of people think programming is all about writing code, but those people are fools. If 90% of your time spent on a project is writing code then you're almost certainly writing awful code. Research and planning are huge parts of any project which includes what we were doing in the previous chapters.<br>
Unfortunately, I plan on writing some awful code for this project, but it's at least planned. Hear me out: writing direct, concise code that doesn't take advantage of OOP or any other design paradigms is going to be easier to follow as a beginner than some massive, abstract cheating framework.<br>

Start by creating a Visual Studio project. Use the 'Dynamic-Link Library (DLL)' template under C++ | Windows | Library filter settings. From there, I add in my `#include <MinHook>` statement to the precompiled header file `pch.h` (did you install minhook via vcpkg as recommended at the start?). I also set the 'C++ Language Standard' under 'Project->\<projName> Properties->General' to 'Preview - Features from the Latest C++ Working Draft (/std:c++latest)' for all configurations. Finally, since we know assault cube is x86, I got rid of the x64 build configuration under 'Build->Configuration Manager...' and under the 'Active solution platform' dropdown I chose '\<Edit...>' and selected x64 and removed it.<br>

Finally, it's worth noting that if you're using the code from this repo you will need to install minhook via vcpkg using the x86-windows-static triplet as I modified the vcxproj file to use that triplet (the `VcpkgTriplet` option in 'Globals' PropertyGroup).

## Giving Shape To Our Data ##

So first thing we're going to do is complete the "bridge" we've been building between the game, its compiled form, and now our cheat. We take all the functions and field offsets we gathered before and define them in our program.<br>
The data structures we mapped out in ReClass.NET in the previous chapter can be exported to C/C++ struct definitions which takes care of a lot of the work. To do so, in ReClass.NET, click on the 'Project' tab in the menu toolbar then 'Generate C++ Code...'. This will pop up a window for you to then select all and copy.<br>
In your Visual Studio project, create a new header file (I named mine `game_definitions.h`) and paste. You can fix up the definitions if you want. For example, I decided to make the proper inheritance chain so that `playerent` is derived from both `dynent` (which itself is derived from `physent`) and `playerstate`. I also wrote in the virtual method definitions so Visual Studio would see the vtables and correctly factor that into the total size of the classes. I also have definitions for `animstate`, `poshist`, `weapon`, and `guninfo`. I expect padding to mess up some of these definitions, but that's an easy fix if we encounter such a problem.<br>
In this `game_definitions.h` file I'm also going to include some typedef'd function definitions.<br>
The functions we gathered from the game and will be using are:
* `gl_drawhud` function @ address `0x45F1C0`
* `intersectclosest` function @ address `0x4CA250`
* `text_width` function @ address `0x46E370`
* `draw_text` function @ address `0x46DD20`


Next, we'll also need some openGL functions. We could `#include` openGL in our project, but I'd rather just provide the function definitions and then get the function addresses via `GetProcAddress`. Thinking about what we'll need from openGL and looking at what the game uses (mostly in the game's `renderhud.cpp` and `rendergl.cpp` files), these are the functions I defined:
* `glMatrixMode`
* `glPushMatrix`
* `glPopMatrix`
* `glLoadIdentity`
* `glOrtho`
* `glFrustum`
* `glEnable`
* `glDisable`
* `glBlendFunc`
* `glBegin`
* `glEnd`
* `glColor4f`
* `glVertex2f`
* `glVertex3f`
* `glRotatef`
* `glScalef`
* `glTranslatef`
* `glGetFloatv`
    * might not need since we can read mvpmatrix directly

And since I plan on getting these addresses via `GetProcAddress` calls I decided to move these definitions into their own files: `opengl_wrapper.h` and `opengl_wrapper.cpp`. We'll define a static class to access these functions and also perform all of our `GetProcAddress` calls in its static constructor (or maybe a static `Initialize` method). We could even place our custom drawing functions in this static class too, like `DrawOutline` or something. This way all of our drawing stuff is in its own class.<br>
We can get the exact openGL function definitions from its online documentation.<br>

Finally, we need something to tie this all together, so I created a class called `CheatMain` that will hold all the global references and function pointers we found from the game. This class will also perform all initialization and all cheat-related functionality: it will be the core of our program. Since we'll be doing all of our work from within hooks, we won't have an object reference. We could make a global instance of `CheatMain` and use that, or we could simply make `CheatMain` a pure static class. I'll be choosing to make it purely static.<br>
These are the global references we need to store a pointer to:
* `player1` global reference @ address `0x58ac00`
* `camera1` global reference @ address `0x57e0a8`
* `VIRTW` global reference @ address `0x57ed2c`
* `curfont` global reference @ `0x57ED28`
* `mvpmatrix` global reference @ address `0x57dfd0`
* `bots` global reference @ address `0x591FCC` (vector of playerents)

And of course we'll also store pointers to the functions whose signatures we defined in the `game_definitions.h` file.<br>
I'll include some helpful utility functions, like allocating a console window to print to.<br>

I'll push a commit containing all the work I've done so far so you can reference the code at this exact moment in time.<br>