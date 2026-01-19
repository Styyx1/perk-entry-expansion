#pragma once
#include "PerkEntries.h"

namespace PEE {
	struct Potion__ApplySpell {
		static inline void Patch() {

			auto& trampoline = SKSE::GetTrampoline();

			REL::Relocation<std::uintptr_t> _playerUsePotion{ REL::RelocationID(39604, 40690, 0x6d7b00), REL::VariantOffset(0x15, 0x15, 0x15) };
			_originalCall = trampoline.write_call<5>(_playerUsePotion.address(), &PlayerUsePotion);
			logger::info("Potion Use Hook complete...");
		}

		static inline void PlayerUsePotion(RE::PlayerCharacter* a_this, RE::AlchemyItem* alch, RE::ExtraDataList* extra_list) 
		{
			_originalCall(a_this, alch, extra_list);

			std::vector<RE::SpellItem*> sp_vec;

			RE::HandleEntryPoint(PACS::perkEntry, a_this, sp_vec, PACS::perkCategory, alch, a_this);

			if (sp_vec.empty())
			{
				logger::debug("Spell Vector for potions is empty");
			}
			else {
				for (auto spell : sp_vec) {
					if (spell) {
						logger::debug("applySpell is: {}", spell->GetName());
						if (spell->IsPermanent()) {
							a_this->AddSpell(spell);
						}
						else {
							a_this->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant)->CastSpellImmediate(spell, false, a_this, 1.0F, false, 0.0F, nullptr);
						}
					}
				}
			}
		};
		static inline REL::Relocation<decltype(&PlayerUsePotion)> _originalCall;
	};
}