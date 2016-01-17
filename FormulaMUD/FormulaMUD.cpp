// FormulaMUD.cpp : Defines the entry point for the console application.
//

#include "Pch.h"

#include "Room.h"
#include "CommandTable.h"
#include "WorldState.h"
#include "Bindings.h"


int main() {
	TokenPool tokens;

	Game::CommandTable commands("Data\\CommandList.json", &tokens);
	Game::WorldState state;

	state.commands = &commands;

	Game::Binder binder(&tokens, &state);

	ScriptWorld world(&tokens, &binder);

	DeserializerFactory::LoadFileIntoScriptWorld("Data\\FormulaMUD.json", &world);

	Game::RoomNetwork roomNetwork("Data\\Rooms.json", &tokens, &world, &state);
	state.roomNetwork = &roomNetwork;

	world.QueueBroadcastEvent("OnUserConnect");
	while (world.DispatchEvents());


	while (state.alive) {
		world.QueueBroadcastEvent("Poll");
		while (world.DispatchEvents());
	}
		

    return 0;
}


