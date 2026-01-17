#pragma once

#include "RE/StringSetting.h"
#include "Utility.h"

namespace PEE::SPCK
{
	//Move this shit

	//I have a macro for this but fuck it. 
	namespace detail
	{
		struct Speech__Type
		{
			enum Type
			{
				VeryEasy,
				Easy,
				Average,
				Hard,
				VeryHard,
				Total,
			};
		};
	}
	using Speech = detail::Speech__Type::Type;

	namespace detail
	{
		struct CheckType__Type
		{
			enum Type
			{
				None,	//Isn't a check
				Speech,	//Is a speech check
				Fail,	//Is a condition to tell if one has succeeded
				Pass	//Is an articulation check
			};
		};
	}
	using CheckType = detail::CheckType__Type::Type;


	struct SpeechCheckHandler
	{
	private:
		

		static int _CheckDifficulty(RE::TESGlobal* global)
		{
			if (!global)
				return false;

			for (int i = 0; i < g_difficultyList.size(); i++)
			{
				if (global == g_difficultyList[i])
					return i + 1;
			}

			//auto end = std::end(difficultyList);

			//auto result = std::find(std::begin(difficultyList), end, global);

			std::string id = global->GetFormEditorID();

			if (id.size() < k_difficultyID.size())
				return 0;

			auto index = id.find(k_difficultyID);

			if (index == std::string::npos)
				return 0;

			return -1;
		}

		using ItemData = RE::CONDITION_ITEM_DATA;


		static CheckType _IsSpeechCheck(RE::CONDITION_ITEM_DATA& data, RE::TESGlobal*& diff)
		{
			auto& func_data = data.functionData;

			VoidCaster<RE::ActorValue> actor_value = func_data.params[0];

			if (actor_value != RE::ActorValue::kSpeech)
				return CheckType::None;

			//The below handles this now to make sure that the under check is valid as well
			//if (auto op = data.flags.opCode; op != ItemData::OpCode::kGreaterThanOrEqualTo && op != ItemData::OpCode::kLessThan)
			//	return CheckType::None;


			CheckType result = CheckType::None;

			switch (data.flags.opCode)
			{
			case ItemData::OpCode::kGreaterThanOrEqualTo:
				result = CheckType::Speech;
				break;

				//I will re enable this when it actually shows itself to cause a problem
			//case ItemData::OpCode::kLessThan:
			//	result = CheckType::Fail;
			//	break;

			default:
				return CheckType::None;
			}
				

			//For now no OR distinction

			//Is not allowed to swap target, nor is it allowed to not be a global
			if (data.flags.swapTarget || !data.flags.global)
				return CheckType::None;

			RE::TESGlobal* global = data.comparisonValue.g;

			//Choosen global must be one of the difficulty modifiers in vanilla, or one designated by users.
			if (_CheckDifficulty(global) == 0)
				return CheckType::None;

			diff = global;

			return result;
		}

		static CheckType _IsPassCheck(RE::CONDITION_ITEM_DATA& data)
		{
			auto& func_data = data.functionData;

			RE::TESForm* comp_form = VoidCaster<RE::TESForm*>{ func_data.params[0] };

			if (!comp_form || comp_form != g_passFormList)
				return CheckType::None;


			if (data.flags.opCode != ItemData::OpCode::kEqualTo)
				return CheckType::None;


			//cant swap, mustn't be global
			if (data.flags.swapTarget || data.flags.global)
				return CheckType::None;

			if (data.comparisonValue.f != 1.f)
				return CheckType::None;

			//No OR check. Unsure about using that for now.

			return CheckType::Pass;
		}


		//Give this an out for global, just for ease of use
		static CheckType _IsPersuasionCheck(RE::TESConditionItem* item, RE::TESGlobal*& diff)
		{
			using GlobalOrFloat = RE::CONDITION_ITEM_DATA::GlobalOrFloat;

			CheckType type = CheckType::None;

			auto& data = item->data;

			RE::CONDITIONITEMOBJECT target_type = *data.object;

			bool player_specific = false;

			switch (target_type)
			{
			//If used on the player ref specifically
			case RE::CONDITIONITEMOBJECT::kRef:
				if (auto ref = data.runOnRef.get(); !ref || ref->IsPlayerRef() == false)
					return CheckType::None;
				player_specific = true;


				[[fallthrough]];
			//If used on target
			case RE::CONDITIONITEMOBJECT::kTarget:
				break;
			
			default:
				return CheckType::None;
			}


			switch (*data.functionData.function)
			{
			case RE::FUNCTION_DATA::FunctionID::kGetActorValue:
				return _IsSpeechCheck(data, diff);
			
			case RE::FUNCTION_DATA::FunctionID::kGetEquipped:
				if (player_specific)
					return _IsPassCheck(data);

			default:
				return CheckType::None;
			}
		}

		
		//Boolean is so I can just use return + this
		static void _ResetGlobals()
		{
			if (g_difficultyLevel)
				g_difficultyLevel->value = 0;

			if (g_difficultyRank)
				g_difficultyRank->value = 0;

			g_processingThreadHash = 0;
		}

