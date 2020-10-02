#ifndef __PROPERTIES
#define __PROPERTIES

#include <vector>
#include "parsetree.hpp"

void printProperties();

vector<string> liftedPropertyTopSort(parsed_task_network* tn);
bool isTopSortTotalOrder(vector<string> & liftedTopSort, parsed_task_network * tn);

#endif
