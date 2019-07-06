#include "ParticleEmitter.h"
#include "extern.h"
#include "Parsers.h"
#include <time.h> 

//HOW DOES TRANSFORM FEEDBACK WORK?
//A transform feedback is basically a mechanism by which OpenGL can tell the 
//output from a shader to be written back to a vertex buffer, (which is 
//probably part of a VAO.
//The transformfeedback object itself is just handle which points the shader attribute
//to the buffer id
//Procedure
//---------
//Create two VAOs 'A' and 'B'
//Create two empty transform feedbacks 'A' and 'B'
//For each VAO:
//create vertex array buffer with initial geometry
//use glbindbufferbase to bind the transform feedback id to the
// - buffer of bound VAO
// - out attribute id in shader
//e.g Transform Feedback A is bound to vertex buffer in VAO A
//
//Now when we draw:
//Bind VAO 'A' but bind Transform feedback *B*
//That way when we draw buffer of A, the out variable in vertex shader will save data to B
//then next frame bind VAO 'B' and transform feedback A 
//etc.


void ParticleSystem::init() {
	const GLchar* feedback_varyings[]{
		"v_vertex",
		"v_velocity",
		"v_age",
		"v_life"
	};

	particle_shader_ = new Shader("data/shaders/particles.vert", "data/shaders/particles.frag", 4 ,feedback_varyings);
	texture_id_ = Parsers::parseTexture("data/assets/droptexture.tga");

	
}

void ParticleSystem::create() {
	auto& emitter = ECS.getAllComponents<ParticleEmitter>();
	max_life = emitter[0].max_life;
	min_life = emitter[0].min_life;
	particle_size = emitter[0].particle_size;
	num_particles = emitter[0].num_particles;

	//tell we want textured sprites
	glEnable(GL_POINT_SPRITE);

	glEnable(GL_PROGRAM_POINT_SIZE);

	//int num_particles = 1000;
	std::srand(time(NULL));
	int vida = (int)min_life + std::rand() % (((int)max_life + 1) - (int)min_life);
	std::vector<GLfloat> vertices(emitter[0].num_particles * 3, 0);
	std::vector<GLfloat> velocities(emitter[0].num_particles * 3, 0);
	std::vector<GLfloat> ages(emitter[0].num_particles, 0);
	std::vector<GLfloat> lives(emitter[0].num_particles, 0);

	for (int i = 0; i < emitter[0].num_particles; i++) {
		lives[i] = (rand() % 9000) / 1000.0f;
	}

	//create VBO and transform feedbacks
	glGenVertexArrays(1, &vaoA_);
	glGenVertexArrays(1, &vaoB_);

	glGenTransformFeedbacks(1, &tfA_);
	glGenTransformFeedbacks(1, &tfB_);

	//from here
	glBindVertexArray(vaoA_);
	GLuint vb_A_pos, vb_A_vel, vb_A_age, vb_A_lif;

	//position buffer
	glGenBuffers(1, &vb_A_pos); //create the buffer
	glBindBuffer(GL_ARRAY_BUFFER, vb_A_pos); //use buffer
	//fill buffer with data
	glBufferData(GL_ARRAY_BUFFER,
		vertices.size() * sizeof(GLfloat),
		&(vertices[0]),
		GL_STREAM_COPY);
	glEnableVertexAttribArray(0); //layout location
	glVertexAttribPointer(0,//layout, must be the same as above
		3,//x, y, z
		GL_FLOAT,
		GL_FALSE,
		0,
		0);

	//velocity buffer
	glGenBuffers(1, &vb_A_vel); //create the buffer
	glBindBuffer(GL_ARRAY_BUFFER, vb_A_vel); //use buffer
	//fill buffer with data
	glBufferData(GL_ARRAY_BUFFER, velocities.size() * sizeof(GLfloat), &(velocities[0]), GL_STREAM_COPY);
	glEnableVertexAttribArray(1); //layout location
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

	//age buffer
	glGenBuffers(1, &vb_A_age); //create the buffer
	glBindBuffer(GL_ARRAY_BUFFER, vb_A_age); //use buffer
	//fill buffer with data
	glBufferData(GL_ARRAY_BUFFER, ages.size() * sizeof(GLfloat), &(ages[0]), GL_STREAM_COPY);
	glEnableVertexAttribArray(2); //layout location
	glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 0, 0);

	//life buffers
	glGenBuffers(1, &vb_A_lif); //create the buffer
	glBindBuffer(GL_ARRAY_BUFFER, vb_A_lif); //use buffer
	//fill buffer with data
	glBufferData(GL_ARRAY_BUFFER, lives.size() * sizeof(GLfloat), &(lives[0]), GL_STREAM_COPY);
	glEnableVertexAttribArray(3); //layout location
	glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 0, 0);

	glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, tfA_);
	glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, vb_A_pos);
	glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 1, vb_A_vel);
	glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 2, vb_A_age);
	glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 3, vb_A_lif);
	//to here

		//from here
	glBindVertexArray(vaoB_);
	GLuint vb_B_pos, vb_B_vel, vb_B_age, vb_B_lif;

	//position buffer
	glGenBuffers(1, &vb_B_pos); //create the buffer
	glBindBuffer(GL_ARRAY_BUFFER, vb_B_pos); //use buffer
	//fill buffer with data
	glBufferData(GL_ARRAY_BUFFER,
		vertices.size() * sizeof(GLfloat),
		&(vertices[0]),
		GL_STREAM_COPY);
	glEnableVertexAttribArray(0); //layout location
	glVertexAttribPointer(0,//layout, must be the same as above
		3,//x, y, z
		GL_FLOAT,
		GL_FALSE,
		0,
		0);

	//velocity buffer
	glGenBuffers(1, &vb_B_vel); //create the buffer
	glBindBuffer(GL_ARRAY_BUFFER, vb_B_vel); //use buffer
	//fill buffer with data
	glBufferData(GL_ARRAY_BUFFER, velocities.size() * sizeof(GLfloat), &(velocities[0]), GL_STREAM_COPY);
	glEnableVertexAttribArray(1); //layout location
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

	//age buffer
	glGenBuffers(1, &vb_B_age); //create the buffer
	glBindBuffer(GL_ARRAY_BUFFER, vb_B_age); //use buffer
	//fill buffer with data
	glBufferData(GL_ARRAY_BUFFER, ages.size() * sizeof(GLfloat), &(ages[0]), GL_STREAM_COPY);
	glEnableVertexAttribArray(2); //layout location
	glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 0, 0);

	//life buffers
	glGenBuffers(1, &vb_B_lif); //create the buffer
	glBindBuffer(GL_ARRAY_BUFFER, vb_B_lif); //use buffer
	//fill buffer with data
	glBufferData(GL_ARRAY_BUFFER, lives.size() * sizeof(GLfloat), &(lives[0]), GL_STREAM_COPY);
	glEnableVertexAttribArray(3); //layout location
	glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 0, 0);

	glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, tfB_);
	glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, vb_B_pos);
	glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 1, vb_B_vel);
	glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 2, vb_B_age);
	glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 3, vb_B_lif);
	//to here
}

