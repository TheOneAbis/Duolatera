// Fill out your copyright notice in the Description page of Project Settings.

#include "DialogueManager.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"



// Sets default values
ADialogueManager::ADialogueManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;
}

// Called when the game starts or when spawned
void ADialogueManager::BeginPlay()
{
	Super::BeginPlay();
	
    PopulateDialogueQueue();

    if (!DialogueQ.IsEmpty())
    {
        CurrentEntryID = 0;
        DuringDialogue();
        NextEntry();
    }
}

void ADialogueManager::Tick(float DeltaTime) {

    Super::Tick(DeltaTime);

    if (StartTiming) 
    {
        timer += DeltaTime;
        if (timer > CurrentEntry.Time) 
        {
            StartTiming = false;
            timer = 0;
            HandlePlayerInput(TIME);
        }
    }
}

void ADialogueManager::ManageCurrentEntryID() 
{
    CurrentEntryID ++;
    if (CurrentEntryID > DialogueArray.Num())
        return;

    DuringDialogue();
}

void ADialogueManager::NextEntry() 
{
    DialogueQ.Dequeue(CurrentEntry);
    DisplayDialogue(CurrentEntry.Dialogue);
    UpdateUIImage(CurrentEntry.UI_image);
    if (CurrentEntry.Input == TIME)
        StartTiming = true;
}

void ADialogueManager::Proceed()
{
    if (!DialogueQ.IsEmpty())
    {
        ManageCurrentEntryID();

        NextEntry();
    }
}

void ADialogueManager::HandlePlayerInput(const RequiredInput PlayerInput)
{
    if (CurrentEntry.Input == PlayerInput)
    {
        ClearDialogueUI();

        if (CurrentEntry.bCanContinue) 
            Proceed();        
        else
            BetweenDialogue();
    }
}

void ADialogueManager::UpdateWidget(const FString& Text)
{
    if (DialogueWidgetComponent)
    {
        UTextBlock* TextBlock = Cast<UTextBlock>(DialogueWidgetComponent->GetWidgetFromName(TEXT("DialogueText")));
        if (TextBlock)
        {
            TextBlock->SetText(FText::FromString(Text));
        }
    }
}

void ADialogueManager::PopulateDialogueQueue()
{
    if (DialogueArray.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("No dialogues parsed from AllDialogues. Please check the input."));
        return;
    }
    for (int32 i = 0; i < DialogueArray.Num(); i ++)
    {
        DialogueQ.Enqueue(DialogueArray[i]);
    }
}

