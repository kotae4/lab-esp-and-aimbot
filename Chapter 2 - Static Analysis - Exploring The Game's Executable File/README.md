# Chapter 2 - Static Analysis #

Last chapter we pored over the game's code and found a lot of interesting functions, globals, and field offsets. This chapter we'll be loading the exe into our disassembler and finding the addresses of those functions and globals as well as the values of the field offsets. This is kind of like building a bridge between the game's source code and the compiled game binary.

## Looking at gl_drawhud ##

The `gl_drawhud` function we decided to use for our hook contained all kinds of global references, function calls, and field offsets, so we'll start there since that'll give us a good 90% of what we need.<br>
I start by picking a unique-looking string from the source code, I'll use `packages/misc/damage.png` as it's near the top of the function and a search through the project directory shows it is only used in this one place. I search for this in my disassembler and go to where it's referenced. Since we know it's only referenced once in the source code, we know that we're now looking at `gl_drawhud` in our disassembler. We can rename the function to help keep things organized.<br>
If we scroll to the very top of the function (at address `0x45F1C0`) we can see the global references to player1 and camera1.
<table style="table-layout:fixed">
<tr>
<th style="width:40%">Source Code</th>
<th style="width:60%">Disassembler</th>
</tr>
<tr>
<td>
  
```cpp
playerent *p = camera1->type<ENT_CAMERA ? (playerent *)camera1 : player1;
```
  
</td>
<td>

```asm
0x0045f1ed      mov     eax, dword [0x57e0a8]
0x0045f1f2      mov     ecx, dword [0x58ac00]
0x0045f1f8      mov     edi, ecx
0x0045f1fa      mov     dword [0x58a910], 0
0x0045f204      cmp     byte [eax + 0x77], 2
0x0045f208      cmovb   edi, eax
```

</td>
</tr>
</table>

So we can see that player1 is at `0x58ac00` and camera1 is at `0x57e0a8` and the `type` field offset is `0x77` and ENT_CAMERA is `2`.<br>
Since there isn't a call instruction until the call the glDisable, we can safely assume both `isspectating()` and `inrange()` were inlined. Let's go to the definition of `isspectating()` in the source code and try to match it up in the disassembler.

<table style="table-layout:fixed">
<tr>
<th style="width:40%">Source Code</th>
<th style="width:60%">Disassembler</th>
</tr>
<tr>
<td>
  
```cpp
bool isspectating() { return state==CS_SPECTATE || (state==CS_DEAD && spectatemode > SM_NONE); }
```
  
</td>
<td>

```asm
0x0045f20b      mov     al, byte [ecx + 0x76]
0x0045f20e      mov     dword [var_2ch], edi
0x0045f212      cmp     al, 5
0x0045f214      je      0x45f22a
0x0045f216      cmp     al, 1
0x0045f218      jne     0x45f223
0x0045f21a      cmp     dword [ecx + 0x318], 0
0x0045f221      jg      0x45f22a
0x0045f223      mov     byte [var_17h], 0
0x0045f228      jmp     0x45f22f
0x0045f22a      mov     byte [var_17h], 1
```

</td>
</tr>
</table>

This seems to be the entirety of the inlined `isspectating()` member function. We can see that `physent->state` field offset is at `0x76` and `playerent->spectatemode` field offset is at `0x318` and, importantly, CS_SPECTATE and CS_DEAD are `0x5` and `0x1` respectively. We can check the state field for those values to prevent our aimbot from locking onto dead / spectator players.<br>
If you're confused where physent and playerent came from, I just looked at the class definition in the source code:
```cpp
class physent
class dynent : public physent
class playerent : public dynent, public playerstate
```
And also the definition of camera1 is `extern physent *camera1;`, so camera1 and player1 will share some fields in common since they are derived from some of the same base classes. For example, both `camera1` and `player1` have a `->state` field.<br>

Moving on, the next interesting thing is the `VIRTW` global reference which is passed as an argument to a `glOrtho` call just before a call to `glEnable`. So let's look at that in both the source code and disassembler.