void ParticleSystem::update() {

	

	glUseProgram(particle_shader_->program);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(GL_FALSE);

	Camera& cam = ECS.getComponentInArray<Camera>(ECS.main_camera);

	auto& emitter = ECS.getAllComponents<ParticleEmitter>();

	particle_size = emitter[0].particle_size;
	num_particles = emitter[0].num_particles;
	if (emitter[0].entra) {
		create();
		emitter[0].entra = false;
	}

	Transform& trans = ECS.getComponentFromEntity<Transform>(emitter[0].owner);

	lm::mat4 pos;

	pos.m[12] = trans.m[12];
	pos.m[13] = trans.m[13];
	pos.m[14] = trans.m[14];

	pos.m[0] = trans.m[0];
	pos.m[1] = trans.m[1];
	pos.m[2] = trans.m[2];

	pos.m[4] = trans.m[4];
	pos.m[5] = trans.m[5];
	pos.m[6] = trans.m[6];

	pos.m[8] = trans.m[8];
	pos.m[9] = trans.m[9];
	pos.m[10] = trans.m[10];

	particle_shader_->setUniform(U_MODEL, pos);
	particle_shader_->setUniform(U_VP, cam.view_projection);
	
	particle_shader_->setUniform(U_TIME, (float)glfwGetTime());
	particle_shader_->setTexture(U_DIFFUSE_MAP, texture_id_, 0);

	int viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
	float height_near_plane = std::abs(viewport[3] - viewport[1]) / (2 * tan(0.5f * cam.fov));

	particle_shader_->setUniform(U_HEIGHT_NEAR_PLANE, height_near_plane * particle_size);

	if (vaoSource == 0) {
		glBindVertexArray(vaoA_);
		glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, tfB_);
		vaoSource = 1;
	}
	else {
		glBindVertexArray(vaoB_);
		glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, tfA_);
		vaoSource = 0;
	}

	glBeginTransformFeedback(GL_POINTS);

	glDrawArrays(GL_POINTS, 0, 1000);
	glEndTransformFeedback();

	glDisable(GL_BLEND);
	glDepthMask(GL_TRUE);
}
