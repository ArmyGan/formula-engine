#include "stdafx.h"
#include "DeserializerFactory.h"
#include "Formula.h"
#include "Actions.h"
#include "PropertyBag.h"
#include "EventHandler.h"
#include "Scriptable.h"
#include "TokenPool.h"
#include "ScriptWorld.h"
#include "Parser.h"



typedef void (ScriptWorld::*RegistrationFunc)(const std::string &, Scriptable &&);
typedef Scriptable * (ScriptWorld::*LookupFunc)(unsigned);


static void LoadArrayOfEvents(const picojson::value & eventarray, ScriptWorld * world, FormulaParser * parser, Scriptable * scriptable) {
	auto & events = eventarray.get<picojson::array>();
	for(auto & evententry : events) {
		if(!evententry.is<picojson::object>())
			continue;

		auto & obj = evententry.get<picojson::object>();

		auto nameiter = obj.find("name");
		if(nameiter == obj.end() || !nameiter->second.is<std::string>())
			continue;

		const std::string & name = nameiter->second.get<std::string>();

		auto actioniter = obj.find("actions");
		if(actioniter == obj.end() || !actioniter->second.is<picojson::array>())
			continue;

		ActionSet actions;
		for(auto & actionentry : actioniter->second.get<picojson::array>()) {
			if(!actionentry.is<picojson::object>())
				continue;

			auto & action = actionentry.get<picojson::object>();
			auto actionkeyiter = action.find("action");
			if(actionkeyiter == action.end() || !actionkeyiter->second.is<std::string>())
				continue;

			const std::string & actionkey = actionkeyiter->second.get<std::string>();
			if(actionkey == "CreateListMember") {
				auto listiter = action.find("list");
				if(listiter == action.end() | !listiter->second.is<std::string>())
					continue;

				auto archetypeiter = action.find("archetype");
				if(archetypeiter == action.end() || !archetypeiter->second.is<std::string>())
					continue;

				unsigned listToken = world->GetTokenPool().AddToken(listiter->second.get<std::string>());
				unsigned archetypeToken = world->GetTokenPool().AddToken(archetypeiter->second.get<std::string>());

				actions.AddActionListSpawnEntry(listToken, archetypeToken);
			}
			else if(actionkey == "SetProperty") {
				auto propiter = action.find("property");
				if(propiter == action.end() || !propiter->second.is<std::string>())
					continue;

				auto valiter = action.find("value");
				if(valiter == action.end() || !valiter->second.is<std::string>())
					continue;

				unsigned targetToken = world->GetTokenPool().AddToken(propiter->second.get<std::string>());
				auto & payload = valiter->second.get<std::string>();

				actions.AddActionSetProperty(targetToken, parser->Parse(payload, &world->GetTokenPool()));
			}
			else if(actionkey == "RepeatEvent") {
				auto eventiter = action.find("event");
				if(eventiter == action.end() || !eventiter->second.is<std::string>())
					continue;

				auto payloaditer = action.find("count");
				if(payloaditer == action.end() || !payloaditer->second.is<std::string>())
					continue;

				unsigned eventToken = world->GetTokenPool().AddToken(eventiter->second.get<std::string>());
				auto & payload = payloaditer->second.get<std::string>();

				actions.AddActionEventRepeat(eventToken, parser->Parse(payload, &world->GetTokenPool()));
			}
		}

		unsigned eventToken = world->GetTokenPool().AddToken(name);
		scriptable->GetEvents().AddHandler(eventToken, std::move(actions));
	}
}


static void LoadArrayOfProperties(const picojson::value & proparray, ScriptWorld * world, FormulaParser * parser, Scriptable * scriptable) {
	auto & props = proparray.get<picojson::object>();
	for(auto & prop : props) {
		if(!prop.second.is<std::string>())
			continue;

		const std::string & formulastring = prop.second.get<std::string>();

		Formula formula = parser->Parse(formulastring, &world->GetTokenPool());
		scriptable->GetScopes().SetFormula(world->GetTokenPool().AddToken(prop.first), formula);
	}
}


static void LoadArrayOfScriptables(const picojson::value & node, ScriptWorld * world, RegistrationFunc func, LookupFunc lookup, FormulaParser * parser) {
	if(!node.is<picojson::array>())
		return;

	auto & scriptables = node.get<picojson::array>();
	for(auto & scriptable : scriptables) {
		if(!scriptable.is<picojson::object>())
			continue;

		auto & obj = scriptable.get<picojson::object>();
		auto nameiter = obj.find("name");
		if(nameiter == obj.end() || !nameiter->second.is<std::string>())
			continue;

		const std::string & name = nameiter->second.get<std::string>();

		Scriptable instance;

		auto propsiter = obj.find("properties");
		if(propsiter != obj.end() && propsiter->second.is<picojson::object>())
			LoadArrayOfProperties(propsiter->second, world, parser, &instance);

		auto eventiter = obj.find("events");
		if(eventiter != obj.end() && eventiter->second.is<picojson::array>())
			LoadArrayOfEvents(eventiter->second, world, parser, &instance);

		(world->*(func))(name, std::move(instance));
	}

	for(auto & scriptable : scriptables) {
		if(!scriptable.is<picojson::object>())
			continue;

		auto & obj = scriptable.get<picojson::object>();
		auto nameiter = obj.find("name");

		if(nameiter == obj.end() || !nameiter->second.is<std::string>())
			continue;

		const std::string & name = nameiter->second.get<std::string>();

		Scriptable * instance = (world->*(lookup))(world->GetTokenPool().AddToken(name));
		if(!instance)
			continue;

		auto listiter = obj.find("lists");
		if(listiter != obj.end() && listiter->second.is<picojson::array>()) {
			auto & listarray = listiter->second.get<picojson::array>();

			for(auto & listentry : listarray) {
				auto & listobj = listentry.get<picojson::object>();

				auto listnameiter = listobj.find("name");
				if(listnameiter == listobj.end() || !listnameiter->second.is<std::string>())
					continue;

				auto contentsiter = listobj.find("contents");
				if(contentsiter == listobj.end() || !contentsiter->second.is<picojson::array>())
					continue;

				const std::string & listname = listnameiter->second.get<std::string>();
				unsigned token = world->GetTokenPool().AddToken(listname);

				
				auto & contents = contentsiter->second.get<picojson::array>();
				for(auto & contententry : contents) {
					if(!contententry.is<std::string>())
						continue;

					const std::string & content = contententry.get<std::string>();
					unsigned contenttoken = world->GetTokenPool().AddToken(content);
					Scriptable * otherInstance = world->GetScriptable(contenttoken);

					if(!otherInstance)
						continue;

					instance->GetScopes().ListAddEntry(token, *otherInstance);
				}
			}
		}
	}
}



void DeserializerFactory::LoadFileIntoScriptWorld(const char filename[], ScriptWorld * world) {
	picojson::value outvalue;

	{
		std::ifstream stream(filename, std::ios::in);
		picojson::parse(outvalue, stream);
	}

	if(!outvalue.is<picojson::object>())
		return;

	FormulaParser parser;
	auto & parsedobject = outvalue.get<picojson::object>();
	for(auto & pair : parsedobject) {
		if(pair.first == "scriptables")
			LoadArrayOfScriptables(pair.second, world, &ScriptWorld::AddScriptable, &ScriptWorld::GetScriptable, &parser);
		else if(pair.first == "archetypes")
			LoadArrayOfScriptables(pair.second, world, &ScriptWorld::AddArchetype, &ScriptWorld::GetArchetype, &parser);
	}
}


