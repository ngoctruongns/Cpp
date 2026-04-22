// ApplianceFactory.h
#pragma once

#include <memory>

// Base class for products
class Appliance {
public:
    virtual ~Appliance() = default;
    virtual void operate() const = 0;
};

// Concrete product classes
class Dryer : public Appliance {
public:
    void operate() const override;
};

class Washer : public Appliance {
public:
    void operate() const override;
};

// Abstract factory class
class ApplianceFactory {
public:
    virtual ~ApplianceFactory() = default;
    virtual std::unique_ptr<Appliance> createAppliance() const = 0;
};

// Concrete factory classes
class DryerFactory : public ApplianceFactory {
public:
    std::unique_ptr<Appliance> createAppliance() const override;
};

class WasherFactory : public ApplianceFactory {
public:
    std::unique_ptr<Appliance> createAppliance() const override;
};