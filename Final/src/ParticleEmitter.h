//
//  Copyright � 2018 Alun Evans. All rights reserved.
//
#pragma once
#include "includes.h"
#include "Shader.h"
#include "Components.h"

class ParticleSystem {
public:
	
	void init();
	//void init(int num_particles);
	void create();
	void update();
private:

	float min_life;
	float max_life;
	float particle_size;
	int num_particles;
	GLuint vaoA_, vaoB_;
	GLuint tfA_, tfB_;
	GLuint vaoSource = 0;
	Shader* particle_shader_;
	GLuint texture_id_;
};