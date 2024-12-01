#include "stdafx.hpp"

#if 0
#include "project/cube.hpp"

#include "game/game.hpp"

using namespace sm;
using namespace sm::project;

Cube::Cube(game::ClassSetup& setup)
    : Entity(setup)
{
    mWidth = 1.0f;
    mHeight = 1.0f;
    mDepth = 1.0f;
}

void Cube::create() {
    // auto& ctx = game::getContext();

    // world::Cube cube = { .width = mWidth, .height = mHeight, .depth = mDepth };
}

void Cube::destroy() {

}

void Cube::update(float dt) {

}
#endif