#pragma once

#include "xbyak/xbyak.h"
#include"SpeechCheckHandler.h"

namespace SPCR
{
	struct Hooks
	{
	private:
		static int IsCallOrJump(uintptr_t addr)
		{
			//0x15 0xE8//These are calls, represented by negative numbers
			//0x25 0xE9//These are jumps, represented by positive numbers.
			//And zero represent it being neither.

			if (addr)
			{
				auto first_byte = reinterpret_cast<uint8_t*>(addr);

				switch (*first_byte)
				{
				case 0x15:
				case 0xE8:
					return -1;

				case 0x25:
				case 0xE9:
					return 1;

				}
			}

			return 0;
		}


		struct TESConditionItem__IsTrue
		{
			//
			//Note, hook is actually for SetBaseActorValue

			static void Patch()
			{
				//SE: 0x4454C0, AE: 0x460B30, VR: ???
				auto hook_addr = REL::RelocationID(29090, 29924).address();

				auto return_addr = hook_addr + 0x5;
				//*
				struct Code : Xbyak::CodeGenerator
				{
					Code(uintptr_t ret_addr)
					{
						//uintptr_t arg_0 = qword ptr 10h
						//mov     [rsp-8+arg_0], rcx ???
						mov(ptr[rsp - 8 + 0x10], rcx);

						mov(rax, ret_addr);
						jmp(rax);
					}
				} static code{ return_addr };

				auto& trampoline = SKSE::GetTrampoline();


				auto placed_call = IsCallOrJump(hook_addr) > 0;

				auto place_query = trampoline.write_branch<5>(hook_addr, (uintptr_t)thunk);

				if (!placed_call)
					func = (uintptr_t)code.getCode();
				else
					func = place_query;


				logger::info("TESConditionItem__IsTrue complete...");
				//*/
			}


			static bool thunk(RE::TESConditionItem* a_this, RE::ConditionCheckParams& a2)
			{
				auto opt = SpeechCheckHandler::Handle(a_this, a2);

				if (opt == std::nullopt)
					return func(a_this, a2);
				
				bool result = opt.value();

				return result;
			}

			static inline REL::Relocation<decltype(thunk)> func;
		};

		struct TESFaction__ResetMerchantChest
		{
			static void Patch()
			{
				//SE: 3551C0+5C AE: 36DB60+5C

				REL::RelocationID reset_call{ 23998 ,24495 };

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
				//For the first, it runs on the player, targets the npc, and has the items go to the merchant chest.
				RE::HandleEntryPoint(RE::PerkEntryPoint::kAddLeveledListOnDeath, player, a_this, reward, 1, { merchant });
				//The second runs on the npc, targets their chest, and items go to the merchant chest.
				RE::HandleEntryPoint(RE::PerkEntryPoint::kAddLeveledListOnDeath, merchant, a_this, reward, 1, {a_this});

				//But this should work too, but currently doesn't because I'm a poopy head programmer. :(
				//RE::HandleEntryPoint(RE::PerkEntryPoint::kAddLeveledListOnDeath, player, a_this, "Reward", 1, merchant);
				//RE::HandleEntryPoint(RE::PerkEntryPoint::kAddLeveledListOnDeath, merchant, a_this, "Reward", 1, a_this);
			}




			//static inline REL::Relocation<decltype(thunk)> func;
		};


		struct TestPatch
		{
			static void Patch()
			{
				func = REL::Relocation<uintptr_t>{ RE::VTABLE_InventoryMenu[0] }.write_vfunc(0x1, thunk);

				logger::info("test hook complete...");
			}


			static int32_t thunk(RE::InventoryMenu* a_this, RE::FxDelegateHandler::CallbackProcessor* a2)
			{
				size_t vtable = *reinterpret_cast<size_t*>(a2);

				logger::info("Vtable {:X}, raw {:X}", vtable - REL::Module::get().base(), vtable);
				return func(a_this, a2);
			}




			static inline REL::Relocation<decltype(thunk)> func;
		};
	public:
		static void Install()
		{
			SKSE::AllocTrampoline(28);
			//TestPatch::Patch();
			TESFaction__ResetMerchantChest::Patch();
			TESConditionItem__IsTrue::Patch();
		}
	};
}


//Dead zone

