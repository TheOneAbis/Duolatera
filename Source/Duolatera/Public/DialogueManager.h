// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
//#include "Containers/Queue.h"
#include "GameFramework/Actor.h"
#include "DialogueManager.generated.h"

//Declare Input Enum here
UENUM(BlueprintType)
enum RequiredInput
{
	TIME			UMETA(DisplayName = "Time"),
	AX				UMETA(DisplayName = "A & X"),
	BY				UMETA(DisplayName = "B & Y"),
	SNAP			UMETA(DisplayName = "Left Thumbstick"),
	TELEPORT		UMETA(DisplayName = "Right Thumbstick"),
	GRAB			UMETA(DisplayName = "Grab"),
	TRIGGER			UMETA(DisplayName = "Trigger"),
	EVENT			UMETA(DisplayName = "Event")
};

USTRUCT(BlueprintType)
struct FDialogueEntry {
	GENERATED_BODY()

public:
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueEntry")
	FString Dialogue;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueEntry")
	TEnumAsByte<RequiredInput> Input;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueEntry")
	float Time = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueEntry")
	bool bCanContinue = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueEntry")
	UTexture2D* UI_image;
};

UCLASS()
class DUOLATERA_API ADialogueManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties

	//Declare Dialogue Entry Here
	
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	//FString AllDialogues;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	//TArray<float> EntryDuration;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	//TArray<TEnumAsByte<RequiredInput>> InputArray;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	//For visualization only
	TArray<FDialogueEntry> DialogueArray;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dialogue")
	int CurrentEntryID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	UUserWidget* DialogueWidgetComponent;

protected:

	bool StartTiming = false;
	float timer = 0;
	FDialogueEntry CurrentEntry;

	TQueue<FDialogueEntry> DialogueQ;

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	
public:	

	ADialogueManager();

	UFUNCTION(BlueprintCallable, Category = "Dialogue", meta = (Description = "Takes in player's input and tries to proceed"))
	void HandlePlayerInput(const RequiredInput PlayerInput);

	UFUNCTION(BlueprintImplementableEvent, Category = "Dialogue", meta = (Description = "Events that happens when new dialogue shows up"))
	void DuringDialogue();

	UFUNCTION(BlueprintImplementableEvent, Category = "Dialogue", meta = (Description = "Events that happens when new dialogue shows up"))
	void BetweenDialogue();

	UFUNCTION(BlueprintCallable, Category = "Dialogue", meta = (Description = "Do not call in blueprint"))
	void ManageCurrentEntryID();

	UFUNCTION(BlueprintCallable, Category = "Dialogue", meta = (Description = "Goes to the next dialogue entry"))
	void Proceed();

	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void UpdateWidget(const FString& Text);

	UFUNCTION(BlueprintImplementableEvent, Category = "Dialogue")
	void DisplayDialogue(const FString& Text);

	UFUNCTION(BlueprintImplementableEvent, Category = "Dialogue")
	void ClearDialogueUI();

	UFUNCTION(BlueprintImplementableEvent, Category = "Dialogue")
	void UpdateUIImage(UTexture2D* image);

private:

	void PopulateDialogueQueue();

	void NextEntry();
};
