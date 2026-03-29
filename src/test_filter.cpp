#include <iostream>
#include <vector>
#include "StringFilter.h"

int main()
{
	StringFilter filter;

	filter.addPartial( "GLWorker" );
	filter.addPartial( "std::" );
	filter.addPartial( "TSSimpleArray" );

	// clang-format off
	std::vector<std::pair<std::string, bool>> tests = {
		{ "GLWorker::GLWorker", true },
		{ "MyClass::operator+", false },
		{ "/usr/lib/libstdc++.6.dylib::std::ostream::_M_insert<unsigned_long_long>", true },
		{ "/usr/lib/libstdc++.6.dylib::std::ostream::_M_insert<void_const*>", true },
		{ "std::vector<char_const*,std::allocator<char_const*>>::_M_insert_aux", true },
		{ "std::vector<unsigned_int,std::allocator<unsigned_int>>::_M_insert_aux", true },
		{ "std::vector<System_SFile::ListfileEntry,std::allocator<System_SFile::ListfileEntry>>::_M_insert_aux", true },
		{ "TSSimpleArray<CMapObjDefGroup*>::FatalArrayBounds(unsigned long) const", true }
	};
	// clang-format on

	std::cout << "=== Test Results ===\n";
	for( const auto &[input, expectedSkip] : tests )
	{
		bool actualSkip = filter.match( input );
		bool passed = ( actualSkip == expectedSkip );

		std::cout << "Input:    " << input << "\n";
		std::cout << "Output:   " << ( actualSkip ? "<skip>" : input ) << "\n";
		std::cout << "Expected: " << ( expectedSkip ? "<skip>" : input ) << "\n";
		std::cout << "Status:   " << ( passed ? "[OK]" : "[FAIL]" ) << "\n";
		std::cout << "---------------------\n";
	}

	return 0;
}