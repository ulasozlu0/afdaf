#pragma once
#include "../Core/Structs.hpp"
#include "../Core/Globals.hpp"

namespace negrumdaddy
{
	namespace insta_lock_agents
	{
		inline uobject* astra = nullptr;
		inline uobject* breach = nullptr;
		inline uobject* brimstone = nullptr;
		inline uobject* chamber = nullptr;
		inline uobject* cypher = nullptr;
		inline uobject* fade = nullptr;
		inline uobject* jett = nullptr;
		inline uobject* kayo = nullptr;
		inline uobject* killjoy = nullptr;
		inline uobject* neon = nullptr;
		inline uobject* omen = nullptr;
		inline uobject* phoenix = nullptr;
		inline uobject* raze = nullptr;
		inline uobject* reyna = nullptr;
		inline uobject* sage = nullptr;
		inline uobject* skye = nullptr;
		inline uobject* sova = nullptr;
		inline uobject* viper = nullptr;
		inline uobject* yoru = nullptr;
		inline uobject* gekko = nullptr;

		inline void init_agents()
		{
			if (!astra) astra = uobject::find_object<uobject*>(crypt(L"Default__Rift_PrimaryAsset_C").decrypt(), reinterpret_cast<uobject*>(-1));
			if (!breach) breach = uobject::find_object<uobject*>(crypt(L"Default__Breach_PrimaryAsset_C").decrypt(), reinterpret_cast<uobject*>(-1));
			if (!brimstone) brimstone = uobject::find_object<uobject*>(crypt(L"Default__Sarge_PrimaryAsset_C").decrypt(), reinterpret_cast<uobject*>(-1));
			if (!chamber) chamber = uobject::find_object<uobject*>(crypt(L"Default__Deadeye_PrimaryAsset_C").decrypt(), reinterpret_cast<uobject*>(-1));
			if (!cypher) cypher = uobject::find_object<uobject*>(crypt(L"Default__Gumshoe_PrimaryAsset_C").decrypt(), reinterpret_cast<uobject*>(-1));
			if (!fade) fade = uobject::find_object<uobject*>(crypt(L"Default__BountyHunter_PrimaryAsset_C").decrypt(), reinterpret_cast<uobject*>(-1));
			if (!jett) jett = uobject::find_object<uobject*>(crypt(L"Default__Wushu_PrimaryAsset_C").decrypt(), reinterpret_cast<uobject*>(-1));
			if (!kayo) kayo = uobject::find_object<uobject*>(crypt(L"Default__Grenadier_PrimaryAsset_C").decrypt(), reinterpret_cast<uobject*>(-1));
			if (!killjoy) killjoy = uobject::find_object<uobject*>(crypt(L"Default__Killjoy_PrimaryAsset_C").decrypt(), reinterpret_cast<uobject*>(-1));
			if (!neon) neon = uobject::find_object<uobject*>(crypt(L"Default__Sprinter_PrimaryAsset_C").decrypt(), reinterpret_cast<uobject*>(-1));
			if (!omen) omen = uobject::find_object<uobject*>(crypt(L"Default__Wraith_PrimaryAsset_C").decrypt(), reinterpret_cast<uobject*>(-1));
			if (!phoenix) phoenix = uobject::find_object<uobject*>(crypt(L"Default__Phoenix_PrimaryAsset_C").decrypt(), reinterpret_cast<uobject*>(-1));
			if (!raze) raze = uobject::find_object<uobject*>(crypt(L"Default__Clay_PrimaryAsset_C").decrypt(), reinterpret_cast<uobject*>(-1));
			if (!reyna) reyna = uobject::find_object<uobject*>(crypt(L"Default__Vampire_PrimaryAsset_C").decrypt(), reinterpret_cast<uobject*>(-1));
			if (!sage) sage = uobject::find_object<uobject*>(crypt(L"Default__Thorne_PrimaryAsset_C").decrypt(), reinterpret_cast<uobject*>(-1));
			if (!skye) skye = uobject::find_object<uobject*>(crypt(L"Default__guide_PrimaryAsset_C").decrypt(), reinterpret_cast<uobject*>(-1));
			if (!sova) sova = uobject::find_object<uobject*>(crypt(L"Default__Hunter_PrimaryAsset_C").decrypt(), reinterpret_cast<uobject*>(-1));
			if (!viper) viper = uobject::find_object<uobject*>(crypt(L"Default__Pandemic_PrimaryAsset_C").decrypt(), reinterpret_cast<uobject*>(-1));
			if (!yoru) yoru = uobject::find_object<uobject*>(crypt(L"Default__Stealth_PrimaryAsset_C").decrypt(), reinterpret_cast<uobject*>(-1));
			if (!gekko) gekko = uobject::find_object<uobject*>(crypt(L"Default__AggroBot_PrimaryAsset_C").decrypt(), reinterpret_cast<uobject*>(-1));
		}

