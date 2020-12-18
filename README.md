## rerun-obs-binding

This library wraps a small portion of libOBS and exposes it to Node.JS apps via the Node native addons API.
The output of this project is a .node file you can `require()` in Javascript. [Rerun](https://github.com/ChapC/rerun) uses it to manage its own instance of [OBS](https://github.com/obsproject/obs-studio).

[RerunOBSBinding.ts](src/RerunOBSBinding.ts) shows the functions this binding currently supports. 
At the moment it's designed only for use in Rerun, so it's missing a few features that prevent it from being a fully-featured binding. Namely,

- Scene creation/destruction. Rerun only uses one scene, accessible via `getMainScene`.
- Proper separation between Sources and SceneItems. There are classes already set up for these objects, but since only one scene ever exists and Rerun doesn't duplicate any sources, each source maintains a reference to its single SceneItem to make certain things easier. This reference would have to be removed to support multiple scenes.
- On-the-fly configuration of A/V encoders. Currently they are set up once during the `init` function and the instance must be restarted to modify them.
- Manual control of Outputs and Services. The binding is hard-coded to hook A/V encoders to RTMP output to RTMP service, both with default settings. The only exposed controls of these components are the `startStream` and `stopStream` functions, where you can pass the RTMP service an address and a stream key.

Implementing these features shouldn't be too tricky, I just haven't because Rerun just doesn't need them at the moment.

I've left lots of comments in here as I've trudged my way through OBS and node-addon-api, so I hope this repo can be useful to anyone who's interested in learning about either. Feel free to open any issues for questions, or submit a pull request if you'd like to take a swing at one of the missing features.

### Building
*Disclaimer - I know very little about C++ build systems and they make me ✨very scared💫*

The library is built using node-gyp. It requires NPM, Python, and few files from a build of OBS. I *hope* this is cross-platform (for any platform Node + OBS supports), but I've only tested it on Windows 10.

Here's how you can build the thing:
1. Clone this repository and `cd` into it from the command line.
2. Run `npm install` to install the binding's node dependencies.
3. Create the obs and obs/src folders in the root of the repository.
4. Copy the the contents of the libobs folder from the [OBS repository](https://github.com/obsproject/obs-studio) into obs/src/libobs.
5. In the obs folder, place the `obs.lib` file from a build of OBS for your platform. You can find build instructions on the [obs-studio wiki](https://github.com/obsproject/obs-studio/wiki/Install-Instructions). The .lib file you're looking for should be built in obs-studio/build/libOBS/{Debug or Release}.
6. Run `node-gyp configure` to generate C++ project files.
7. Run `npm build` to build the binding.

The output binary links with OBS at runtime, so you'll need to have built OBS libraries (found in the build/rundir/Debug/bin/64bit folder of the OBS project) in the same folder as RerunOBSBinding.node when running your app. If Node is giving you "The specified module could not be found" when you `require()` the module, it's probably having trouble finding some of those libraries.

### License
ISC License

Copyright (c) 2020, Chap Callanan

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
