#pragma once
#include <boost_1_88_0/boost/move/move.hpp>
#include <unordered_map>
#include <string>
#include "../../Utility/Error.hpp"


//template は CPPに実装が分けられないから，ここに書いちゃう
namespace Irori::Engine{
   
template <typename T>
class UniqueResourceRegistry{

private:
    BOOST_MOVABLE_BUT_NOT_COPYABLE(UniqueResourceRegistry);

public:
    std::unordered_map<std::string, T> core;

public:
    UniqueResourceRegistry(BOOST_RV_REF(UniqueResourceRegistry) rhs) = default;               //move constractor
    UniqueResourceRegistry& operator=(BOOST_RV_REF(UniqueResourceRegistry) rhs) = default;    //move assignment
    UniqueResourceRegistry(){;}
    UniqueResourceRegistry<T>& reg_resource(std::string resource_name){
        ASSERT_WITH_MSG((!core.contains(resource_name)), std::format("It already has resource : [{}], please use other one.", resource_name));
        core[resource_name];
        return *this;
    }

    T& operator[](std::string resource_name){
        ASSERT_WITH_MSG((core.contains(resource_name)), std::format("It doesn't containts resource : [{}], please use valid name.", resource_name));
        return core[resource_name];
    }
    
};

}