		static void _SetGlobals(RE::TESGlobal* global)
		{
			if (g_difficultyRank)
			{
				g_difficultyRank->value = 0;

				for (int i = 0; i < g_difficultyList.size(); i++)
				{
					if (global == g_difficultyList[i]) {
						g_difficultyRank->value = i + 1;
					}
				}

				if (g_difficultyRank->value == 0)
					g_difficultyRank->value = -1;
			}

			if (g_difficultyLevel)
				g_difficultyLevel->value = global->value;

		}

		static std::optional<bool> _HandleForcedResult(RE::Actor* target, RE::Actor* subject, RE::TESForm* third_arg)
		{
			std::optional<bool> result = std::nullopt;

			float value = 0;

			//values below 0 count as zero.

			//For these 2, I may want to do them both, so I can send a notification.

			//The force fail entry points. Higher priority than force success.
			RE::HandleEntryPoint(RE::PerkEntryPoint::kShouldApplyPlacedItem, target, value, k_categoryName, third_arg, subject);//2 + out
			RE::HandleEntryPoint(RE::PerkEntryPoint::kGetShouldAttack, subject, value, k_categoryName, third_arg);//1 + out

			logger::debug("Forced failure: {}", value > 0);

			if (value > 0) {
				//result = false;
				return false;
			}
			value = 0;


			//The force success entry points
			RE::HandleEntryPoint(RE::PerkEntryPoint::kCalculateMyCriticalHitChance, target, value, k_categoryName, third_arg, subject);//2 + out
			RE::HandleEntryPoint(RE::PerkEntryPoint::kModEnemyCriticalHitChance, subject, value, k_categoryName, third_arg, target);//2 + out

			logger::debug("Forced success: {}", value > 0);


			if (value > 0) {
				return true;
				//if (result == std::nullopt)
				//	result = true;
				//else{
				//	RE::DebugNotification(g_forceFailString.GetCString());
				//}
			}

			return result;
		}


		static float _HandleSkillValue(RE::Actor* target, RE::Actor* subject, RE::TESForm* third_arg, RE::TESGlobal* global)
		{
			float value = 0;

			float speech_skill = target->AsActorValueOwner()->GetActorValue(RE::ActorValue::kSpeech);
			

			//max base skill check(player focus). Basically caps a persons base speech before the persuasion check. can be useful
			// if one wishing to prevent say, 100 speech from winning all speech checks.
			RE::HandleEntryPoint(RE::PerkEntryPoint::kModPickpocketChance, target, value, k_categoryName, third_arg, subject);//2 + out

			logger::debug("init skill: {} ({})", speech_skill, value);

			if (value > 0)
				speech_skill = fmin(speech_skill, value);



			//Persuasion, player check, enemy check, 
			RE::HandleEntryPoint(RE::PerkEntryPoint::kModAttackDamage, target, speech_skill, k_categoryName, third_arg, subject);//2 + out
			RE::HandleEntryPoint(RE::PerkEntryPoint::kModIncomingDamage, subject, speech_skill, k_categoryName, third_arg, target);//2 + out

			logger::debug("speech skill: {}", speech_skill);

			return speech_skill;
		}

		static float _HandleCheckValue(RE::Actor* target, RE::Actor* subject, RE::TESForm* third_arg, RE::TESGlobal* global)
		{
			float speech_check = global->value;

			//Speech check eps. First for player, second for subject.
			RE::HandleEntryPoint(RE::PerkEntryPoint::kModTargetDamageResistance, target, speech_check, k_categoryName, third_arg, subject);//2 + out
			RE::HandleEntryPoint(RE::PerkEntryPoint::kModArmorRating, subject, speech_check, k_categoryName, third_arg);//1 + out

			logger::debug("speech check: {}", speech_check);

			return speech_check;
		}


