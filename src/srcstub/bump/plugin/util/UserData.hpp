#pragma once
#include <memory>
#include <typeinfo>
#include <vector>
#include <iostream>


namespace plugin::util
{

    class UserData
    {
    public:
        UserData(){};
        UserData(const char* msg)
        {
            auto ptr = std::make_shared<std::string>(msg);
            userData = ptr;
            rawUserData = ptr.get();
            typeInfo = const_cast<std::type_info*>(&typeid(ptr.get()));
        }
        template<typename T>
        UserData(std::shared_ptr<T> data){
            userData = data;
            rawUserData = userData.get();
            typeInfo = const_cast<std::type_info*>(&typeid(data.get()));
        };
        template<typename T>
        UserData(T* data){
            rawUserData = data;
            typeInfo = const_cast<std::type_info*>(&typeid(data));
        };

        template<typename T, typename ...fnParamTypes>
        void Set(fnParamTypes... args)
        {
            auto ptr = std::make_shared<T>(std::forward<fnParamTypes>(args)...);
            userData = ptr;
            rawUserData = userData.get();
            typeInfo = const_cast<std::type_info*>(&typeid(ptr.get()));
        }


        const std::type_info& GetTypeInfo() const { return *typeInfo; }
        template <class T> bool Is() const {
            return GetTypeInfo() == typeid(T*);
        }

        template<class T2>
        auto Get()
        {
            return reinterpret_cast<T2*>(rawUserData);
        }

        template<class T2>
        T2* GetSpecificaly()
        {
            if (Is<T2>())
            {
                return static_cast<T2*>(rawUserData);
            }
            return nullptr;
        }

        void* GetRaw()
        {
            return rawUserData;
        }

    public:
        std::shared_ptr<void> userData; //for ownership semantics.
        void* rawUserData;
        std::type_info* typeInfo;
    };
}
