// Demo.catalog.g.h. protacoc 0.11.0, 7 messages. Do not edit.
#pragma once

#include <cstddef>

#include "Protaco.hpp"

/// Messages generated together with this file: name, version, schema fingerprint and canonical text, in name order.
/// The table outlives every call. Compare the tables of two builds with protaco::CatalogDifferences.
inline const protaco::SchemaEntry* DemoCatalog(size_t* count)
{
    static const protaco::SchemaEntry entries[] = {
        {"Delivery", 1, 0xA031C44Eu, "protaco-schema-1|Delivery|1|1|address:string(64);floor:int=1;doorCode:list<int>;contacts:list<string>;"},
        {"DeliveryProof", 1, 0xF769024Au, "protaco-schema-1|DeliveryProof|1|1|orderId:string(32);photo:bytes!;signatures:list<bytes>;"},
        {"DeliveryRoute", 1, 0x7A0526C6u, "protaco-schema-1|DeliveryRoute|1|1|rider:string!;stops:list<RouteStop@9f1c87c1>!;note:string=;"},
        {"Heartbeat", 1, 0x2BB85C55u, "protaco-schema-1|Heartbeat|1|0|node:string(32);seq:int64;healthy:bool=true;cpuLoad:double;uptimeSec:int;"},
        {"PizzaOrder", 1, 0x9D0CF242u, "protaco-schema-1|PizzaOrder|1|1|orderId:string(32);store:string(32);customer:string=anonymous;size:int=12;slices:int=8;woodFired:bool=true;deepDishCm:int=0;toppings:list<Topping@eb8a11d8>!;notes:list<string>;delivery:Delivery@a031c44e;"},
        {"RouteStop", 1, 0x9F1C87C1u, "protaco-schema-1|RouteStop|1|1|orderId:string(16);address:string(48);etaMin:int=15;"},
        {"Topping", 1, 0xEB8A11D8u, "protaco-schema-1|Topping|1|1|name:string(24);grams:int=30;"},
    };
    *count = sizeof(entries) / sizeof(entries[0]);
    return entries;
}