	public:
		static std::optional<bool> Handle(RE::TESConditionItem* a_this, RE::ConditionCheckParams& a2)
		{
			RE::TESGlobal* dif_global = nullptr;

			auto check_result = _IsPersuasionCheck(a_this, dif_global);

			
			bool reverse_result = false;

			switch (check_result)
			{
			case CheckType::None:
				return std::nullopt;

			case CheckType::Pass://We ignore amu. of articulation checks
				return false;

			case CheckType::Fail://and reverse process checks that ask for less than with 
				reverse_result = true;
				break;

				//Speech just breaks
			}

			//switch instead
			//if (check_result != CheckType::Speech)
			//	return  check_result == CheckType::None ? std::nullopt : std::optional{ false };




			//Should be player. Even if it's running on player, the target should be the player, as that's what dialogue menus do. Will change if I see a reason to.
			RE::Actor* target = a2.targetRef->As<RE::Actor>();

			//Should be the guy we're talking to. Perhaps it's worth double checking via dialogue topic ref?
			RE::Actor* subject = a2.actionRef->As<RE::Actor>();

			if (!target || !target->IsPlayerRef() || !subject || !dif_global)
				return std::nullopt;
		

			//Should it currently be processing one of these, it will wait if its the same thread however, 
			// it will not go through this and instead perform the check as normal

			std::hash<std::thread::id> hasher;

			size_t thread_hash = hasher(std::this_thread::get_id());

			if (g_processingThreadHash == thread_hash)
				return std::nullopt;
			
			std::lock_guard<std::mutex> guard{ g_entryPointLock };

			g_processingThreadHash = thread_hash;

			

			_SetGlobals(dif_global);

			//The third arg isn't used yet, want to make an papyrus API for that. For now? Nothin.
			// To be safe though, third arg is always the subject if null. Or maybe the player? Easier to nullify.
			RE::TESForm* third_arg = nullptr;

			if (!third_arg)
				third_arg = subject;

			

			if (auto opt = _HandleForcedResult(target, subject, third_arg); opt != std::nullopt) {
				//I don't know if the reference return fucks with the result
				bool result = opt.value();
				_ResetGlobals();
				return result != reverse_result;
			}



			float speech_skill = _HandleSkillValue(target, subject, third_arg, dif_global);
			float speech_check = _HandleCheckValue(target, subject, third_arg, dif_global);

			_ResetGlobals();

			bool result = speech_skill >= speech_check;

			return result != reverse_result;
		}


		static void Init()
		{
			auto col = RE::GameSettingCollection::GetSingleton();

			col->InsertSetting(g_forceFailString);
		}
	

		static void Install()
		{
			g_difficultyList[Speech::VeryEasy] = RE::TESForm::LookupByID<RE::TESGlobal>(0xD16A3);
			g_difficultyList[Speech::Easy] = RE::TESForm::LookupByID<RE::TESGlobal>(0xD16A4);
			g_difficultyList[Speech::Average] = RE::TESForm::LookupByID<RE::TESGlobal>(0xD16A5);
			g_difficultyList[Speech::Hard] = RE::TESForm::LookupByID<RE::TESGlobal>(0xD1953);
			g_difficultyList[Speech::VeryHard] = RE::TESForm::LookupByID<RE::TESGlobal>(0xD1954);
			//Will need the data manager
			g_difficultyRank = RE::TESForm::LookupByEditorID<RE::TESGlobal>("_SpeechCheck_DifficultyRank");
			g_difficultyLevel = RE::TESForm::LookupByEditorID<RE::TESGlobal>("_SpeechCheck_DifficultyLevel");

			//Checks for articulation
			g_passFormList = RE::TESForm::LookupByID<RE::BGSListForm>(0xF7593);
		}


	private:


		inline static std::mutex g_entryPointLock;

		inline static size_t g_processingThreadHash = 0;

		inline static RE::BGSListForm* g_passFormList = nullptr;

		inline static RE::TESGlobal* g_difficultyRank = nullptr;
		inline static RE::TESGlobal* g_difficultyLevel = nullptr;

		
		inline static RE::StringSetting g_forceFailString {"sSpeechCheck_ForceFail", "Convincing them might not be possible right now."};

		inline static std::vector<RE::TESGlobal*> g_difficultyList{ Speech::Total };


		//Please fix the fucking function so it accounts for constexpr shit please, the problem is the use of string_views
		inline static std::string_view k_difficultyID = "SPEECH_DIFFICULTY";
		inline static std::string_view k_categoryName = "SPEECH_CHECK";
		inline static uint8_t k_categoryChannel = 1;
	};
}