		inline uobject* get_agent_object(int id)
		{
			if (id == 0) return astra;
			else if (id == 1) return breach;
			else if (id == 2) return brimstone;
			else if (id == 3) return chamber;
			else if (id == 4) return cypher;
			else if (id == 5) return fade;
			else if (id == 6) return jett;
			else if (id == 7) return kayo;
			else if (id == 8) return killjoy;
			else if (id == 9) return neon;
			else if (id == 10) return omen;
			else if (id == 11) return phoenix;
			else if (id == 12) return raze;
			else if (id == 13) return reyna;
			else if (id == 14) return sage;
			else if (id == 15) return skye;
			else if (id == 16) return sova;
			else if (id == 17) return viper;
			else if (id == 18) return yoru;
			else if (id == 19) return gekko;
			else return nullptr;
		}
	}

	namespace insta_lock
	{
		inline ULONGLONG old_time = 0;
		inline int agent_counter = 0;

		inline void agent_roulette(aplayercontroller* controller) {
			uclass* _class = uobject::find_object<uclass*>(crypt(L"ShooterGame.PregamePlayerController").decrypt());
			if (controller && _class && controller->is_child_of(_class))
			{
				if (pregame_view_controller* pregame_view_controller_ptr = controller->get_pregame_view_controller())
				{
					if (pregame_view_model* pregame_view_model_ptr = pregame_view_controller_ptr->get_pregame_view_model())
					{
						if (!pregame_view_model_ptr->is_local_player_locked_in())
						{
							ULONGLONG current_time = GetTickCount64();
							if (old_time == 0) old_time = current_time;

							if (current_time > old_time)
							{
								old_time = current_time + 50;
								
								uobject* agent_obj = insta_lock_agents::get_agent_object(agent_counter);
								if (agent_obj)
								{
									pregame_view_controller_ptr->select_character(agent_obj);
								}

								agent_counter++;
								if (agent_counter >= 20)
									agent_counter = 0;
							}
						}
						else if (globals::misc::agent_roulette)
						{
							globals::misc::agent_roulette = false;
						}
					}
				}
			}
		}

		inline void run(aplayercontroller* controller)
		{
			insta_lock_agents::init_agents();

			if (globals::misc::agent_roulette)
				agent_roulette(controller);

			if (globals::misc::insta_lock) {
				static uclass* _class = nullptr;
				if (!_class) _class = uobject::find_object<uclass*>(crypt(L"ShooterGame.PregamePlayerController").decrypt());
				if (!controller || !_class) return;

				uclass* ctrl_class = memory::read<uclass*>((uintptr_t)controller + offsets::object_class);
				bool is_pregame = false;
				uclass* check = ctrl_class;
				for (int depth = 0; check && depth < 20; depth++) {
					if (check == _class) { is_pregame = true; break; }
					check = check->super_class();
				}

				if (is_pregame) {
					if (pregame_view_controller* pvc = controller->get_pregame_view_controller())
					{
						if (pregame_view_model* pvm = pvc->get_pregame_view_model())
						{
							if (!pvm->is_local_player_locked_in())
							{
								uobject* agent_object = insta_lock_agents::get_agent_object(globals::misc::agent_to_instalock);
								if (agent_object)
								{
									pvc->select_character(agent_object);
									pvc->lock_character(agent_object);
									globals::misc::insta_lock = false;
								}
							}
						}
					}
				}
			}
		}
	}
}
