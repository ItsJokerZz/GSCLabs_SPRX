#include "stdafx.h"

bool is_relatively_far(const void* start, const void* destination)
{
	const int64_t diff = size_t(destination) - (size_t(start) + JUMP_32SIZE);
	const auto small_diff = int32_t(diff);
	return diff != int64_t(small_diff);
}

std::uintptr_t get_relative_address(void* address, void* destination)
{
	return std::size_t(destination) - (std::size_t(address) + JUMP_32SIZE);
}

void write_jump32(void* address, void* destination)
{
	std::uint8_t jmp_bytes[JUMP_32SIZE] = { 0xE9, 0x90, 0x90, 0x90, 0x90 };
	*reinterpret_cast<std::uint32_t*>(jmp_bytes + 1) = get_relative_address(address, destination);

	sceKernelMprotect(address, sizeof(jmp_bytes), SCE_KERNEL_PROT_CPU_ALL);
	memcpy(address, jmp_bytes, sizeof(jmp_bytes));
}

void write_jump64(void* address, void* destination)
{
	std::uint8_t jmp_bytes[JUMP_64SIZE] = { 0xFF, 0x25, 0x00, 0x00, 0x00, 0x00, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };
	*reinterpret_cast<std::uint64_t*>(jmp_bytes + 6) = reinterpret_cast<std::uintptr_t>(destination);

	sceKernelMprotect(address, sizeof(jmp_bytes), SCE_KERNEL_PROT_CPU_ALL);
	memcpy(address, jmp_bytes, sizeof(jmp_bytes));
}

void write_call32(void* address, void* destination)
{
	std::uint8_t call_bytes[CALL_32SIZE] = { 0xE8, 0x90, 0x90, 0x90, 0x90 };
	*reinterpret_cast<std::uint32_t*>(call_bytes + 1) = get_relative_address(address, destination);

	sceKernelMprotect(address, sizeof(call_bytes), SCE_KERNEL_PROT_CPU_ALL);
	memcpy(address, call_bytes, sizeof(call_bytes));
}

void trampoline::restore()
{
	if (this->set)
	{
		sceKernelMprotect(reinterpret_cast<void*>(this->address), JUMP_32SIZE, SCE_KERNEL_PROT_CPU_ALL);
		memcpy(reinterpret_cast<void*>(this->address), this->original_bytes, JUMP_32SIZE);

		sceKernelMunmap(this->trampoline_bytes, JUMP_64SIZE);
		delete[] this->original_bytes;

		this->trampoline_bytes = nullptr;
		this->original_bytes = nullptr;
		this->address = 0;

		this->set = false;
	}
	else
	{
//#ifdef _DEBUG
//		klog("[trampoline] restore: cannot restore trampoline that was not set!\n");
//#endif
	}
}

void jump::create(void* address, void* destination, uint64_t base)
{
	if (address == 0 || destination == nullptr)
	{
//#ifdef _DEBUG
//		klog("[jump] create: \"address\" or \"destination\" NULL (%llX -> %llX)\n", address, destination);
//#endif
		return;
	}

	this->address = address;

	// save the original bytes
	this->original_bytes = new std::uint8_t[JUMP_32SIZE];
	memcpy(this->original_bytes, address, JUMP_32SIZE);

//#ifdef _DEBUG
//	klog("[jump] create: original bytes saved\n");
//#endif

	// create the trampoline and jump to it
	auto res = sceKernelMmap((void*)(base == 0 ? BaseAddress : base), JUMP_64SIZE, SCE_KERNEL_PROT_CPU_ALL, 0x1000 | 0x2, -1, 0, (void**)&this->trampoline_bytes);
	if (res < 0 || this->trampoline_bytes == nullptr)
	{
//#ifdef _DEBUG
//		klog("[jump] create: an error has occurred allocating trampoline: %X, %llX\n", res, this->trampoline_bytes);
//#endif
		return;
	}

//#ifdef _DEBUG
//	klog("[jump] create: writing trampoline jumps\n");
//#endif

	write_jump64(this->trampoline_bytes, destination);
	write_jump32(address, this->trampoline_bytes);

//#ifdef _DEBUG
//	klog("[jump] create: jump set\n");
//	hexdump(this->trampoline_bytes, JUMP_64SIZE);
//	hexdump(address, JUMP_32SIZE);
//#endif

	this->set = true;
}

