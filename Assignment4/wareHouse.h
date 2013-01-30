#include <set>
#include <string>
#include "date.h"
#include "foodItem.h"


#ifndef WAREHOUSE_H
#define WAREHOUSE_H

namespace assignment4
{
  class ware_house
  {
  public:
    ware_house(std::set<food_item> food_set, const std::string name, 
	       std::set<date> dates);
    ware_house(const ware_house & other);
    ~ware_house();

    

    std::set<food_item> food_set; 
    const std::string name;
    std::set<date> dates;

  private: 
    
  };
}

#endif
