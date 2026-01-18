#pragma once

namespace PEE
{

	namespace SPCK
	{
		static inline constexpr auto perkCategory = "SpeechCheck";
		static inline constexpr RE::PerkEntryPoint perkEntry = RE::PerkEntryPoint::kActivate;//Temporary I need to figure it out.
	}

	namespace WARES
	{
		static inline constexpr auto perkCategory = "MerchantWares";
		static inline constexpr RE::PerkEntryPoint perkEntry[2]{};//Temporary I need to figure it out.
	}

	namespace MACS
	{
		static inline constexpr auto perkCategory = "MagicApplySpell";
		static inline constexpr RE::PerkEntryPoint perkEntry = RE::PerkEntryPoint::kApplyCombatHitSpell;
	}

	namespace PACS 
	{
		static inline constexpr auto perkCategory = "PotionApplySpell";
		static inline constexpr RE::PerkEntryPoint perkEntry = RE::PerkEntryPoint::kApplyCombatHitSpell;
	}

}