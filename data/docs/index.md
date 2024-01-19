# Simcoe Engine {#index}

## Core features needed
* RHI across d3d11 and d3d12
    * maybe other apis but probably not needed
* Client-Server networking
    * Cooperative authority model
* Async model internally
    * Need good scaling on larger machines
    * Green threads/fibers
    * Ideally cpu layout aware
* Simple 3d graphics
    * Targeting d3d11 so no fancy compute
* An actual content pipeline
    * Biggest pain point of old engine
    * Need codegen of some form
        * Network transport forms
        * On disk forms
        * Save files
        * Probably more
    * Use cthulhu to manage dsls and generating boilerplate
        * Maybe make use of flatbuffers as an intermediate form
        * Protocolbuffers and gRPC also look nice
* A better editor
    * More than just debug info
    * Content authoring
    * Asset bake pipeline
    * Can still do asset authoring in 3rd party tools
        * Blender mostly

## Game features
* Slow paced co-op game
    * Swat4, ready or not, rainbow 6, etc
* online co-op
    * will do manual ip setup for now
    * discoverability is hard (and expensive)
* 3 maps
    * lobby level
        * select mission
        * choose loadouts
        * pick roles
        * discuss tactics
        * test weapons
    * house of some sort
        * small easy level
        * good starter mission
    * factory or other large building
        * harder mission
        * more enemies
        * more objectives

* required assets
    * player models
        * 2 or 3 should be enough
        * only slight variations in textures
    * enemy models
        * probably 1 per map
    * hostage models
        * maybe?
    * weapons
        * 2 primary
        * 2 secondary
        * a lethal gadget
        * a tactical gadget
    * map geometry
    * map textures
    * will do the most simple animations possible
    * basic foley
