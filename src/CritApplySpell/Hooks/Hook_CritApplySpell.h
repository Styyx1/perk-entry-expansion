#pragma once 
#include "PerkEntries.h"

/*
SE ID: 42844 SE Offset: 0x3a3 (Heuristic)
AE ID: 44016 AE Offset: 0x40b
*/

namespace PEE {
	struct Crit__ApplyCombatSpell {
		static void Patch()
		{
			auto& trampoline = SKSE::GetTrampoline();

			REL::Relocation<std::uintptr_t> _magicApplyHook{ REL::RelocationID(42844, 44016), REL::VariantOffset(0x3a3, 0x40b, 0x3a3) }; //VR ID and offset completely unknown
			_originalCall = trampoline.write_call<5>(_magicApplyHook.address(), &ApplyCritDMGEntry);
			logger::info("Critical Hit Apply Hook complete...");
		}

		static void  ApplyCritDMGEntry(RE::BGSPerkEntry::EntryPoint ep, RE::Actor* attacker, RE::TESObjectWEAP* weapon, RE::Actor* target, float& damage) 
		{
			_originalCall(ep, attacker, weapon, target, damage);

			std::vector<RE::SpellItem*> sp_vec;

			RE::HandleEntryPoint(CACS::perkEntry, attacker, sp_vec, CACS::perkCategory, weapon, target);

			if (sp_vec.empty())
			{
				logger::debug("Spell Vector for critical is empty");
			}
			else {
				for (auto spell : sp_vec) {
					if (spell) {
						logger::debug("applySpell is: {}", spell->GetName());
						if (spell->IsPermanent()) {
							target->AddSpell(spell);
						}
						else {
							attacker->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant)->CastSpellImmediate(spell, false, target, 1.0F, false, 0.0F, nullptr);
						}
					}
				}
			}
		};
		static inline REL::Relocation<decltype(&ApplyCritDMGEntry)> _originalCall;
			
		
	};
}