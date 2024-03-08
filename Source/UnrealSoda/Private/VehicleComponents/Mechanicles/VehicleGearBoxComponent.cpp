// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/Mechanicles/VehicleGearBoxComponent.h"
#include "Soda/UnrealSoda.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Soda/Vehicles/SodaWheeledVehicle.h"
#include "Soda/VehicleComponents/VehicleInputComponent.h"
#include "Soda/DBGateway.h"

#include "bsoncxx/builder/stream/helpers.hpp"
#include "bsoncxx/exception/exception.hpp"
#include "bsoncxx/builder/stream/document.hpp"
#include "bsoncxx/builder/stream/array.hpp"
#include "bsoncxx/json.hpp"

UVehicleGearBoxBaseComponent::UVehicleGearBoxBaseComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.Category = TEXT("Vehicle Mechanicles");
	GUI.IcanName = TEXT("SodaIcons.GearBox");

	Common.Activation = EVehicleComponentActivation::OnStartScenario;
}

FString UVehicleGearBoxBaseComponent::GetGearChar() const
{
	switch (GetGearState())
	{
	case EGearState::Neutral: return TEXT("N");
	//case EGearState::Drive: return TEXT("D");
	case EGearState::Reverse: return TEXT("R");
	case EGearState::Park: return TEXT("P");
	}
	
	if (GetGearNum() == 0)
	{
		return TEXT("N");
	}
	else if(GetGearNum() == -1)
	{
		return TEXT("R");
	}
	else
	{
		return FString::FromInt(GetGearNum());
	}
}

bool UVehicleGearBoxBaseComponent::AcceptGearFromVehicleInput(UVehicleInputComponent* VehicleInput)
{
	if (!VehicleInput)
	{
		return false;
	}

	if (VehicleInput->GetInputState().GearInputMode == EGearInputMode::ByState)
	{
		EGearState NewGear = VehicleInput->GetInputState().GearState;
		if (GetGearState() != NewGear)
		{
			SetGearByState(NewGear);
		}
	}
	if (VehicleInput->GetInputState().GearInputMode == EGearInputMode::ByNum)
	{
		int NewGear = VehicleInput->GetInputState().GearNum;
		if (GetGearNum() != NewGear)
		{
			SetGearByNum(NewGear);
		}
	}
	if (VehicleInput->GetInputState().bWasGearUpPressed)
	{
		if (GetGearNum() >= 0 && GetGearNum() < GetForwardGearsCount())
		{
			SetGearByNum(GetGearNum() + 1);
			VehicleInput->GetInputState().GearNum = GetGearNum();
			VehicleInput->GetInputState().GearState = GetGearState();
		}
		VehicleInput->GetInputState().bWasGearUpPressed = false;
	}
	if (VehicleInput->GetInputState().bWasGearDownPressed)
	{
		if (GetGearNum() > 0)
		{
			SetGearByNum(GetGearNum() - 1);
			VehicleInput->GetInputState().GearNum = GetGearNum();
			VehicleInput->GetInputState().GearState = GetGearState();
		}
		VehicleInput->GetInputState().bWasGearDownPressed = false;
	}
	return true;
}

//------------------------------------------------------------------------------------------------------------

UVehicleGearBoxSimpleComponent::UVehicleGearBoxSimpleComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.ComponentNameOverride = TEXT("Gear Box Simple");
	GUI.bIsPresentInAddMenu = true;

	PrimaryComponentTick.bCanEverTick = true;
	bTickInEditor = false;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;

	ReverseGearRatios.Add({ 10});
	ForwardGearRatios.Add({ 10});
}

bool UVehicleGearBoxSimpleComponent::OnActivateVehicleComponent()
{
	if (!Super::OnActivateVehicleComponent())
	{
		return false;
	}

	if (GetVehicle())
	{
		UObject* TorqueTransmissionObject = LinkToTorqueTransmission.GetObject<UObject>(GetOwner());
		ITorqueTransmission* TorqueTransmissionInterface = Cast<ITorqueTransmission>(TorqueTransmissionObject);
		if (TorqueTransmissionObject && TorqueTransmissionInterface)
		{
			OutputTorqueTransmission.SetInterface(TorqueTransmissionInterface);
			OutputTorqueTransmission.SetObject(TorqueTransmissionObject);
		}
		else
		{
			AddDebugMessage(EVehicleComponentHealth::Error, TEXT("Transmission isn't connected"));
		}
	}

	if (!OutputTorqueTransmission)
	{
		SetHealth(EVehicleComponentHealth::Error);
		return false;
	}

	return true;
}

void UVehicleGearBoxSimpleComponent::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();
}


void UVehicleGearBoxSimpleComponent::PassTorque(float InTorque)
{
	InTorq = InTorque;
	if (GetHealth() == EVehicleComponentHealth::Ok)
	{
		OutTorq = InTorque * Ratio;
		OutputTorqueTransmission->PassTorque(OutTorq);
	}
	else
	{
		InTorq = 0;
		OutTorq = 0;
	}
}

float UVehicleGearBoxSimpleComponent::ResolveAngularVelocity() const
{
	if (GetHealth() == EVehicleComponentHealth::Ok)
	{
		InAngularVelocity = OutputTorqueTransmission->ResolveAngularVelocity();
		OutAngularVelocity = InAngularVelocity * Ratio;
		return OutAngularVelocity;
	}
	else
	{
		InAngularVelocity = 0;
		OutAngularVelocity = 0;
		return 0;
	}
}


