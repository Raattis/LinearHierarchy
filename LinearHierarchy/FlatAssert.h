#pragma once

#if FLAT_ASSERTS_ENABLED == true
	#define FLAT_ASSERT_IMPL(expr, fmt, ...) do{ if(!(expr)){ printf("\nAssertion failed.\nExpr: \"%s\"\nin %s() at %s:%u\n", #expr, __FUNCTION__, __FILE__, __LINE__); printf(fmt, ##__VA_ARGS__); __debugbreak(); } }while(false)

	#define FLAT_ASSERT(expr) FLAT_ASSERT_IMPL(expr, "", "") /* "\nat " #__FILE__ ":" #__LINE - %s\n", "asdf", __FUNCTION__, __FILE__, __LINE__, ""*/
	#define FLAT_ASSERTF(expr, fmt, ...) FLAT_ASSERT_IMPL(expr, "Message: " # fmt "\n", __VA_ARGS__)
#else
	#define FLAT_ASSERT(expr) do{}while(false)
	#define FLAT_ASSERTF(expr, fmt, ...) do{}while(false)
#endif
