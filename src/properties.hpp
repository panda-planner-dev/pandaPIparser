#ifndef __PROPERTIES
#define __PROPERTIES

void printProperties();

vector<string> liftedPropertyTopSort(parsed_task_network* tn);
bool isTopSortTotalOrder(vector<string> & liftedTopSort, parsed_task_network * tn);

#endif
