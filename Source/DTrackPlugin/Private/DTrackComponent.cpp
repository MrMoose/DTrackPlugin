// Copyright (c) 2017, Advanced Realtime Tracking GmbH
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
// 3. Neither the name of copyright holder nor the names of its contributors
//    may be used to endorse or promote products derived from this software
//    without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "DTrackPluginPrivatePCH.h"
#include "DTrackComponent.h"
#include "DTrackInterface.h"
#include "FDTrackPlugin.h"
#include "Engine.h"
#include "CoreUObject.h"

UDTrackComponent::UDTrackComponent(const FObjectInitializer &n_initializer)
		: Super(n_initializer) {

 	bWantsInitializeComponent = true;
 	bAutoActivate = true;
 	PrimaryComponentTick.bCanEverTick = true;
}

void UDTrackComponent::OnRegister() {

	Super::OnRegister();
	

}

void UDTrackComponent::OnUnregister() {

	Super::OnUnregister();


}

void UDTrackComponent::BeginPlay() {

	Super::BeginPlay();

	UE_LOG(DTrackPluginLog, Display, TEXT("DTrack BeginPlay() called"));

	// Attach the delegate pointer automatically to the owner of the component

	if (FDTrackPlugin::IsAvailable()) {
		IDTrackPlugin &plugin = FDTrackPlugin::Get();

		m_plugin = &plugin;

		m_plugin->start_up(this);

		if (!m_plugin->IsRemoteEnabled()) {
			UE_LOG(DTrackPluginLog, Warning, TEXT("Tracking could not be initialized"));
		}

		m_plugin->begin_tracking();

	} else {
		UE_LOG(DTrackPluginLog, Warning, TEXT("DTrack Plugin not available, cannot track this object"));
	}
}

void UDTrackComponent::EndPlay(const EEndPlayReason::Type n_reason) {

	Super::EndPlay(n_reason);

	UE_LOG(DTrackPluginLog, Display, TEXT("DTrack EndPlay() called"));

	if (m_plugin) {
		m_plugin->remove(this);
		m_plugin = nullptr;
	} else {
		UE_LOG(DTrackPluginLog, Warning, TEXT("DTrack Plugin not available, cannot stop tracking of this object"));
	}
}

void UDTrackComponent::TickComponent(float n_delta_time, enum ELevelTick n_tick_type,
	FActorComponentTickFunction *n_this_tick_function) {

	Super::TickComponent(n_delta_time, n_tick_type, n_this_tick_function);

	// Forward the component tick to the plugin
	if (m_plugin) {
		m_plugin->DTrackTick(n_delta_time);
	}
}

//Functions forwards, required implementations
bool UDTrackComponent::IsRemoteEnabled() {


	return true;

//	return DTrackIsRemoteEnabled();
}



void UDTrackComponent::body_tracking(const int32 n_body_id, const FVector &n_translation, const FRotator &n_rotation) {

	if (GetOwner()->GetClass()->ImplementsInterface(UDTrackInterface::StaticClass())) {
		IDTrackInterface::Execute_OnBodyData(GetOwner(), n_body_id, n_translation, n_rotation);
	} else {
		GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Yellow, "Owning Actor does not implement DTrack interface");
	}
}