<table style="table-layout:fixed">
<tr>
<th style="width:35%">Source Code</th>
<th style="width:65%">Disassembler</th>
</tr>
<tr>
<td>
  
```cpp
glOrtho(0, VIRTW, VIRTH, 0, -1, 1);
```
  
</td>
<td>

```asm
0x0045f28f      movaps  xmm0, xmmword [0x55c250]
0x0045f296      sub     esp, 0x30
0x0045f299      xorps   xmm1, xmm1
0x0045f29c      movups  xmmword [var_20h], xmm0
0x0045f2a1      movsd   xmm0, qword [0x55bdc8]
0x0045f2a9      movsd   qword [var_18h], xmm1
0x0045f2af      movsd   qword [var_10h_2], xmm0
0x0045f2b5      movd    xmm0, dword [0x57ed2c]
0x0045f2bd      cvtdq2pd xmm0, xmm0
0x0045f2c1      movsd   qword [var_8h], xmm0
0x0045f2c7      movsd   qword [esp], xmm1
0x0045f2cc      call    dword [glOrtho] ; 0x52520c
```

</td>
</tr>
</table>

The disassembly may seem a little daunting but don't worry, we don't actually have to spend much brain power figuring it out. We know it's a function call and we know the function definition is `void glOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble nearVal, GLdouble farVal);` and GLdouble is just a 64-bit floating point value (all of this was obtained by googling - openGL is a well-documented graphics library). We also know that function arguments are pushed onto the stack in right-to-left order. Don't see any push instructions? That's because the arguments are being `mov`'d directly onto the stack after space is allocated with the `sub esp, 0x30` instruction. My disassembler attempts to identify local variables (variables allocated on the stack) and renames them accordingly, hence why we see `movsd qword [var_18h], xmm1` instead of `movsd qword [esp+18], xmm1` or similar.<br>
So the `mov` instruction closest to the call is the first argument as seen in the source code which in this case is `0`. The second argument is the one we're interested in, `VIRTW`, which corresponds to this instruction `movsd   qword [var_8h], xmm0` and if we look up to where xmm0 is last defined we see `movd    xmm0, dword [0x57ed2c]` so `0x57ed2c` is our global `VIRTW` variable. And we don't even have to figure out the rest because that's the only variable we care about here (remember `VIRTH` is defined as an integer literal).<br>

Now, the next interesting thing we need to find is the `playerincrosshair()` function call along with the `playerent->weaponsel` field offset and the `weapon->reloading` and `weapon->info` field offsets which are referenced just after the call (all around line 808 in the source code).<br>
We can quickly find this in the disassembly by looking just past the third `glEnable` call. While I'm scrolling down past all these other calls I'm also going to make note of the various openGL constants like `GL_TEXTURE_2D`, `GL_ONE`, `GL_ONE_MINUS_SRC_ALPHA`, `GL_MODELVIEW` and others. We could look them up in openGL's online documentation but I'm paranoid about version mismatches and would rather just look at the disassembly. It's easy to do this since they're often passed as the sole argument to these openGL function calls.<br>

So, admittedly, this part is a little hard to read as some of the calls were inlined. From that third `glEnable` call we can deduce that `playerincrosshair()` was inlined but the function that it calls (`intersectclosest(...)`) was not inlined. This is actually perfect as that's the "Raycast" function we're looking for. Its address is `0x4CA250` and it's called at address `0x45F56E`.<br>
Now let's look at the part that references playerent->weaponsel, which is just below:
<table style="table-layout:fixed">
<tr>
<th style="width:40%">Source Code</th>
<th style="width:60%">Disassembler</th>
</tr>
<tr>
<td>
  
```cpp
bool reloading = lastmillis < p->weaponsel->reloading + p->weaponsel->info.reloadtime;
```
  
</td>
<td>

```asm
0x0045f59a      mov     ecx, dword [edi + 0x364]
0x0045f5a0      mov     dword [var_3ch], eax
0x0045f5a4      mov     eax, dword [ecx + 0xc]
0x0045f5a7      movsx   eax, word [eax + 0x46]
0x0045f5ab      add     eax, dword [ecx + 0x20]
0x0045f5ae      cmp     dword [0x57f10c], eax
```

