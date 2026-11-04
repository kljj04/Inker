// Inker/Common/Types.hpp
// Inker Project
// Copyright (C) 2026 Inker Developer kljj04

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#define NULLPTR     nullptr
#define CONST       const
#define TRUE        true
#define FALSE       false
#define STATIC      static
#define EXTERN      extern
#define INLINE      inline
#define VOLATILE    volatile
#define PACKED      __attribute__((packed))
#define ASM         __asm__ volatile
#define NORETURN    [[noreturn]]
#define CONSTEXPR   constexpr
#define CONSTEVAL   consteval
#define CONSTINIT   constinit
#define THREADLOCAL thread_local
#define NOEXCEPT    noexcept
#define EXPLICIT    explicit
#define MUTABLE     mutable
#define FORCEINLINE __attribute__((always_inline)) inline
#define NOINLINE    __attribute__((noinline))
#define ALIGNED(N)  alignas(N)
#define ALIGNOF(T)  alignof(T)
#define LIKELY      [[likely]]
#define UNLIKELY    [[unlikely]]
#define NODISCARD   [[nodiscard]]
#define MAYBEUNUSED [[maybe_unused]]
#define DEPRECATED  [[deprecated]]
#define FALLTHROUGH [[fallthrough]]

typedef std::uint8_t      UINT8;
typedef std::uint16_t     UINT16;
typedef std::uint32_t     UINT32;
typedef std::uint64_t     UINT64;
typedef unsigned __int128 UINT128;
typedef std::uintptr_t    UINTN;
typedef std::int8_t       INT8;
typedef std::int16_t      INT16;
typedef std::int32_t      INT32;
typedef std::int64_t      INT64;
typedef __int128          INT128;
typedef std::intptr_t     INTN;
typedef char              CHAR8;
typedef char16_t          CHAR16;
typedef char8_t           UTF8;
typedef std::string       STRING;
typedef std::string_view  STRING_VIEW;
typedef float             FLOAT32;
typedef double            FLOAT64;
typedef std::size_t       SIZE;
typedef std::ptrdiff_t    SSIZE;
typedef bool              BOOLEAN;
typedef void              VOID;