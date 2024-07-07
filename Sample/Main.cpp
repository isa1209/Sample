#include "World.h"

int main(int argc, char *argv[])
{
	World world;

	bool success = world.Initialize();

	if (success)
	{
		world.RunLoop();
	}

	world.ShutDown();

	return 0;
}