</td>
</tr>
</table>

If you were confused on where exactly this chunk starts, keep in mind that `menuvisible()` was inlined as well as `getcurcommand(NULL)`.<br>
That leaves us at the above snippet starting with `mov ecx, dword [edi + 0x364]` which is the field offset of `playerent->weaponsel`.<br>
Some compiler trickiness attempts to throw us off with `mov eax, dword [ecx + 0xc]` and make us think `0xc` is the field offset of `weapon->reloading` but we won't fall for such tricks. We can see that the next instruction is `movsx eax, word [eax + 0x46]` which makes it clear that `0xc` is the field offset of `weapon->info` and `0x46` is the field offset for `weapon->info.reloadtime` (whatever the `info` type is, haven't looked it up yet).<br>
That leaves us with `add eax, dword [ecx + 0x20]` which, finally, means the field offset for `weapon->reloading` is actually `0x20`.

And these are the openGL constants I jotted down as we scrolled past them:
```
GL_DEPTH_TEST = 0xb71
GL_MODELVIEW = 0x1700
GL_PROJECTION = 0x1701
GL_BLEND = 0xbe2
GL_SRC_ALPHA = 0x302
GL_ONE_MINUS_SRC_ALPHA = 0x303
GL_TRIANGLE_STRIP = 0x5
GL_ONE = 0x1
GL_TEXTURE_2D = 0xde1
```
Before we get to the next interesting bit, let's just nab the `CS_ALIVE` constant value first. If you'll recall the field offset of `playerent->state` was `0x76`, we can look just past the disassembly snippet we were just looking at and see `mov al, [edi+76h]` followed shortly by `test al, al` and `jz short loc_45F5C8` which corresponds partially to `if(p->state==CS_ALIVE || p->state==CS_EDITING)` in the source code which means the constant `CS_ALIVE` is `0`. We can, again, use this as a filter for our aimbot to aim only at alive enemies. Lots of ways of doing the same thing.<br>

