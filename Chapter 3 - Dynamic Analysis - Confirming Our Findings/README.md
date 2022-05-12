# Chapter 3 - Dynamic Analysis #

In this chapter we'll be loading up cheat engine and ReClass.NET to confirm our findings and map out some data structures.<br>
To do so, we'll be plugging in all the addresses we recorded last chapter and making sure they match our expectations.

Here's all the functions and global references we found from the previous chapter:
* `gl_drawhud` function @ address `0x45F1C0`
* `player1` global reference @ address `0x58ac00`
* `camera1` global reference @ address `0x57e0a8`
* `VIRTW` global reference @ address `0x57ed2c`
* `intersectclosest` function @ address `0x4CA250`
* `text_width` function @ address `0x46E370`
* `draw_text` function @ address `0x46DD20`
* `curfont` global reference @ `0x57ED28`
* `draw_textf` function @ address `0x46e640`
* `gl_drawframe` function @ address `0x4560E0`
* `mvpmatrix` global reference @ address `0x57dfd0`
* `bots` global reference @ address `0x591FCC` (vector of playerents)
* `bots->ulen` global reference @ address `0x591fd4` (length field of vector of playerents)

And the field offsets of some structs:
* `playerent->type` @ field offset `0x77`
* `playerent->state` @ field offset `0x76`
* `playerent->spectatemode` @ field offset `0x318`
* `playerent->weaponsel` @ field offset `0x364`
* `weapon->info` @ field offset `0xc`
* `weapon->info.reloadtime` @ field offset `0x46`
* `weapon->reloading` @ field offset `0x20`
* `playerent->team` @ field offset `0x30c`
* `font->defaulth` @ field offset `0x18`
* `playerent->health` field offset @ `0xec`
* `playerent->armour` field offset @ `0xf0`
* `weapon->type` field offset @ `0x4`
* `playerent->prevweaponsel` field offset @ `0x360`
* `playerent->akimbo` field offset @ `0x100`
* `playerent->weapons` field offset @ `0x33c`
* `playerent->mag` field offset somewhere around `0x144`, maybe `0x128`.

## Refresher on Function Addresses and Image Bases ##

So a few things to note before we get started:<br>

With compiled languages like C/C++, functions are always going to be at the same relative position within the image. The function has already been compiled to machine code and this machine code is placed within the .text section of the image file.<br>
With interpreted languages or languages that compile to an intermediate language before being Just-In-Time (JIT) compiled, functions are dynamically allocated and will be at a random address. The JITer parses the input code and produces machine code then allocates enough space in the process to hold it via VirtualAlloc (or `new` or malloc or whatever else eventually leads to a VirtualAlloc call).<br>

The PE image format has a flag for "dynamic base". When this flag is set then Windows image loader will allocate the image at a randomized base address. This is part of ASLR and the randomized base address will change each time you restart your PC.<br>
Because of randomized base addresses, we often use image offsets for addressing. For example, let's say we load the image into our disassembler at a base address of 0x400000 and we find our `gl_drawhud` function at address `0x45F1C0` in our disassembler. Since the image base is `0x400000` we'd subtract that to get the image offset: `0x45F1C0` - `0x400000` = `0x5F1C0` and we'd add `0x5F1C0` to the randomized base address of the game process to find `gl_drawhud` in the game. For example, let's say windows decided to give the process a random base of `0x501000`, we'd do: `0x501000` + `0x5F1C0` = `0x5601C0` and this would get us `gl_drawhud`.<br>
This idea of subtracting the base to find the offset and then adding that offset to other bases to normalize everything is very important. I often set the base to 0x0 in my disassembler so all addresses are also their image offset.

Now, Assault Cube is written in C/C++ which is a compiled language and it does not have the dynamic base flag set, so we know that its base address will be `0x400000` every single time. This means there's no real need to use the base+offset paradigm. We can use the absolute addresses if we want because as long as our disassembler is basing the image at `0x400000` (it should be) then the addresses in the disassembler will match the addresses in the live game too.

With all that said, we only really need to confirm the field offsets and try to map out the rest of those data structures. We don't need to verify that the function addresses are valid because there's no way they aren't.

## Confirming Our Findings ##

Let's start by loading into the game then launching cheat engine and ReClass.NET and attaching both to the game's process.<br>
We'll look at our local player first, `player1` at address `0x58ac00`. So plug that into ReClass.NET since we're just trying to map out data structures. We can also pull up the class definition of playerent in the `entity.h` file of the game's source code. We may have to look at the dynent, physent, and playerstate definitions too which are all in the same file too.<br>
Remember, `player1` is defined as `playerent *player` so it's a pointer. We want to follow that pointer in ReClass.NET to the actual `playerent` instance. We can do that by selecting the first field and pressing the 'PTR' button then expand the drop-down twice to see the actual playerent instance.<br>
The first field is the vtable which is a pointer to somewhere in the \<DATA> section.<br>
The second field, at `0x4`, if you recall is the `playerent->o` field. "o" in this case is short for "origin" which is the player's position. We know that positions are very often stored as 3 floats (a vec3, vector3, etc). We can confirm by looking at the definition of `physent` in `entity.h` and we see that the first defined field is indeed `vec o, vel`. "vel" in this case is short for "velocity" so we can also map that. We can do this by selecting the field in ReClass.NET and pressing the 'VEC 3' button. Do this for both the `o` field and the `vel` field. If you go into the game and run around a bit you should see both `o` and `vel` changing in ReClass.NET. We can see that the 'z' component of the vectors corresponds to the height which is somewhat rare in games. Usually the 'y' component is height.<br>
Continue mapping out the data structures and making sure the known offsets we found match up with the mapping. Refer to the source code definitions but don't rely on them completely as the published build may use a slightly different codebase. You should also be checking that they're changing according to in-game actions.

## TO-DO
confirm playerent->mag offset

## Summary ##

This chapter is really short because there isn't much to do. In some games this part might be the longest part, but in this case most of our time went to reading the source code and looking at the game in a disassembler.<br>
Next chapter we'll finally start coding the thing.