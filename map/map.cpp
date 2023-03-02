#include <iostream>
#include <map>

int main()
{
// Create a map of strings to integers
std::map<std::string, int> map;

// Insert some values into the map
map["one"] = 1;
map["two"] = 2;
map["three"] = 3;
std::cout << "int size:" << sizeof(int) << std::endl;

// Get an iterator pointing to the first element in the map
std::map<std::string, int>::iterator it = map.begin();

// Iterate through the map and print the elements
while (it != map.end())
{
	std::cout << "Key: " << it->first << ", Value: " << it->second << std::endl;
	++it;
}

 // Print the size of the map
std::cout << "Size of map: " << map.size() << std::endl;

// print key
std::cout << map["one"] << std::endl;
map.erase("two");
map.insert({"nice", 9});

it = map.begin();
while (it != map.end())
{
	std::cout << "Key: " << it->first << ", Value: " << it->second << std::endl;
	++it;
}

/* Find */
it = map.find("tow");
if (it != map.end())
std::cout << "find: " << it->second << std::endl;
else
std::cout << "no find key" << std::endl;

return 0;
}
