#pragma once
#include "SpeechCheck/SpeechCheckHandler.h"
namespace PEE::SPCK
{

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


			auto call_or_jmp = IsCallOrJump(hook_addr);

			auto place_query = trampoline.write_branch<5>(hook_addr, (uintptr_t)thunk);

			if (call_or_jmp != OperEnum::Jump)
				func = (uintptr_t)code.getCode();
			else
				func = place_query;

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

}