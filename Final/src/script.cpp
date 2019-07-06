#include "script.h"
#include "extern.h"
#include <time.h>
#include <cstdlib>



void script::update(float dt) {

}

void script::find_target(std::string name) {

	id = ECS.getEntity(name);

}

void script::randomTranslate() {
	Transform& target = ECS.getComponentFromEntity<Transform>(id);
	int x, y, z;

	srand(time(NULL));

	x = -100 + rand() % (100 + 100);
	y = -100 + rand() % (100 + 100);
	z = -100 + rand() % (100 + 100);

	target.translate(x, y, z);
}

void script::randomRotate() {
	Transform& target = ECS.getComponentFromEntity<Transform>(id);

	srand(time(NULL));

	float rad = 1 + rand() % (366 - 1);
	rad = rad * 3.1415 / 180;
	int x, y, z;

	x = rand() % 2;
	y = rand() % 2;
	z = rand() % 2;

	if (x == 0 && y == 0 && z == 0) {
		y = 1;
	}

	target.rotateLocal(rad, lm::vec3(x, y, z));
}

void script::randomScale() {
	Transform& target = ECS.getComponentFromEntity<Transform>(id);
	float x, y, z;

	srand(time(NULL));

	x = 1 + rand() % (3 - 1);
	y = 1 + rand() % (3 - 1);
	z = 1 + rand() % (3 - 1);

	target.scaleLocal(x, y, z);
}