Now, the next interesting bit is the `playerent->team` field offset which is actually very close to where we were just looking.<br>
If we scroll past the copious amount of cmp and jz instructions (that don't seem to match the source code at all, but whatever) we can see:
<table style="table-layout:fixed">
<tr>
<th style="width:40%">Source Code</th>
<th style="width:60%">Disassembler</th>
</tr>
<tr>
<td>
  
```cpp
isteam(targetplayer->team, p->team) // fragment from line 811 in renderhud.cpp
```
  
</td>
<td>

```asm
0x0045f610      mov     eax, dword [edx + 0x30c]
0x0045f616      cmp     eax, dword [edi + 0x30c]
```

</td>
</tr>
</table>

This gives us the `playerent->team` field offset at `0x30c`. Our aimbot can now be made to aim only at enemies and ignore teammates.<br>

Now, the next tidbit we need is the address of the `draw_text` function and if you recall we decided to look for the chunk starting around line 883 in the source code because this gives us the `pitch`, `yaw`, `o` fields and the `text_width` and `draw_text` functions. So let's scroll down in the disassembly until we see the string `%05.2f YAW`. I found it around address `0x46067C`.<br>
<table style="table-layout:fixed">
<tr>
<th style="width:40%">Source Code</th>
<th style="width:60%">Disassembler</th>
</tr>
<tr>
<td>
  
```cpp
// fragment of line 884 in renderhud.cpp
formatstring(text)("%05.2f YAW", camera1->yaw);
```
  
</td>
<td>

```asm
.text:0046065B  lea     eax, [esp+368h+var_318]
.text:0046065F  sub     esp, 8
.text:00460662  mov     [esp+370h+var_35C], eax
.text:00460666  mov     eax, camera1
.text:0046066B  movss   xmm0, dword ptr [eax+34h]
.text:00460670  lea     eax, [esp+370h+var_35C]
.text:00460674  cvtps2pd xmm0, xmm0
.text:00460677  movsd   qword ptr [esp+370h+x], xmm0 ; int
.text:0046067C  push    offset a052fYaw ; "%05.2f YAW"
.text:00460681  push    eax             ; int
.text:00460682  call    sub_411F40
```

</td>
</tr>
</table>

So we can see that the address of `camera1` is moved into eax and then xmm0 is loaded with the value of `camera1+0x34` which is the `yaw` field offset.<br>
The rest of the fields follow the same pattern, so if we continue looking down until we get to the string `%05.2f Z  ` we find the following offsets:
```
physent->yaw field offset @ 0x34
physent->pitch field offset @ 0x38
physent->o.x field offset @ 0x4
physent->o.y field offset @ 0x8
physent->o.z field offset @ 0xc
text_width function address @ 0x46E370
draw_text function address @ 0x46DD20
```
We're able to find the function addresses because we know formatstring is called and then the result is passed to text_width which is used in the draw_text call.<br>
We can also see that `FONTH` is part of the same draw_text calls and we know that FONTH actually just resolves to `(curfont->defaulth)` so I got address `57ED28` for the `curfont` global reference and field offset `0x18` for `curfont->defaulth`. We can also see `VIRTW` used here and confirm that the address we found earlier is correct.<br>

Now scroll all the way to the "huddigits" string for the next important tidbit:
<table style="table-layout:fixed">
<tr>
<th style="width:40%">Source Code</th>
<th style="width:60%">Disassembler</th>
</tr>
<tr>
<td>
  
```cpp
draw_textf("%d", HUDPOS_HEALTH + HUDPOS_NUMBERSPACING, 823, p->health);
if(p->armour) draw_textf("%d", HUDPOS_ARMOUR + HUDPOS_NUMBERSPACING, 823, p->armour);
```
  
</td>
<td>

```asm
0x00461628      push    dword [edi + 0xec]
0x0046162e      push    0x337      ; 823 
0x00461633      push    0x50       ; 80
0x00461635      push    0x52e994
0x0046163a      call    fcn.0046e640
0x0046163f      mov     eax, dword [edi + 0xf0]
0x00461645      add     esp, 0x10
0x00461648      test    eax, eax
0x0046164a      je      0x461664
0x0046164c      push    eax
0x0046164d      push    0x337      ; 823
0x00461652      push    0x13b      ; 315
0x00461657      push    0x52e994
0x0046165c      call    fcn.0046e640
0x00461661      add     esp, 0x10
```

</td>
</tr>
</table>

It's important to note that `edi` is the `p` variable that holds either `player1` or `camera`. In either case, it's cast as a `playerent*`. So we can see that `playerent->health` field offset is `0xec` and `playerent->armour` field offset is `0xf0`.<br>
We can also see the address of `draw_textf` is `0x46e640` and if we look into it we can see it does indeed call `draw_text` (address `0x46DD20`) so that confirms we got the right address before.<br>
Now let's snag the last stuff we need:
<table style="table-layout:fixed">
<tr>
<th style="width:40%">Source Code</th>
<th style="width:60%">Disassembler</th>
</tr>
<tr>
<td>
  
```cpp
if (p->weaponsel->type!=GUN_GRENADE) p->weaponsel->renderstats();
else if (p->prevweaponsel->type==GUN_AKIMBO || p->prevweaponsel->type==GUN_PISTOL) p->weapons[p->akimbo ? GUN_AKIMBO : GUN_PISTOL]->renderstats();
else p->weapons[getprevweaponsel(p)]->renderstats();
if(p->mag[GUN_GRENADE]) p->weapons[GUN_GRENADE]->renderstats();
```
  
</td>
<td>

```asm
0x00461683      mov     ecx, dword [edi + 0x364]
0x00461689      cmp     dword [ecx + 4], 7
0x0046168d      jne     0x4616c9
0x0046168f      mov     eax, dword [edi + 0x360]
0x00461695      mov     eax, dword [eax + 4]
0x00461698      cmp     eax, 8     ; 8
0x0046169b      je      0x4616b2
0x0046169d      cmp     eax, 1     ; 1
0x004616a0      je      0x4616b2
0x004616a2      mov     ecx, edi
0x004616a4      call    fcn.0045dd20
0x004616a9      mov     ecx, dword [edi + eax*4 + 0x33c]
0x004616b0      jmp     0x4616c9
0x004616b2      cmp     byte [edi + 0x100], 0
0x004616b9      mov     ecx, 0x340 ; 832
0x004616be      mov     eax, 0x35c ; 860
0x004616c3      cmovne  ecx, eax
0x004616c6      mov     ecx, dword [ecx + edi]
0x004616c9      mov     eax, dword [ecx]
0x004616cb      call    dword [eax + 0x38] ; 56
0x004616ce      cmp     dword [edi + 0x144], 0
0x004616d5      je      0x4616e2
0x004616d7      mov     ecx, dword [edi + 0x358]
0x004616dd      mov     eax, dword [ecx]
0x004616df      call    dword [eax + 0x38] ; 56
```

</td>
</tr>
</table>

I'm getting fatigued writing all this out so just look at the assembly and compare it to the source code and figure out how I got these:
* `weapon->type` field offset = `4`
* `GUN_GRENADE` = `7`
* `playerent->prevweaponsel` field offset = `0x360`
* `GUN_AKIMBO` = `8`
* `GUN_PISTOL` = `1`
* `playerent->akimbo` field offset = `0x100`
* `playerent->weapons` field offset = `0x33c`
* `playerent->mag` field offset is somewhere around `0x144`

The only thing we can't reliably glean is the mag field offset because in the source code we see `if(p->mag[GUN_GRENADE])` and the compiler optimized the indexing operation so the result is hardcoded into the assembly. We know `GUN_GRENADE` is `7` and we know `mag` is defined as `int ammo[NUMGUNS], mag[NUMGUNS], gunwait[NUMGUNS];` (line 257 in entity.h, remember `playerent` is derived from `dynent` and `playerstate`). So we can probably assume it starts at offset `0x128` but don't take my word for it. This is why we confirm things with dynamic analysis afterwards.<br>

We're all done with the `gl_drawhud` function! Now we move on to `gl_drawframe`...

## Harvesting gl_drawframe ##

We can find `gl_drawframe` by looking at what calls `gl_drawhud`. This should be an option in your disassembler.<br>
In IDA you can select the function name and press 'X' to see what references it.<br>
In Ghidra you can select the function name and press 'Ctrl+Shift+F' to see what references it.<br>

Arriving at the call to `gl_drawhud` inside `gl_drawframe` (at address `0x456EDE`) I can see a lot of assembly and I don't like it. Scrolling to the top of the function (at address `0x4560E0`) I can tell a lot of calls have been inlined. Thankfully, we're only here to grab `mvpmatrix` which is supposed to be referenced in a call to `readmatrices()` but I have a feeling it's inlined. Either way, it occurs right after a `glMatrixMode` call near the top of the function. It should be the second `glMatrixMode` call unless it's also called from an inlined function.<br>
So scrolling from the very top of the function and trying to follow along in the source code, I see the sse2 lib functions (tan, atan2, sinf, etc) which serves as a good marker of where I am.<br>
Finally I see the second `glMatrixMode` call and above it I see the `setperspective(...)` call hasn't been inlined, so we could nab that function address but I'm not sure we'll need it (we can just write it ourselves).<br>
Below, I see the `transplayer()` function hasn't been inlined either but we definitely won't need this as we can easily implement it ourselves.<br>
Finally, we see that the `readmatrices()` call has indeed been inlined. We can tell by looking at the source code of the `readmatrices` function and see that it starts with two calls to `glGetFloatv` which matches what we see in the disassembly.<br>
It's not so terrible to read though, the non-literal argument to each `glGetFloatv` call is the `mvmatrix.v` and `projmatrix.v` respectively. We know that these are multiplied together shortly after and the result is stored in the `mvpmatrix` which is the global reference we're trying to find.

<table style="table-layout:fixed">
<tr>
<th style="width:40%">Source Code</th>
<th style="width:60%">Disassembler</th>
</tr>
<tr>
<td>
  
```cpp
glGetFloatv(GL_MODELVIEW_MATRIX, mvmatrix.v);
glGetFloatv(GL_PROJECTION_MATRIX, projmatrix.v);
// ... (omitted because they're long, inlined matrix operations)
mvpmatrix.mul(projmatrix, mvmatrix);
```
  
</td>
<td>

```asm
0x0045640f      mov     esi, dword [glGetFloatv] ; 0x52521c
0x00456415      push    0x57e010   ; mvmatrix
0x0045641a      push    0xba6      ; 2982
0x0045641f      call    esi
0x00456421      push    0x57e0b0   ; projmatrix
0x00456426      push    0xba7      ; 2983
0x0045642b      call    esi
0x0045642d      movss   xmm0, dword [mvmatrix] ; 0x57e010
0x00456435      mov     ecx, 0x57dfd0
...             ...     ...        ... ; omitted
0x004564c8      push    0x57e010   ; mvmatrix ; int32_t arg_8h
0x004564cd      xorps   xmm0, xmm1
0x004564d0      push    0x57e0b0   ; projmatrix ; int32_t arg_4h
0x004564d5      movss   dword [0x592078], xmm0
0x004564dd      call    fcn.004114b0
```

</td>
</tr>
</table>

So I had to omit some of the inlined matrix operations in the middle there. These are really obvious to see in the disassembly as it's always a sea of `movss` instructions.<br>
So we can see our two known matrices, `mvmatrix` and `projmatrix` are pushed as arguments to a call. This call corresponds to `mvpmatrix.mul(projmatrix, mvmatrix);` and surprisingly hasn't been inlined. Since this is a class member function we know that the implicit 'this' parameter is always in register ecx. What is ecx last set to? `0x57dfd0` is the address of our `mvpmatrix`.<br>

And that's all we set out to find in `gl_drawframe` so we can move on to the next (and last) function. When we get to coding the ESP functionality we may decide to revisit this `gl_drawframe` function to find some more potentially useful variables or functions so I suggest you name it in your disassembler and save the project so you can easily return to it.


## Devouring kickbot ##

The last thing we need to find is our "Player list" which is actually exclusively the collection of bot entities. We decided to use the string `bot %s disconnected` even though it's used in two places because either place will get us the `bots` global reference. So search for that string in the disassembler and find what references it, then go to the first reference. For me, this brings me to address `0x5120E9`.<br>
I can already see the array we're looking for because I recognize the `lea ecx, [eax+edx*4]` pattern. This is how arrays are commonly looped in assembly. `eax` holds the base address of the array, `edx` is the index into the array, and `4` is the size of each element in the array. I see this instruction just below the string reference, at address `0x512125` and just above that I see `mov eax, dword_591FCC` which means our `bots` global reference is at address `0x591FCC`.<br>
At the bottom of the loop, at address `0x512136` I see:
```asm
0x0051212e      mov     edi, dword [0x591fd4]
0x00512134      cmp     edx, edi
0x00512136      jl      0x512120
```
This gives us the 'ulen' field of the vector class (@ address `0x591fd4`) which represents the number of elements. It's given as an absolute address rather than a field offset, but this is somewhat common for simple container instances defined in the global scope.<br>
We should now have all we need! We may find out later that we're missing some stuff, so be sure to name all of the functions we visited and save your disassembler database so we can quickly return to it later.


## Summary ##

That concludes this lengthy and meticulous chapter on static analysis. It took me about 12 hours to write this chapter alone. I intentionally chose places in the disassembly that'd be easy to turn into a snippet (I only had to cut some fat out of one snippet!) and easy to find for those that are following along.<br>
You should have named all of the functions we visited so we can return to them later if need be (we probably will need to). Don't forget to save your disassembler project before exiting. And close those notepad++ tabs.