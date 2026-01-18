#pragma once 
#include "PerkEntries.h"

namespace PEE {
	struct ApplyHitMagicHitSpells {

		static void Patch()
		{
			auto& trampoline = SKSE::GetTrampoline();

			REL::Relocation<std::uintptr_t> _magicApplyHook{ REL::RelocationID(43015, 44206), REL::VariantOffset(0x216, 0x218, 0x216) };
			_originalCall = trampoline.write_call<5>(_magicApplyHook.address(), &MagicApplyHit);
			logger::info("Magic Apply Hook complete...");
		}

		static void MagicApplyHit(RE::MagicCaster* a_caster, RE::NiPoint3* a_hitPosition, RE::Projectile* a_proj, RE::TESObjectREFR* a_targ, float a_unk1, float a_unk2) {
			_originalCall(a_caster, a_hitPosition, a_proj, a_targ, a_unk1, a_unk2);

			auto caster = a_caster->GetCasterAsActor();
			auto target = a_targ ? a_targ->As<RE::Actor>() : nullptr;
			std::vector<RE::SpellItem*> sp_vec;
			auto spell = a_proj->GetProjectileRuntimeData().spell ? skyrim_cast<RE::SpellItem*>(a_proj->GetProjectileRuntimeData().spell) : nullptr;


			RE::HandleEntryPoint(MACS::perkEntry, caster, sp_vec, MACS::perkCategory, spell, target);

			if (sp_vec.empty())
			{
				logger::warn("Spell Vector is empty");
			}
			else {
				for (auto spell : sp_vec) {
					if (spell) {
						logger::debug("applySpell is: {}", spell->GetName());
						if (spell->IsPermanent()) {
							target->AddSpell(spell);
						}
						else {
							caster->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant)->CastSpellImmediate(spell, false, target, 1.0F, false, 0.0F, nullptr);
						}
					}
				}
			}
		};
		static inline REL::Relocation<decltype(&MagicApplyHit)> _originalCall;
	};
}