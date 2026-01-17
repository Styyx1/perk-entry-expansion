#pragma once



namespace PEE
{
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


	enum struct OperEnum
	{
		Jump = 1,
		Other = 0,
		Call = -1,
	};

	inline static OperEnum IsCallOrJump(uintptr_t addr)
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
				return OperEnum::Call;

			case 0x25:
			case 0xE9:
				return OperEnum::Jump;

			}
		}

		return OperEnum::Other;
	}

	//Need to move to a general use library
	struct Allocator
	{
		static size_t GetAlloc()
		{
			return allocCount;
		}

		inline static size_t allocCount = 0;


		Allocator(size_t count = 14)
		{
			allocCount += count;
		}

	};

#define DECLARE_ALLOC(...) inline static Allocator _alloc{ __VA_ARGS__ };



	struct ProloguePatch : Xbyak::CodeGenerator
	{
		explicit ProloguePatch(uintptr_t address, uintptr_t length)
		{
			// Hook returns here. Execute the restored bytes and jump back to the original function.
			for (size_t i = 0; i < length; i++)
				db(*reinterpret_cast<uint8_t*>(address + i));

			jmp(ptr[rip]);
			dq(address + length);
		}

		static uint8_t* GetInstructions(uintptr_t address, uintptr_t length)
		{
			ProloguePatch it{ address, length };
			it.ready();
			return it.GetInstructions();
		}

		uint8_t* GetInstructions()
		{
			auto size = getSize();
			uint8_t* result{ new uint8_t[size] };
			const uint8_t* from = getCode();
			uint8_t* in = result;
			while (size--) {
				*(in++) = *(from++);
			}

			//std::memcpy(result.get(), getCode(), size);
			return result;
		}
	};
	template <typename T>
	void InstallHook()
	{
		logger::info("Installing {}. . .", typeid(T).name());
		T::Install();
		//If there's a cleaner way to get the name of this
		logger::info("Successfully installed {}", typeid(T).name());
	}


	struct Profiler
	{
		Profiler() : start{ std::chrono::high_resolution_clock::now() }
		{

		}


		double time()
		{
			auto end = std::chrono::high_resolution_clock::now();
			std::chrono::duration<double, std::milli> duration = end - start;
			return duration.count();
		}

		std::chrono::steady_clock::time_point start;




	};

	struct AutoProfiler : public Profiler
	{
		struct Pause
		{
			bool old;

			Pause()
			{
				old = dontProfile;
				dontProfile = true;
			}

			~Pause()
			{
				dontProfile = old;
			}
		};

		inline static thread_local bool dontProfile = false;

		std::string_view name;
		std::source_location location;
		bool profiling{};

		AutoProfiler(const char* n, std::source_location loc) : name{ n }, location{ loc }, profiling{ !dontProfile }
		{
		}

		~AutoProfiler()
		{
			if (profiling) {
				auto t = time();

				std::string_view file = location.file_name();

				std::cout << "============" << name << " at " << file.substr(file.find_last_of("/\\") + 1) << "(" << location.line() << ") " << t << "ms============" << std::endl;
			}
		}


	};

	template <typename R>
	R ProfileCall(std::function<R()> func, std::source_location loc = std::source_location::current())
	{

		AutoProfiler profiler{ "Call", loc };

		return func();
	}

#ifdef NDEBUG

#define PROFILE(mc_expr) mc_expr
#define PROFILE_STATEMENT(mc_stat) mc_stat

#else

#define PROFILE(mc_expr) ProfileCall(std::function{[&]() -> auto { return mc_expr; }})
#define PROFILE_STATEMENT(mc_stat) ProfileCall(std::function{[&]() -> auto { mc_stat; }})

#endif
}