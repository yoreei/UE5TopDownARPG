// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include <memory>
#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class CrowdPfImpl;  // Forward declaration of the implementation class

class FCrowdPFModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	void Build

private:
	std::experimental::propagate_const<std::unique_ptr<CrowdPfImpl>> pImpl;
};



class MyClass {
public:
	MyClass();
	~MyClass();  // Destructor needs to be defined where MyClassImpl is a complete type

	// Public interface methods
	void doSomething();

private:
	
};