/*


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

//This formlist enables the speech check to pass automatically.
RE::BGSListForm* passFormList = nullptr;

RE::TESGlobal* difficultyRank = nullptr;
RE::TESGlobal* difficultyLevel = nullptr;

std::vector<RE::TESGlobal*> difficultyList{ Speech::Total };

constexpr std::string_view k_difficultyID = "SPEECH_DIFFICULTY";

int CheckDifficulty(RE::TESGlobal* global)
{
	if (!global)
		return false;

	for (int i = 0; i < difficultyList.size(); i++)
	{
		if (global == difficultyList[i])
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



//On second thought, not required.
class FormlistVisitor : public RE::InventoryChanges::IItemChangeVisitor
{
	using ReturnType = bool;
	using SearchResult = RE::BSContainer::ForEachResult;
public:
	virtual ~FormlistVisitor() = default;  // 00

	virtual bool Visit(RE::InventoryEntryData* a_entryData) override
	{
		if (!passFormList || !a_entryData || !a_entryData->object)
			return ReturnType(SearchResult::kContinue);

		wornFound = passFormList->HasForm(a_entryData->object);

		return wornFound ? ReturnType(SearchResult::kStop) : ReturnType(SearchResult::kContinue);
	}


	bool wornFound = false;
};


namespace detail
{
	struct CheckType__Type
	{
		enum Type
		{
			None,	//Isn't a check
			Speech,	//Is a speech check
			Pass	//Is an articulation check
		};
	};
}
using CheckType = detail::CheckType__Type::Type;


template<class A, class B>
union ConversionCaster
{
	A _a;
	B _b;

	ConversionCaster() = default;
	ConversionCaster(A a) : _a(a) {}
	ConversionCaster(B b) : _b(b) {}

	ConversionCaster& operator=(A a) { _a = a; return *this; }
	ConversionCaster& operator=(B b) { _b = b; return *this; }

	operator A() { return _a; }
	operator B() { return _b; }
};
template <class B>
using VoidCaster = ConversionCaster<void*, B>;

//Give this an out for global, just for ease of use
CheckType temp_SpeechCheck(RE::TESConditionItem* item, RE::TESGlobal*& diff)
{
	using ItemData = RE::CONDITION_ITEM_DATA;

	using GlobalOrFloat = RE::CONDITION_ITEM_DATA::GlobalOrFloat;

	CheckType type = CheckType::None;

	auto& data = item->data;

	RE::CONDITIONITEMOBJECT target_type = *data.object;

	switch (target_type)
	{
		//If used on target
	case RE::CONDITIONITEMOBJECT::kRef:
		//This will test if this is really player.
		if (data.runOnRef.get()->IsPlayerRef() == false)
			return type;

	case RE::CONDITIONITEMOBJECT::kTarget:
		//I may want to ignore this if the targets are switched and such

		break;
	default:
		return type;
	}

	auto& func_data = data.functionData;


	CheckType result = CheckType::None;

	//Not using right now, just building.
	switch (*func_data.function)
	{
	case RE::FUNCTION_DATA::FunctionID::kGetActorValue:
		//VoidCaster<RE::ActorValue> actor_value = func_data.params[0];
		result = CheckType::Speech;
		break;
	case RE::FUNCTION_DATA::FunctionID::kGetEquipped:
		//VoidCaster<RE::TESForm*> actor_value = func_data.params[0];
		result = CheckType::Pass;
		break;

	default:
		return type;
	}

	if (func_data.function != RE::FUNCTION_DATA::FunctionID::kGetActorValue)
		return type;

	//Move into the switch sometime.
	VoidCaster<RE::ActorValue> actor_value = func_data.params[0];

	if (actor_value != RE::ActorValue::kSpeech)
		return type;


	if (data.flags.opCode != ItemData::OpCode::kGreaterThanOrEqualTo)
		return type;

	//For now no OR distinction

	//Is not allowed to swap target, nor is it allowed to not be a global
	if (data.flags.swapTarget || !data.flags.global)
		return type;

	RE::TESGlobal* global = data.comparisonValue.g;

	//Choosen global must be one of the difficulty modifiers in vanilla, or one designated by users.
	if (CheckDifficulty(global) == false)
		return type;

	diff = global;

	return CheckType::Speech;
}


inline static std::mutex entryPointLock;

inline static bool perkPointProcessing = false;

//Boolean is so I can just use return + this
static bool temp_ResetGlobals(bool value = true)
{
	if (difficultyLevel)
		difficultyLevel->value = 0;

	if (difficultyRank)
		difficultyRank->value = 0;

	perkPointProcessing = false;

	return value;
}

static bool thunk(RE::TESConditionItem* a_this, RE::ConditionCheckParams& a2)
{
	RE::TESGlobal* dif_global = nullptr;

	auto check_result = temp_SpeechCheck(a_this, dif_global);

	if (check_result == CheckType::None)
		return func(a_this, a2);
	else if (check_result == CheckType::Pass)//Not used right now, but accounts for the articulation check.
		return false;


	if (a_this)
	{
		std::lock_guard<std::mutex> guard{ entryPointLock };


		//Should be player
		RE::Actor* target = a2.targetRef->As<RE::Actor>();

		//Should be the guy we're talking to
		RE::Actor* subject = a2.actionRef->As<RE::Actor>();

		if (!target || !subject || !dif_global)
			return func(a_this, a2);


		if (perkPointProcessing)
			return func(a_this, a2);

		perkPointProcessing = true;

		//Make it's own function
		if (difficultyRank)
		{
			difficultyRank->value = 0;

			for (int i = 0; i < difficultyList.size(); i++)
			{
				if (dif_global == difficultyList[i]) {
					difficultyRank->value = i + 1;
				}
			}

			if (difficultyRank->value == 0)
				difficultyRank->value = -1;
		}

		if (difficultyLevel)
			difficultyLevel->value = dif_global->value;


		//End of function

		//The third arg isn't used yet, want to make an papyrus API for that. For now? Nothin.
		//To be safe though, third arg is always the subject. if null.
		RE::TESForm* third_arg = nullptr;

		if (!third_arg)
			third_arg = subject;

		//I'm making it 2 (3 in editor) because 2 is likely to be taken space. So honestly it's just in case.
		//constexpr std::string_view k_categoryName = "SPEECH_CHECK";
		std::string categoryName = "SPEECH_CHECK";
		constexpr uint8_t k_entryChannel = 1;


		//Need to reorganize the targets here, mainly because they need to align with what the perk entry implies.

		float value = 0;

		//values below 0 count as zero.

		//For these 2, I may want to do them both, so I can send a notification.

		//The force fail entry points. Higher priority than force success.
		RE::HandleEntryPoint(RE::PerkEntryPoint::kShouldApplyPlacedItem, target, value, categoryName, k_entryChannel, third_arg, subject);//2 + out
		RE::HandleEntryPoint(RE::PerkEntryPoint::kGetShouldAttack, subject, value, categoryName, k_entryChannel, third_arg);//1 + out

		logger::debug("Forced failure: {}", value > 0);

		if (value > 0) {
			return temp_ResetGlobals(false);
		}
		value = 0;


		//The force success entry points
		RE::HandleEntryPoint(RE::PerkEntryPoint::kCalculateMyCriticalHitChance, target, value, categoryName, k_entryChannel, third_arg, subject);//2 + out
		RE::HandleEntryPoint(RE::PerkEntryPoint::kModEnemyCriticalHitChance, subject, value, categoryName, k_entryChannel, third_arg, target);//2 + out

		logger::debug("Forced success: {}", value > 0);


		if (value > 0) {
			return temp_ResetGlobals(true);
		}
		value = 0;

		float speech_skill = target->GetActorValue(RE::ActorValue::kSpeech);
		float speech_check = dif_global->value;



		//max base skill check(player focus). Basically caps a persons base speech before the persuasion check. can be useful
		// if one wishing to prevent say, 100 speech from winning all speech checks.
		RE::HandleEntryPoint(RE::PerkEntryPoint::kModPickpocketChance, target, value, categoryName, k_entryChannel, third_arg, subject);//2 + out

		logger::debug("init skill: {} ({})", speech_skill, value);

		if (value > 0)
			speech_skill = fmin(speech_skill, value);



		//Persuasion, player check, enemy check, 
		RE::HandleEntryPoint(RE::PerkEntryPoint::kModAttackDamage, target, speech_skill, categoryName, k_entryChannel, third_arg, subject);//2 + out
		RE::HandleEntryPoint(RE::PerkEntryPoint::kModIncomingDamage, subject, speech_skill, categoryName, k_entryChannel, third_arg, target);//2 + out

		logger::debug("speech skill: {}", speech_skill);

		//Speech check eps. First for player, second for subject.
		RE::HandleEntryPoint(RE::PerkEntryPoint::kModTargetDamageResistance, target, speech_check, categoryName, k_entryChannel, third_arg, subject);//2 + out
		RE::HandleEntryPoint(RE::PerkEntryPoint::kModArmorRating, subject, speech_check, categoryName, k_entryChannel, third_arg);//1 + out

		logger::debug("speech check: {}", speech_check);


		return temp_ResetGlobals(speech_skill >= speech_check);


	}


	return false;
}

//*/