bool UVehicleGearBoxSimpleComponent::FindWheelRadius(float& OutRadius) const
{
	if (GetHealth() == EVehicleComponentHealth::Ok)
	{
		return OutputTorqueTransmission->FindWheelRadius(OutRadius);
	}
	return false;
}

bool UVehicleGearBoxSimpleComponent::FindToWheelRatio(float& OutRatio) const
{
	if (GetHealth() == EVehicleComponentHealth::Ok)
	{
		float PrevRatio;
		if (OutputTorqueTransmission->FindToWheelRatio(PrevRatio))
		{
			OutRatio = PrevRatio * Ratio;
			return true;
		}
	}
	return false;
}

bool UVehicleGearBoxSimpleComponent::SetGearByState(EGearState InGearState)
{
	GearState = InGearState;
	switch(InGearState)
	{
	case EGearState::Drive:
		if (GetGearNum() <= 0) return SetGearByNum(1);
		else return true;
	case EGearState::Reverse:
		if (GetGearNum() >= 0) return SetGearByNum(-1);
		else return true;
	case EGearState::Park:
	case EGearState::Neutral:
		return SetGearByNum(0);
	default:
		return false;
	};
}

bool UVehicleGearBoxSimpleComponent::SetGearByNum(int InGearNum)
{
	if (InGearNum > 0) 
	{
		if (InGearNum > ForwardGearRatios.Num())
		{
			return false;
		}
		GearState = EGearState::Drive;
		Ratio = ForwardGearRatios[InGearNum - 1];
		GearNum = InGearNum;
		return true;
	}
	else if (InGearNum < 0)
	{
		if (FMath::Abs(InGearNum) > ReverseGearRatios.Num())
		{
			return false;
		}
		GearState = EGearState::Reverse;
		Ratio = -ReverseGearRatios[FMath::Abs(InGearNum) - 1];
		GearNum = InGearNum;
		return true;
	}
	else // InGearNum == 0
	{

		if (GearState != EGearState::Park)
		{
			GearState = EGearState::Neutral;
		}
		Ratio = 0.f;
		GearNum = 0;
		return true;
	}
}

void UVehicleGearBoxSimpleComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bAcceptGearFromVehicleInput)
	{
		if (UVehicleInputComponent* VehicleInput = GetWheeledVehicle()->GetActiveVehicleInput())
		{
			AcceptGearFromVehicleInput(VehicleInput);
		}
	}

	if (bUseAutomaticGears && GetGearState() == EGearState::Drive)
	{
		if (FMath::Abs(OutAngularVelocity) * ANG2RPM > ChangeUpRPM)
		{
			if (GetGearNum() < ForwardGearRatios.Num())
			{
				SetGearByNum(GetGearNum() + 1);
				OutAngularVelocity = InAngularVelocity * Ratio;
			}
		}
		else if (FMath::Abs(OutAngularVelocity) * ANG2RPM < ChangeDownRPM)
		{
			if (GetGearNum() > 1)
			{
				SetGearByNum(GetGearNum() - 1);
				OutAngularVelocity = InAngularVelocity * Ratio;
			}
		}
	}
}

void UVehicleGearBoxSimpleComponent::DrawDebug(UCanvas* Canvas, float& YL, float& YPos)
{
	Super::DrawDebug(Canvas, YL, YPos);

	if (Common.bDrawDebugCanvas)
	{
		UFont* RenderFont = GEngine->GetSmallFont();
		Canvas->SetDrawColor(FColor::White);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("Gear: %s "), *GetGearChar()), 16, YPos);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("Ratio: %.2f "), GetGearRatio()), 16, YPos);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("InTorq: %.2f "), InTorq), 16, YPos);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("OutTorq: %.2f "), OutTorq), 16, YPos);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("InAngVel: %.2f "), InAngularVelocity), 16, YPos);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("OutAngVel: %.2f "), OutAngularVelocity), 16, YPos);
	}
}

void UVehicleGearBoxSimpleComponent::OnPushDataset(soda::FActorDatasetData& Dataset) const
{
	using bsoncxx::builder::stream::document;
	using bsoncxx::builder::stream::finalize;
	using bsoncxx::builder::stream::open_document;
	using bsoncxx::builder::stream::close_document;
	using bsoncxx::builder::stream::open_array;
	using bsoncxx::builder::stream::close_array;

	try
	{
		Dataset.GetRowDoc()
			<< std::string(TCHAR_TO_UTF8(*GetName())) << open_document
			<< "GearNum" << GetGearNum()
			<< "GearState" << int(GetGearState())
			<< "GearRatio" << GetGearRatio()
			<< "InTorq" << InTorq
			<< "OutTorq" << OutTorq
			<< "InAngVel" << InAngularVelocity
			<< "OutAngVel" << OutAngularVelocity
			<< close_document;
	}
	catch (const std::system_error& e)
	{
		UE_LOG(LogSoda, Error, TEXT("URacingSensor::OnPushDataset(); %s"), UTF8_TO_TCHAR(e.what()));
	}
}