#include "inputparams.h"

#include <string>
#include <iostream>


int main(int argc, char** argv)
{
	// Start parsing parameters
	InputParams params(argc, argv);
	std::cout << "--------------------------------------------------------------------------------" << std::endl;

	// General parameters
	int test;
	params.getOption("--test", "Test", test);

	// Done parsing parameters
	std::cout << "--------------------------------------------------------------------------------" << std::endl;




	// TODO 

	// Exit
	std::cout << "Done" << std::endl;
	return EXIT_SUCCESS;
}
