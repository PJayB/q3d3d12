Quake 3: Arena on DirectX 12
============================

*Includes Desktop, UWP and HoloLens Support*

Introduction
------------

_February, 2022_

Hi,

This project was an ongoing effort from 2014-2016. I found this while going through some old backups, and I realized that I had never open-sourced it. At the time, I used Quake 3 as a means to get to better know the various platforms I worked with.

If you're looking to start a Quake 3-based project of your own, I don't think I'd recommend forking this. As I mentioned above, this was a solo project I used to learn various platform and graphics APIs. It also contains many changes that aren't relevant to that goal that I probably wrote for funsies. The only situation where I think this code might have value is as reference for a Vulkan-based port of Q3A. If you're looking for a well-maintained foundation from which to start a project, I recommend you head over to [ioquake3](https://ioquake3.org/).

Additionally, before anyone gets too excited by the words "HoloLens support", I mean that this code runs on HoloLens in a virtual window. It is not a "full 3D" augmented reality port.

If you're still curious, I've written up some notes that hopefully help explain some of the major differences to its parent project, my [Direct3D 11 port of Quake 3](https://github.com/PJayB/Quake-III-Arena-D3D11). All of this was written from my memory of creating this 5+ years ago, so please don't take my notes as gospel; it's possible I may have forgotten some important detail that may lead you astray if you don't verify these notes for yourself.

This originally started out as a Direct3D 11 port of Quake 3, and I added DirectX 12 later. The D3D11 code is still in here, but I've no idea if it works. Before uploading this to GitHub I quickly checked that the Desktop Visual Studio 2015 project loaded, upgraded, built and ran against Visual Studio 2019 in DirectX 12 mode. I haven't checked the HoloLens projects.

Enjoy,

-- Pete

## Platform Support

The [Direct3D 11 port of Quake 3](https://github.com/PJayB/Quake-III-Arena-D3D11) -- from which this is forked -- brought along 64-bit and ARM architecture support, XAudio2, XInput and WinRT support. Most importantly, it abstracted the rendering code from the rest of the engine and game code, making DirectX 12 a drop-in.

A few features from that project were dropped for simplicity, e.g. the "proxy" renderer that could handle rendering the game twice using two different graphics APIs in side-by-side windows, which was useful, but it's usefulness didn't outweigh the effort it took to maintain it.

Due to the existing work done for the Windows 8 port ([code/win8](code/win8)), the HoloLens ([code/hololens](code/hololens)), and UWP ([code/uwp](code/uwp)) ports were quite easy due to all these platforms being based on WinRT ([code/winrt](code/winrt)). However, I'm a little disappointed how much code I copy-pasta'd among all of these.

## DirectX 12

Much of the challenge in the original Direct3D 11 port of the game was batching all the immediate-mode OpenGL commands that Quake issues. The general premise of the solution is that I accumulate `glVertex*` calls into a giant ringbuffer, and then draw everything all at once. I have multiple ringbuffers depending on the vertex layout, and each one has an associated shader responsible for rendering that buffer.

For the DirectX 12 version, I really had to double-down on the batching. DirectX 12 requires that we know almost all rendering state up-front, and this bundle of state is known as a Pipeline State Object (PSO). As such, my approach was to precompute as many of the common combinations of rendering state as possible at initialization time. The combinations were determined by static analysis[^1] of the code, and then baking those in to the renderer initialization. For example:

	CachePipelineStateBits( __FUNCTION__, &s_PSO, 0, CT_TWO_SIDED, FALSE, FALSE );
	CachePipelineStateBits( __FUNCTION__, &s_PSO, GLS_DEFAULT, CT_TWO_SIDED, FALSE, FALSE );
	CachePipelineStateBits( __FUNCTION__, &s_PSO, GLS_SRCBLEND_SRC_ALPHA|GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA|GLS_DEPTHTEST_DISABLE, CT_TWO_SIDED, FALSE, FALSE );

Any PSO cache misses are computed at run-time and cached to disk for next time.

As part of this, I had to simmer down all of Quake's possible rendering state into some kind of hash, so that I could look up PSOs for a given draw call. The hash is a 64-bit integer aliased to the `PIPELINE_DRAW_STATE_DESC` struct. This is composed of:

* Quake's `GLS` state bits found in [tr_state.h](code/renderer/tr_state.h) (e.g. `GLS_*BLEND_*`).
* The culling mode (none, CW, CCW)
* Polygon offset for decals and such (1 bit Boolean)
* Outline (which I think is only used for `r_showTris`)

Quake also has a few different modes of rendering:

* Single-textured, unlit (e.g. UI, HUD, skybox)
* Single-textured with lightmap
* Multi-textured with lightmap
* Dynamic lit surfaces, which are additively blended over the top of one of the above for each

Over the course of the frame, I accumulate all of Quake's immediate mode rendering requests, and based on the state bits that are set, associate it with a PSO, and sort the request into a compatible queue. Check out the `code/d3d12/GenericDrawStage*.*` files for more info on those.

In addition to batching all the state, there was also the matter of creating and setting descriptors. These are needed to associate resources with a draw call, e.g. textures and samplers. In OpenGL this is a simple `glBindTexture` call but alas, in DX12, life is not so simple. Descriptors are sub-allocated from a fixed-size pool ([DescriptorBatch](code/d3d12/DescriptorBatch.cpp)) each frame, and at the end of the frame, the descriptor pool is "reset" (`count` is set to zero).

There's a lot of code here, and I think this writeup has already overflowed most people's attention buffers. Much of what I haven't already covered here is a straight DX12 port from the D3D11 version. However, there's likely some interesting nuggets around fog rendering, CPU/GPU synchronization, transparency handling, etc. I'll leave the rest as an exercise for the reader.

## QABI

The QABI stuff is likely one of the more curveball changes in here. Honestly, I do not fully remember why I wrote it, but I _think_ it was to simplify cross-VM calls as well as provide me much better type safety. All the exports from both the engine and each of the VM DLLs (e.g. `game`, `cgame`, `ui`) are declared in `code/qabi`. During the build, the QAbiGen tool spits out headers that (1) have a LUT that maps function indices to function pointers, (2) have functions that wrap the relevant `VM_CallA` invocations.

For example, this QABI definition,

	abi (CGVM_ConsoleCommand) qboolean CG_ConsoleCommand( void );

On the engine-side, this is invoked like so:

	qboolean CGVM_ConsoleCommand( void ) {
		vmArg_t r = {0};
		vmArg_t args[2];
		args[0].p = (size_t) QABI_CGAME_CGVM_CONSOLE_COMMAND;
		r.p = VM_CallA( cgvm, args );
		return (qboolean) r.i;
	}

And on the VM-side, this command is mapped to a `cgame` function invocation:

	static size_t QDECL __QABI_CGVM_ConsoleCommand( vmArg_t* args ) {
		vmArg_t r = { 0 };
		r.i = (int) CG_ConsoleCommand();
		return r.p;
	}

## Visual Studio Project Generation

I've added the project files to the Git tree for the convenience of anyone who wants to quickly check out the code, but the canonical workflow was to actually generate these. This was done by a tool I wrote, `tools/ProjectGen`. There isn't too much else to say on this, except that I remembered writing this to work around differing support for various platform features and MSVC versions across Desktop, XBO, UWP and HL at the time.

Original ReadMe
===============

Installation
------------
- Install retail Quake 3: Arena (and optionally Quake 3: Team Arena)
- Create an environment variable called `Q3INSTALLDIR` and point it at your Quake 3 installation directory
- Load `projects\Q3_`*VS Flavor*`_`*Platform*`.sln`
- Set your platform and configuration
- Build Solution
- Set *quake3* as Startup Project

Running on PC
-------------
- Go to *quake3 Project Properties > Debugging*
- Set your *Command Arguments* to:

	`+set fs_userpath "." +set fs_basepath "$(OutDir)" +set fs_cdpath "$(Q3INSTALLDIR)" +set r_fullscreen 0 +set dx12_debug 1 +devmap testbed`

- Set *Working Directory* to:

	`$(SolutionDir)..`

- F5.


[^1]: Grepping is technically static analysis... right?
[^2]: It appears I never wrote a "Running on HoloLens" or "Running on UWP" section.
