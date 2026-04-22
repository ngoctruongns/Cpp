// FactoryMethodExample.cpp
#include "ApplianceFactory.h"
#include <iostream>

int main() {
    // Create a dryer using the dryer factory
    ApplianceFactory* dryerFactory = new DryerFactory();
    std::unique_ptr<Appliance> dryer = dryerFactory->createAppliance();
    dryer->operate();
    delete dryerFactory;

    // Create a washer using the washer factory
    ApplianceFactory* washerFactory = new WasherFactory();
    std::unique_ptr<Appliance> washer = washerFactory->createAppliance();
    washer->operate();
    delete washerFactory;

    return 0;
}