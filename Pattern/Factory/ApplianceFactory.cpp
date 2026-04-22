// ApplianceFactory.cpp
#include "ApplianceFactory.h"
#include <iostream>

// Concrete product implementations
void Dryer::operate() const {
    std::cout << "Dryer is operating" << std::endl;
}

void Washer::operate() const {
    std::cout << "Washer is operating" << std::endl;
}

// Concrete factory implementations
std::unique_ptr<Appliance> DryerFactory::createAppliance() const {
    return std::make_unique<Dryer>();
}

std::unique_ptr<Appliance> WasherFactory::createAppliance() const {
    return std::make_unique<Washer>();
}