void call::create(void* address, void* destination, uint64_t base)
{
	if (address == 0 || destination == nullptr)
	{
//#ifdef _DEBUG
//		klog("[call] create: \"address\" or \"destination\" NULL (%llX -> %llX)\n", address, destination);
//#endif
		return;
	}

	this->address = address;

	// save the original bytes
	this->original_bytes = new std::uint8_t[CALL_32SIZE];
	memcpy(this->original_bytes, address, CALL_32SIZE);

	original_ptr = (void*)((uint64_t)address + *(uint32_t*)((uint64_t)address + 1));

//#ifdef _DEBUG
//	klog("[call] create: original bytes saved\n");
//#endif

	// create the trampoline and call it
	auto res = sceKernelMmap((void*)(base == 0 ? BaseAddress : base), JUMP_64SIZE, SCE_KERNEL_PROT_CPU_ALL, 0x1000 | 0x2, -1, 0, (void**)&this->trampoline_bytes);
	if (res < 0 || this->trampoline_bytes == nullptr)
	{
//#ifdef _DEBUG
//		klog("[call] create: an error has occurred allocating trampoline: %X, %llX\n", res, this->trampoline_bytes);
//#endif
		return;
	}

//#ifdef _DEBUG
//	klog("[call] create: writing trampoline jumps\n");
//#endif

	write_jump64(this->trampoline_bytes, destination);
	hexdump(this->trampoline_bytes, JUMP_64SIZE);

	write_call32(address, this->trampoline_bytes);

//#ifdef _DEBUG
//	klog("[jump] create: jump set\n");
//	hexdump(this->trampoline_bytes, JUMP_64SIZE);
//	hexdump(address, JUMP_32SIZE);
//#endif

	this->set = true;
}

void detour::create(void* address, void* destination, uint64_t base)
{
	if (address == 0 || destination == nullptr)
	{
//#ifdef _DEBUG
//		klog("[detour] create: \"address\" or \"destination\" NULL (%llX -> %llX)\n", address, destination);
//#endif
		return;
	}

	//klog("Testing hook %llX\n", address);

	// calculate the necessary instruction count to detour
	auto instruction_count = 0u;
	while (instruction_count < JUMP_64SIZE)
	{
		auto ptr = reinterpret_cast<std::uint64_t>(address) + instruction_count;

		hde64s hs;
		instruction_count += hde64_disasm(reinterpret_cast<void*>(ptr), &hs);

		if (hs.flags & F_ERROR)
		{
			//klog("Fatal error\n");
			return;
		}
	}
//
//#ifdef _DEBUG
//	klog("[detour] create: instruction_count = %i\n", instruction_count);
//#endif

	this->address = address;

	// save the original instructions
	this->original_size = instruction_count;
	this->original_bytes = new std::uint8_t[instruction_count];
	memcpy(this->original_bytes, this->address, instruction_count);
//
//#ifdef _DEBUG
//	klog("[detour] create: original bytes saved\n", instruction_count);
//#endif

	// allocate the stub
	this->stub_size = instruction_count + JUMP_64SIZE;
	auto res = sceKernelMmap(0, this->stub_size, SCE_KERNEL_PROT_CPU_ALL, 0x1000 | 0x2, -1, 0, &this->stub_ptr);
	if (res < 0 || this->stub_ptr == nullptr)
	{
//#ifdef _DEBUG
//		klog("[detour] create: sceKernelMmap returned error = %X\n", res);
//#endif
		return;
	}

	// write original instructions to the stub
	memcpy(this->stub_ptr, this->address, instruction_count);

	// write the jump back to main function to the stub
	auto stub_jump = reinterpret_cast<std::uint64_t>(this->stub_ptr) + instruction_count;
	auto stub_return = reinterpret_cast<std::uint64_t>(this->address) + instruction_count;
//
//#ifdef _DEBUG
//	klog("[detour] create: writing detour jumps\n");
//#endif

	write_jump64(reinterpret_cast<void*>(stub_jump), reinterpret_cast<void*>(stub_return));
	write_jump64(this->address, destination);

//#ifdef _DEBUG
//	klog("[detour] create: detour set\n");
//	hexdump(this->stub_ptr, this->stub_size);
//	hexdump(this->address, JUMP_64SIZE);
//#endif

	this->set = true;
}

void detour::restore()
{
	if (this->set)
	{
		sceKernelMprotect(reinterpret_cast<void*>(this->address), this->original_size, SCE_KERNEL_PROT_CPU_ALL);
		memcpy(reinterpret_cast<void*>(this->address), this->original_bytes, this->original_size);

		sceKernelMunmap(this->stub_ptr, this->stub_size);

		delete[] this->original_bytes;

		this->stub_ptr = nullptr;
		this->stub_size = 0;
		this->original_bytes = nullptr;
		this->original_size = 0;
		this->address = 0;

		this->set = false;
	}
	else
	{
//#ifdef _DEBUG
//		klog("[detour] restore: cannot restore detour that was not set!\n");
//#endif
	}
}