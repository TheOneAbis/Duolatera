// Fill out your copyright notice in the Description page of Project Settings.

#include "DialogueManager.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"

// Sets default values
ADialogueManager::ADialogueManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
}

// Called when the game starts or when spawned
void ADialogueManager::BeginPlay()
{
	Super::BeginPlay();
	
    PopulateDialogueQueue();

    if (!DialogueQueue.IsEmpty())
    {
        StartDialogueQueue();
    }
}

void ADialogueManager::StartDialogueQueue()
{
    ShowNextDialogue();
}

void ADialogueManager::ShowNextDialogue()
{
    if (!DialogueQueue.IsEmpty())
    {
        if (DialogueQueue.Dequeue(CurrentDialogue))
        {
            DisplayDialogue(CurrentDialogue.Key);
        }
    }
}

void ADialogueManager::HandlePlayerInput(const FString& PlayerInput)
{
    if (CurrentDialogue.Value == PlayerInput)
    {
        if (!DialogueQueue.IsEmpty())
        {
            if (GEngine)
            {
                GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("PlayerInput: %s"), *PlayerInput));
            }
            ShowNextDialogue();
        }
        else
        {
            if (GEngine)
            {
                GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("PlayerInput: %s"), *PlayerInput));
            }
            ClearDialogueUI();
        }
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
    TArray<FString> DialogueParts;
    AllDialogues.ParseIntoArray(DialogueParts, TEXT("|"), true);

    TArray<FString> ActionParts = {
       TEXT("A/X_Button"),
       TEXT("Teleport_BothButtons"),
       TEXT("Grab_Button"),
       TEXT("Trigger_Button")
    };

    int32 NumDialogues = FMath::Min(DialogueParts.Num(), ActionParts.Num());
    if (NumDialogues == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("No dialogues parsed from AllDialogues. Please check the input."));
        return;
    }

    for (int32 i = 0; i < NumDialogues; i++)
    {
        DialogueQueue.Enqueue(TPair<FString, FString>(DialogueParts[i], ActionParts[i]));
    }
}

