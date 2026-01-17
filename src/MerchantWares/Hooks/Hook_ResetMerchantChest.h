#pragma once

namespace PEE::WARES
{

	struct TESFaction__ResetMerchantChest
	{
		static void Patch()
		{
			//SE: 3551C0+5C AE: 36DB60+5C

			REL::RelocationID reset_call{ 23998, 24495 };

			auto& trampoline = SKSE::GetTrampoline();

			//No func because it's a vtable call. If someone else overrides the same place, I can try to detect if what was there was a call or not
			// and if so place into func. But, not needed for not.
			trampoline.write_call<6>(reset_call.address() + 0x5C, thunk);


			logger::info("ResetMerchantChest hook complete...");
		}


		static void thunk(RE::TESObjectREFR* a_this, bool a2)
		{
			a_this->ResetInventory(a2);

			//RE::GPtr<RE::BarterMenu> menu = RE::UI::GetSingleton()->GetMenu<RE::BarterMenu>();

			//if (!menu)
			//	return;


			//RE::RefHandle merc_handle = menu->GetTargetRefHandle();
			RE::RefHandle merc_handle = RE::BarterMenu::GetTargetRefHandle();

			RE::ActorPtr merc = RE::Actor::LookupByHandle(merc_handle);

			if (!merc)
				return;

			logger::info("C");

			RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
			RE::Actor* merchant = merc.get();

			//For these it needs the form* as the out. not the form**. So deref it.

			//The version that works regardless
			std::string reward = "Reward";

			RE::HandleEntryPoint(RE::PerkEntryPoint::kAddLeveledListOnDeath, player, a_this, reward,  merchant);
			//The second runs on the npc, targets their chest, and items go to the merchant chest.
			RE::HandleEntryPoint(RE::PerkEntryPoint::kAddLeveledListOnDeath, merchant, a_this, reward, a_this);

			//But this should work too, but currently doesn't because I'm a poopy head programmer. :(
			//RE::HandleEntryPoint(RE::PerkEntryPoint::kAddLeveledListOnDeath, player, a_this, "Reward", 1, merchant);
			//RE::HandleEntryPoint(RE::PerkEntryPoint::kAddLeveledListOnDeath, merchant, a_this, "Reward", 1, a_this);
		}




		//static inline REL::Relocation<decltype(thunk)> func;
	};

}