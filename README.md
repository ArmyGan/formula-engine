# formula-engine
## A simple but powerful embeddable scripting library for games

The Formula Engine is designed to operate around a small yet highly composable set of primitives. These are:

 * Properties, which are defined by Formulas
 * Entities, which can be organized into Lists
 * Events, as well as Actions (which are performed in response to events)

By using these primitives judiciously, it is possible to express quite rich game systems and game logic without needing heavy bindings from engine code to script code. Moreover, the project aims to demonstrate a moderately detailed simulation using these tools alongside a stripped-down grid-based game engine.

Code is exclusively C++11; data is exclusively JSON. Examples of the input scripts can be found in the `Data` directory.

<br>

### Properties and Formulas
Properties are simply numeric data, either statically defined or dynamically computed. Static properties are useful for describing data-driven aspects of a design. Dynamic properties are expressed in terms of _formulas_, which are simply arithmetic expressions such as `1 + (health * 4)`.

The implementation of f## formula-engine: a simple scripting approach for games

The Formula Engine is designed to operate around a small yet highly composable set of primitives. These are:

 * Properties, which are defined by Formulas
 * Entities, which can be organized into Lists
 * Events, as well as Actions (which are performed in response to events)

By using these primitives judiciously, it is possible to express quite rich game systems and game logic without needing heavy bindings from engine code to script code. Moreover, the project aims to demonstrate a moderately detailed simulation using these tools alongside a stripped-down grid-based game engine.

Code is exclusively C++11; data is exclusively JSON. Examples of the input scripts can be found in the `Data` directory.

<br>

### Properties and Formulas
Properties are simply numeric data, either statically defined or dynamically computed. Static properties are useful for describing data-driven aspects of a design. Dynamic properties are expressed in terms of _formulas_, which are simply arithmetic expressions such as `1 + (health * 4)`.

The implementation of formulas can be found in `Formula.h` and `Formula.cpp` under the `FormulaEngine` directory. Formulas are often (but not always!) parsed from text strings, using a class implemented in `Parser.h` and `Parser.cpp`.

Groups of properties are implemented as so-called "bags" and the code for them can be found in `PropertyBag.h` and `PropertyBag.cpp`.

<br>

### Entities and Lists
Entities are primarily used as groupings of properties, although they are also used as targets of Events. They are organized into Lists as a mechanism of representing ownership or composition. Entities do not carry much behavior of their own; they are mostly there for organizational purposes.

<br>

### Events and Actions
When the game engine detects that something has occurred which might be of interest to a script, it propagates an _event_. In response to these events, entities may fire a sequence of _actions_. Unlike most traditional game scripting models, actions are not a rich set of verbs exposed by the game engine. Instead, actions fall into a relatively small set of highly generic behaviors that modify critical data used by the game engine itself.

<br>

## FAQ

 1. **How do I do (x) in this paradigm?**
Please stand by and watch this space. The ideas behind the Formula Engine are still undergoing a lot of refinement, so examples for a wide variety of use cases are still being worked out.

 2. **How do I represent things that happen over time?**
The non-answer is that _you don't_. A more accurate reflection of things would look something like this: events, and the actions that respond to them, are immediate and atemporal. They do not model the notion of time elapsing, and this is on purpose. One of the goals of this project is to build a simulation that _does_ handle complex time-based scripts (such as strategies or plans). The honest answer is that we don't know yet how that will play out.

 3. **Is there more documentation?**
Not yet. This is still a very nascent project and it'll get fleshed out more as time goes on.

 4. **How do I embed this in my own game?**
For now, the basics of what you need are in the `Formula Engine` filter in the Visual Studio solution. Examples of composing and constructing larger systems from those basic ingredients can be found in various places, such as `Tests.cpp`. In general the stuff that lives in `Game Engine` and `Support` filters can be replaced with whatever engine you're working with.

## Future Plans

There should be a PLAN.md file in the root directory with relatively up-to-date thoughts and notes on where this whole thing is heading.