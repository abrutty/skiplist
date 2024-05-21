# include <iostream>
# include "skiplist.h"

int main() 
{
	SkipList<int, std::string> skipList(6);
	skipList.insert_element(1, "good");
	skipList.insert_element(3, "ok");
	skipList.insert_element(7, "fire");
	skipList.insert_element(8, "code");
	skipList.insert_element(9, "study");
	skipList.insert_element(19, "in");
	skipList.insert_element(19, "fine£¡");

	std::cout << "skipList size:" << skipList.size() << std::endl;
	skipList.display_list();
	skipList.dump_file();
	/*skipList.delete_element(1);
	skipList.delete_element(2);*/

	/*skipList.search_element(19);

	skipList.update_element(3, "zyx");
	skipList.display_list();*/
	return 0;
}