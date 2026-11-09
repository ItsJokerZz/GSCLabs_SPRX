#pragma once

#define JUMP_32SIZE 5
#define JUMP_64SIZE 14
#define CALL_32SIZE JUMP_32SIZE

bool is_relatively_far(const void* start, const void* destination);
void write_jump32(void* address, void* destination);
void write_jump64(void* address, void* destination);
void write_call32(void* address, void* destination);

class trampoline
{
protected:
	std::uint8_t* trampoline_bytes = nullptr;
	std::uint8_t* original_bytes = nullptr;
	void* address = nullptr;
	bool set = false;
public:
	virtual void create(void* address, void* destination, uint64_t base) = 0;
	virtual void restore();

	virtual inline ~trampoline()
	{
		this->restore();
	}
};

class jump : public trampoline
{
public:
	template<class T>
	jump(void* address, T(*function), uint64_t base = 0)
	{
		this->create(address, reinterpret_cast<void*>(function), base);
	}

	template<class T>
	jump(std::uintptr_t address, T(*function)(), uint64_t base)
	{
		this->create(reinterpret_cast<void*>(address), reinterpret_cast<void*>(function), base);
	}

	virtual inline ~jump()
	{
		this->restore();
	}

	virtual void create(void* address, void* destination, uint64_t base) override;
};

class call : public trampoline
{
private:
	void* original_ptr = nullptr;

public:
	template<class T>
	call(void* address, T(*function), uint64_t base)
	{
		this->create(address, reinterpret_cast<void*>(function), base);
	}

	template<class T>
	call(std::uintptr_t address, T(*function), uint64_t base)
	{
		this->create(reinterpret_cast<void*>(address), reinterpret_cast<void*>(function), base);
	}

	template <typename result, typename... Args>
	result invoke(Args... args)
	{
		result(*f)(Args... args) = decltype(f)(this->original_ptr);
		return f(args...);
	}

	virtual inline ~call()
	{
		this->restore();
	}

	virtual void create(void* address, void* destination, uint64_t base) override;
};

class detour : public trampoline
{
private:
	void* stub_ptr = nullptr;
	std::size_t stub_size = 0;
	std::size_t original_size = 0u;

public:
	template<class T>
	detour(void* address, T(*function), uint64_t base)
	{
		this->create(address, reinterpret_cast<void*>(function), base);
	}

	template<class T>
	detour(std::uintptr_t address, T(*function), uint64_t base)
	{
		this->create(reinterpret_cast<void*>(address), reinterpret_cast<void*>(function), base);
	}

	template <typename result, typename... Args>
	result invoke(Args... args)
	{
		result(*f)(Args... args) = decltype(f)(this->stub_ptr);
		return f(args...);
	}

	virtual inline ~detour()
	{
		this->restore();
	}

	virtual void create(void* address, void* destination, uint64_t base) override;
	virtual void restore() override;
};