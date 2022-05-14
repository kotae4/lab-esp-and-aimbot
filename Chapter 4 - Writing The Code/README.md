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
You can find it here: https://github.com/kotae4/lab-esp-and-aimbot/tree/Part1Chapter4 <br>

Next, we need to add some meat to it. The first thing I always try to do is to draw the string "Hello World" on screen. So let's add a call to  to our `hk_gl_drawhud` function in `CheatMain.cpp`. We can see the default parameters in the game's source code are 255 for color component parameters and -1 for the cursor and maxwidth parameters, so here's what my call looks like: `CheatMain::odraw_text("Hello World", 10, 10, 255, 255, 255, 255, -1, -1);`. Now we're ready to compile and inject it into the game.<br>
I use cheat engine's built-in injector. To do so, open up cheat engine and in the upper left corner click the glowing computer icon and select the game process. Then, click the 'Memory View' button near the bottom left of the main cheat engine window. In the popped up Memory View window open the 'Tools' menu then 'Inject DLL' at the bottom of the list. Select our compiled DLL and when cheat engine asks if you'd like to execute a function click 'No' (DllMain still gets executed, don't worry).<br>

## A Snag With Drawing Text ##
The game crashes immediately. A slight snag in our plans, but this is okay. This is common when you're messing with low-level stuff. So we know we could successfully inject before we added the call to `odraw_text`, so that already narrows it down perfectly. Let's compare how our call looks in assembly against how the call looks in the game's assembly. I load up two instances of my disassembler and load the cheat DLL into one and the game's exe into the other.<br>
So by looking at all the calls to draw_text in the game I can see it's definitely a non-standard calling convention. It looks like the string argument is loaded into the ecx register and the 'left' or X-coordinate argument is loaded into the edx register. The rest of the arguments are pushed onto the stack in reverse order.<br>
If we look at our compiled DLL we can see all the arguments are pushed, none are loaded into registers, hence the crash.<br>
So let's try writing a "wrapper" around `odraw_text` that mimics the assembly we see in the game. Thankfully, since this is x86, we can use `__asm` blocks to write assembly instructions directly. If it were x64 we'd have to use something like asmjit. So here's what I came up with:
```cpp
void CheatMain::draw_text(const char* str, int x, int y, int r, int g, int b, int a /*= 255*/, int cursor /*= -1*/, int maxwidth /*= -1*/)
{
	__asm
	{
		mov ecx, str
		mov edx, x
		push maxwidth
		push cursor
		push a
		push b
		push g
		push r
		push y
		call odraw_text
		add esp, 28
	}
}
```
And I call it like this at the top of `hk_gl_drawhud`, before we call the trampoline:
```cpp
CheatMain::draw_text("Hello World", 1000, 1000, 255, 0, 0);
```

Restarting the game and injecting it again we see that we don't crash anymore, but we also don't see our string being drawn anywhere. Painful.<br>
So now my idea is to go in game and place a breakpoint on the `draw_text` function (remember it's at address `0x46DD20`). We can do this in cheat engine's 'Memory View' window.<br>
The first hit for me is the "FPS 200" string and I can see I got the calling convention right. If I plug the address contained in ECX into ReClass.NET I see the "FPS 200" string. If I plug the address contained in ESP (the stack pointer) into ReClass.NET I can see the arguments that were pushed onto the stack in the same order as we're pushing ours. Everything is as it should be, but we still aren't seeing our string. Now I'm thinking maybe let's just mimic the parameter values of that call, so I change my call to:
```cpp
CheatMain::draw_text("Hello World", 4273, 3504, 255, 255, 255);
```

And after restarting the game and injecting again, I still don't see my string. Very strange. Ah, the graphics state isn't prepared for HUD drawing when our `hk_gl_drawhud` is executed.<br>
So if we look at the game's `gl_drawhud` function in `renderhud.cpp` we can see some initial work at the start:
```cpp
glDisable(GL_DEPTH_TEST);
glMatrixMode(GL_MODELVIEW);
glLoadIdentity();
glMatrixMode(GL_PROJECTION);
glLoadIdentity();
...
glEnable(GL_BLEND);
```
This disables depth, clears the mv and p matrices, and enables blending which is used to get rid of the black background of the font atlas. If we scroll further down to where the first `draw_text` calls start and then look above that for more GL stuff we see:
```cpp
glEnable(GL_TEXTURE_2D);
glOrtho(0, VIRTW*2, VIRTH*2, 0, -1, 1);
```
This allows us to draw with 2d textures (which is what the font atlas is) and multiplies the current matrix (the last set matrix mode - projection in this case) by an orthographic transformation matrix.<br>
So I'm going to move all of this into a new function, `CheatMain::SetupHUDDrawing`, and call that before we start drawing text.<br>
And I just realized I have a bug in my code. In `opengl_wrapper.h` where I define all the GL constants, I accidentally placed an `=` sign between the token and the value. The correct definitions should be:
```cpp
#define GL_DEPTH_TEST 0xb71
#define GL_MODELVIEW 0x1700
#define GL_PROJECTION 0x1701
#define GL_BLEND 0xbe2
#define GL_SRC_ALPHA 0x302
#define GL_ONE_MINUS_SRC_ALPHA 0x303
#define GL_TRIANGLE_STRIP 0x5
#define GL_ONE 0x1
#define GL_TEXTURE_2D 0xde1
```
I also realized I didn't define VIRTH anywhere even though we know it's a constant, so I added `#define VIRTH 1800` to my `CheatMain.h` file.<br>
With those changes, we can compile and inject again, and finally we see our string in the lower right.<br>
We can change the coordinates now so it draws in the top-left. Here's my `hk_gl_drawhud` function now:
```cpp
CheatMain::SetupHUDDrawing();
CheatMain::draw_text("Hello World", 100, 100, 255, 255, 255);

CheatMain::draw_textf(100, 200, 255, 255, 0, "MyModule: %tx", CheatMain::hMod);

CheatMain::ogl_drawhud_trampoline(w, h, curfps, nquads, curvert, underwater, elapsed);
```
I went ahead and added a formatted version of our draw_text wrapper, as you can see. It just writes the formatted varargs into a char buffer then passes that to CheatMain::draw_text.<br>
Finally, a last note, the coordinates passed to these drawing functions are not 1:1 screen coordinates. It seems the game uses a "virtual" coordinate system, hence the presence of `VIRTW` and `VIRTH`. We could translate this virtual coordinate system back to our screenspace coordinate system, but I don't think it's entirely necessary for HUD drawing. Perhaps as we continue toward our goals we'll find a need to do so, I'm not sure yet.<br>
Anyway, I'll push this commit now before we move on to the rest.<br>
You can view the codebase at this point in time here: https://github.com/kotae4/lab-esp-and-aimbot/tree/Part2Chapter4 <br>