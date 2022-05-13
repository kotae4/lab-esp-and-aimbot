# Chapter 1 - Defining Objectives & Gathering Information

First step in any project is to define your goals. I like to think of the broad goals first and then break each one up into more detailed goals where applicable.

## Goals ##

1. ESP functionality
    * Box ESP (draw box around each player)
    * Name ESP (draw player names above their head)
2. Aimbot functionality
    * Aim at nearest enemy player (by distance in game world)
    * Skip players that are behind walls
3. Inject cheat into game process & execute
    * Using cheat engine's injector is fine

## Checklists ##

Aside from goals, I also like defining and keeping track of the implementation-specific details we need in order to complete the goals.<br>
I like to keep the top-level of these checklists as general as possible so they can be applied to any game cheat project.

### Rendering Checklist ###

- [x] Hook a drawing function
    * `gl_drawhud` seems like the best function to hook, contains a lot of strings to search for in disassembler
- [x] World-To-Screen (w2s)
    * `mvpmatrix` global referenced in `readmatrices()` function called by `gl_drawframe`
- [x] Box rendering (or line rendering and we'll construct our own box)
    * We found `box2d`, `box`, and `line` utility functions in rendergl.cpp, but it's easier to just write our own
- [x] Text rendering
    * `gl_drawhud` contains calls to both `draw_text` and `draw_textf`

### Data Checklist ###

- [x] Local player instance
    * `player1` and `cam1` global references found in `gl_drawhud` (and lots of other places)
- [x] Player list (including start, size, and how to iterate it if it's a complicated structure)
    * `vector` structure, global reference to `bots` found in `kickbot` (and lots of other places)
    * Use string `bot %s disconnected` to find in disassembler

### Aimbot Checklist ###

- [x] Raycast / Traceline (so you don't aim through walls)
    * `CBot::CheckStuck` calls `TraceLine` and is near string `Randomly avoiding stuck...`
- [x] Set view angles (any method of making the player aim at our target)
    * `camera1`'s pitch and yaw can be set, which we found in `gl_drawhud`

I'll be marking the above checklists off as we go throughout the project and adding details, but I'll also provide the blank "template" checklist below so you can use them.

### (Blank) Rendering Checklist ###

- [ ] Hook a drawing function
- [ ] World-To-Screen (w2s)
- [ ] Box rendering
- [ ] Text rendering

### (Blank) Data Checklist ###

- [ ] Local player instance
- [ ] Player list (including start, size, and how to iterate it if it's a complicated structure)

### (Blank) Aimbot Checklist ###

- [ ] Raycast / Traceline (so you don't aim through walls)
- [ ] Set view angles (any method of making the player aim at our target)

Feel free to copy the blank checklists over to your own project.

## Gathering Information ##

With our goals and checklists defined, we can now begin the research phase of the project.<br>
We'll start by exploring the game's codebase since it's an open source project. This can also be done for Unity engine games built on the mono backend using an IL disassembler/decompiler and several other games or game engines. Access to a target's codebase, or partial codebase, is surprisingly common and helps tremendously.<br>
Assault cube's codebase can be found here: https://github.com/assaultcube/AC/tree/v1.3.0.2

So, since our first goal is ESP functionality and we know we need to draw on the screen we can start by searching for the word "draw" in all .c/.cpp files<br>
To do so, I use notepad++ and its excellent 'Find in Files' search tool. I point it at the `/source/src` directory as that's where the core game code seems to be located.<br>
I get 239 hits in 15 files for the word "draw". We can be pretty confident that the core drawing functions aren't going to be located in a file named "botmanager.cpp" or anything like that, so we can manually filter these results pretty quickly.<br>
The first semi-interesting result I see is in the `main.cpp` file and it's a call to a `gl_drawframe` function (line 1572). I assume this call is in the main loop of the game application which would be perfect for our needs (we want to hook a draw function that gets called every frame).<br>
After looking into it I can see it is within an infinite `(for;;)` loop (lines 1522-1597) of the application's `main()` function (lines 1208-1605), so this is our first interesting function to note.<br>
I'm now interested in seeing the implementation of this function so I search for `gl_drawframe` in notepad++ (or if you open the project in an IDE you can probably just right-click -> go to definition).<br>
Looking at its implementation in `rendergl.cpp` (lines 1019-1167) I can see it does a lot of openGL state management before calling various game drawing functions, so hooking this function won't be perfectly ideal because we'd have to re-create the openGL state ourselves. That's not necessarily a deal-breaker but for now I want to look for something that already has an ideal graphics state set up.<br>
Scrolling to the bottom of the `gl_drawframe` function I can see it calls a function named `gl_drawhud` (line 1161). The HUD would be a nice place to draw some of our cheat's debug info (such as a "Hello World" message to let us know we've got everything working) and we could even implement our ESP features on the HUD but that would look kind of ugly as everything would be sized the same even if the target player is far away from our local player.<br>
Before looking at the implementation of `gl_drawhud`, I'll make note of how player models are drawn further up in this `gl_drawframe` function in case we do need to manually set up the graphics state for our ESP features.<br>
All of the glXXX calls leading up to `renderclients()` call at line 1121 are important as we can see these prepare the graphics state for drawing the players in the world. We can see how the far plane and the vertical fov is calculated, as well as how the camera is rotated in the world. We also see two very interestingly named functions: `transplayer()` and `readmatrices()` called at lines 1059 and 1060 just after the switch to GL_MODELVIEW matrix mode and before the call to `render_world` (line 1080). So we'll definitely have to investigate all of this later, but for now it's good enough just to jot them down.<br>
Looking at the implementation of `gl_drawhud` in renderhud.cpp (lines 750-1173) now I can immediately see some juicy details: 
```cpp
playerent *p = camera1->type<ENT_CAMERA ? (playerent *)camera1 : player1;
```
Two globals: `camera1` and `player1` will definitely be useful later. We can assume player1, being a global, is the local player instance which means we can check that off our checklist above.<br>

Continuing to scroll through this function we see all sorts of goodies:
* Call to `playerincrosshair()` at line 805, interesting because of its name: it might have something to do with raycast / traceline which we'll need to prevent our aimbot from locking onto targets behind walls
* `p->weaponsel` and `p->weaponsel->info.reloadtime` at line 808 which will give us our offsets to the player's current weapon and the info associated with that weapon
* `p->state==CS_ALIVE` at line 809 which gives us the offset to the 'state' of player struct and the constant denoting CS_ALIVE: this will be helpful for our aimbot as we can filter out players that aren't alive
* `targetplayer->team` at line 811 gives us the offset to the 'team' of player struct. Helpful for both our aimbot and ESP, we can now choose to lock on to only enemy players and also draw enemies and teammates using different colors.

And, another checklist item, drawing text at line 832:
```cpp
else if(infostr)
{
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, VIRTW * 2, VIRTH * 2, 0, -1, 1);
    glScalef(1.0, 1.0, 1.0); //set scale
    draw_text(infostr, 48, VIRTH * 2 - 3 * FONTH);
    glPopMatrix();
}
```
We see not only the call to `draw_text` but also exactly how the graphics state is set up for drawing text. We'll look into the draw_text implementation later to make sure it does what we expect it to do. We can also look at `VIRTW`, `VIRTH` and `FONTH` to see if those will be compiled to literals or global references in assembly.<br>
Just below that, around line 864 we start to see more strings too, and at line 884 we see some pretty interesting ones:
```cpp
pushfont("mono");
formatstring(text)("%05.2f YAW", camera1->yaw);     draw_text(text, VIRTW*2 - ( text_width(text) + FONTH ), VIRTH*2 - 17*FONTH/2);
formatstring(text)("%05.2f PIT", camera1->pitch);   draw_text(text, VIRTW*2 - ( text_width(text) + FONTH ), VIRTH*2 - 15*FONTH/2);
formatstring(text)("%05.2f X  ", camera1->o.x);     draw_text(text, VIRTW*2 - ( text_width(text) + FONTH ), VIRTH*2 - 13*FONTH/2);
formatstring(text)("%05.2f Y  ", camera1->o.y);     draw_text(text, VIRTW*2 - ( text_width(text) + FONTH ), VIRTH*2 - 11*FONTH/2);
formatstring(text)("%05.2f Z  ", camera1->o.z);     draw_text(text, VIRTW*2 - ( text_width(text) + FONTH ), VIRTH*2 - 9*FONTH/2);
popfont();
```
This gets us the `physent->yaw`, `physent->pitch`, and `physent->o` field offsets which represent the rotation and position of the object. We can set the pitch and yaw to force our player character to aim a certain direction, and knowing the position of each player is vital to both aimbot and ESP functionality. Since this little chunk also contains plenty of easy-to-find strings and calls to `draw_text` and `text_width` we'll probably look for this in the disassembler as opposed to the first `draw_text` call we saw around line 832.

Near the bottom of this `gl_drawhud` function we see:
```cpp
pushfont("huddigits");
draw_textf("%d", HUDPOS_HEALTH + HUDPOS_NUMBERSPACING, 823, p->health);
if(p->armour) draw_textf("%d", HUDPOS_ARMOUR + HUDPOS_NUMBERSPACING, 823, p->armour);
if(p->weaponsel && valid_weapon(p->weaponsel->type))
{
    glMatrixMode(GL_MODELVIEW);
    if (p->weaponsel->type!=GUN_GRENADE) p->weaponsel->renderstats();
    else if (p->prevweaponsel->type==GUN_AKIMBO || p->prevweaponsel->type==GUN_PISTOL) p->weapons[p->akimbo ? GUN_AKIMBO : GUN_PISTOL]->renderstats();
    else p->weapons[getprevweaponsel(p)]->renderstats();
    if(p->mag[GUN_GRENADE]) p->weapons[GUN_GRENADE]->renderstats();
    glMatrixMode(GL_PROJECTION);
}
popfont();
```
Which gets us a potentially more useful text drawing function (the f usually means 'formatted' such as in printf), as well as: the player struct's health and armour offset, the player struct's `mag` and `weapons` array offsets, the weapon struct's `type` offset and some constants (we can search for those in the source files to see all the others too).<br>
We can also note down the `pushfont` and `popfont` functions but I don't really care which font we're rendering with, so I'll leave that as a nice-to-have feature for later implementation.<br>

That concludes our gleaning of the `gl_drawhud` function. I think it's a good hooking candidate too, it has a lot of strings that should make it easy to find in our disassembler. So we can cross "Hook a drawing function" off of our checklist as well as "Text rendering".<br>
Since `gl_drawframe` also contained some interesting functions and we know that it calls `gl_drawhud` I'm curious if we can find `gl_drawframe` just by looking at `gl_drawhud`'s callers, so I'll do a quick search for `gl_drawhud` and see if it's called anywhere else.<br>
It turns out it's only called one place, which is inside `gl_drawframe`, so we can find `gl_drawframe` by looking at `gl_drawhud`'s references in our disassembler later. This will allow us to find those interesting tidbits we discovered earlier much quicker.<br>

Before moving on, let's quickly look into all those things that we noted down:
* `draw_text` / `draw_textf` functions
    * Both are declared in protos.h file at line 725, and defined in rendertext.cpp at line 385 and line 99
    * We can see draw_textf just calls draw_text after formatting the variadic arguments which is something we can implement ourselves
* `VIRTW`, `VIRTH`, `FONTH`
    * All declared in protos.h at lines 714-718
    * VIRTW is declared as a global non-const integer so we'll need to read it in memory
    * VIRTH is a preprocessor symbol defined as integer literal 1800.
    * FONTH is a preprocessor symbol resolving to (curfont->defaulth)
        * curfont is defined as a global ptr to a font object (just below VIRTW's definition too) so we'll have to read that from memory if we want to use it
* `playerincrosshair()`
    * Defined in weapon.cpp at line 371
    * Has a lot hardcoded (`player1` and `camera1`)
    * The function it calls, `intersectclosest`, will probably be better as we can pass it a vector to check from
* `transplayer()`
    * Defined in rendergl.cpp at line 595
    * Just prepares graphics state using `camera1`'s position and rotation.
    * Small enough to be inlined by compiler, so be wary of that.
* `readmatrices()`
    * Defined in rendergl.cpp at line 978
    * Looks like it constructs `mvpmatrix` and all other matrices and spatial variables like `camright`, `camup`, and `camdir`
    * Maybe small enough to be inlined by compiler, but doubtful
    * Can read these matrices from memory if we find their addresses

The `mvpmatrix` global referenced in the `readmatrices()` function lets us check off "World-To-Screen" from our checklist. We can also check off "Text rendering" for sure now that we've investigated it further.<br>
So looking at the checklist now we still need some kind of line or box drawing function, a Raycast / Traceline function, and a method for making our localplayer face a target. And, perhaps most importantly, the list of entities to target.<br>
We could easily implement our own line/box drawing function using openGL but let's see if we can find a game function first. My first thought is to look for the function that draws the radar because it has an opaque black background that doesn't seem like it'd be part of the map texture.<br>
So looking back at the `gl_drawhud` function I see a call to `drawradar` at line 921. Its implementation is at line 693 and it calls two functions: `drawradar_showmap` and `drawradar_vicinity` so let's look at the first one, found at line 464.<br>
We're looking for anything that sounds like it'd draw a line, a box, a rectangle, or anything like that.<br>
I see the `quad` call at line 498 which sounds like a rectangle, but I know that a quad is often a textured rectangle and we don't want to draw textures. I'll make note of it in case we can't find anything else. Maybe it'll be defined in some utility file that's close to other primitive drawing functions.<br>
Let's look at the `drawradar_vicinity` function now at line 569. Again, looking for any function call that sounds box-related.<br>
I see a call to `circle` but it seems to take in a texture parameter. Still, "circle" seems like it'd be defined very close to "rectangle" doesn't it? Plainly named functions like "circle" and "quad" make me think "utility functions" and who wouldn't make a utility function that draws a line or rectangle? So I'll look at its implementation and scroll through to see if I can find any such other functions.<br>
I see `circle` function defined in rendergl.cpp at line 275. I see `quad` defined right above it so my theory of these being "utility functions" is already looking good. Above `quad` I see `box2d` then `box` and finally `line`. All the functions we could need. Now we have to see if they're called anywhere in the game code, if they aren't then they'll be very difficult to find in a disassembler (assuming the compiler didn't just omit them).<br>
On second thought, these functions don't do anything special so why not just write our own? We can check "Box rendering" off our checklist now.

So next comes the Raycast / Traceline. So a good tip for finding this is to go in-game and see if you can find any kind of pop-up that displays only when you're looking at something. Or maybe you can interact with something in the world if you're looking at it and press an 'interact' key. In assault cube, we can see the player name pop up in the lower right corner of the screen when we're targeting one, and we can see this behaviour comes from `gl_drawhud` (line 841) and relies on the function `playerincrosshair()`. From looking at that function (defined in weapon.cpp at line 371) we can see it calls `intersectclosest` which seems like it'd work perfectly for us. So we can check off "Raycast / Traceline" from our checklist.<br>

The next item on our checklists: a way to make our localplayer aim at our target. This should be really easy, we should be able to just set the camera's pitch and yaw after we calculate the angle between us and the enemy, and since we've already found `camera1` in the `gl_drawhud` function (and a lot of its field offsets) we can go ahead and check that off our checklist too.<br>

Finally, the only item remaining is to find the list of entities that we want to aim at. Since assault cube is technically a multiplayer game there's the potential that some less mature people will use this lab to ruin the enjoyment of others in multiplayer matches. To guard against that, I won't be searching for the list of player entities, but rather only bot entities. Bots can be added to singleplayer games while offline and this is the ideal situation for us. We do not want to ruin anyone's multiplayer experience or give the gamedevs a headache in any way.<br>
So, if you go in game and start up a bot match (Esc -> Singleplayer -> Bot team deathmatch -> whatever else) then you'll see a message in the top left: "Bot connected: \<bot name>". That's a clue! We can search for that string in the source code and find it's used on line 937 in botmanager.cpp. Now we could look into the BotManager.CreateBot function and probably find what we're looking for, but looking at this file I can see a bunch of bot-related console commands are being registered, so I'm going to scroll down and see if I can find a really small function that loops the bots as that'll probably be easier for us to find in the disassembly later.<br>
So scrolling down I see the string "bot %s disconnected" and I see this is used in both the `kickbot` and `kickallbots` functions, but it doesn't matter which one we look at in the disassembly as they both loop a `bots` global reference and that's what we're looking for. Now we can cross the final item off our checklist: the "Player list".

So now we're all done! We've gathered all the information we need to fulfill our goals, and now all that's left is the implementation. The next chapter in this incredibly dry and meticulous novel will be finding all these functions in our disassembler and also finding all the global references and field offsets that we need.<br>

## Closing ##

Hopefully this chapter - as meticulous as it is - shines light on how you can go about familiarizing yourself with a large codebase. Being able to read other people's code is an incredibly valuable skill as a programmer and reverse engineer, so feel free to explore the codebase more on your own: it's a great exercise.