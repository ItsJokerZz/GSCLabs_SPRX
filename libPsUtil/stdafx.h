// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

// TODO: reference additional headers your program requires here

#include <kernel.h>
#include <stdio.h>
#include <string.h>
#include <libdbg.h>
#include <string>
#include <chrono>
#include <algorithm>
#include <memory>
#include <net.h>
#include <sys/socket.h>

template <typename ...Args>
static void server_printf(std::string fmt, Args... args) {
    int size_s = std::snprintf(nullptr, 0, fmt.c_str(), args...) + 1; // Extra space for '\0'
    if (size_s <= 0) { return; }
    auto size = static_cast<size_t>(size_s);
    std::unique_ptr<char[]> buf(new char[size]);
    std::snprintf(buf.get(), size, fmt.c_str(), args...);

    int sock = sceNetSocket("PS4 Output", AF_INET, SOCK_STREAM, SCE_NET_IPPROTO_TCP);

    int optval = 1;
    sceNetSetsockopt(sock, SOL_SOCKET, 0x0800, (void*)&optval, sizeof(int));

    SceNetSockaddrIn sockAddr = { 0 };
    sockAddr.sin_family = AF_INET;
    sockAddr.sin_port = sceNetHtons(9902);
    sceNetInetPton(SCE_NET_AF_INET, "192.168.1.69", &sockAddr.sin_addr);

    sceNetConnect(sock, (SceNetSockaddr*)&sockAddr, sizeof(SceNetSockaddrIn));
    sceNetSend(sock, buf.get(), 0x400, 0);
    sceNetSocketClose(sock);
}

extern uint64_t BaseAddress;

//#define printf server_printf

#include "hde64.h"
#include "utilities.h"
#include "detour.h"
#include "math.h"