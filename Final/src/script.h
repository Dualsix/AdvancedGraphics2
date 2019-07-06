#pragma once
#include "ScriptSystem.h"


class script : public Script {

public:

	script(int owner) : Script(owner) {}

	void update(float dt);

	void find_target(std::string name);
	void randomTranslate();
	void randomRotate();
	void randomScale();

private:

	int id = -1;

};