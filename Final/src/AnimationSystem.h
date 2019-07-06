#pragma once
#include "includes.h"
#include "Shader.h"
#include "Components.h"

class AnimationSystem {
public:
	float ms;
    ~AnimationSystem();
    void init();
    void lateInit();
    void update(float dt);
    
private:
    GLuint curr_frame_ = 0;
    float ms_per_frame_ = 41.666f;
    float ms_counter_ = 0;
    
    void incrementJointFrame_(Joint* joint);
    
    void deformBlendShapes_();
    
};
